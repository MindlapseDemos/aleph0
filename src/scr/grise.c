#include <cgmath/cgmath.h>
#include "demo.h"
#include "imago2.h"
#include "noise.h"
#include "rbtree.h"
#include "screen.h"
#include "treestor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rlebmap.h"
#include "util.h"

/* APPROX. 170 FPS Minimum */

static void updatePropeller(float t);

#define BG_FILENAME "data/grise.png"
#define GROBJ_01_FILENAME "data/grobj_01.png"

#define BB_SIZE 512 /* Let's use a power of 2. Maybe we'll zoom/rotate the effect */

/* Every backBuffer scanline is guaranteed to have that many dummy pixels before and after */
#define PIXEL_PADDING 32

/* Make sure this is less than PIXEL_PADDING*/
#define MAX_DISPLACEMENT 16

#define MIN_SCROLL PIXEL_PADDING
#define MAX_SCROLL (backgroundW - FB_WIDTH - MIN_SCROLL)

#define FAR_SCROLL_SPEED 15.0f
#define NEAR_SCROLL_SPEED 120.0f

#define HORIZON_HEIGHT 100
#define REFLECTION_HEIGHT (240 - HORIZON_HEIGHT)

#define NORMALMAP_SCANLINE 372

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static void convert32To16(unsigned int *src32, unsigned short *dst16, unsigned int pixelCount);
static void processNormal();
static void initScrollTables();
static void updateScrollTables(float dt);

static unsigned short *background = 0;
static int backgroundW = 0;
static int backgroundH = 0;

static unsigned int lastFrameTime = 0;
static float lastFrameDuration = 0.0f;

static short *displacementMap;

static unsigned short *backBuffer;

static float scrollScaleTable[REFLECTION_HEIGHT];
static float scrollTable[REFLECTION_HEIGHT];
static int scrollTableRounded[REFLECTION_HEIGHT];
static int scrollModTable[REFLECTION_HEIGHT];
static float nearScrollAmount = 0.0f;

static unsigned char miniFXBuffer[1024];

static RleBitmap *grobj = 0;
static RleBitmap *rlePropeller = 0;

static struct screen scr = {"galaxyrise", init, destroy, start, 0, draw};

struct rbtree *bitmap_props = 0;

struct screen *grise_screen(void) {
	return &scr;
}

static int init(void) {
	unsigned char *tmpBitmap;
	int tmpBitmapW, tmpBitmapH;
	char *propsFile = "data/grise/props.txt";
	struct ts_node *node = 0;
	struct ts_node *props = 0;
	struct ts_attr *attr = 0;
	
	struct rbnode *bitmap_node = 0;
	char *filename = 0;
	char tb[100];
	int i;

	/* Load props file */
	props = ts_load(propsFile);
	if (!props) {
		printf("Failed to load props file: %s\n", propsFile);
		return 1;
	}

	/* Keep all bitmaps to be loaded */
	bitmap_props = rb_create(RB_KEY_STRING);

	for (node = props->child_list; node; node = node->next) {
		printf("Child: %s\n", node->name);
		if (!strcmp(node->name, "image")) {
			attr = ts_lookup(node, "image.file");
			if (!attr) {
				printf("Cannot file attribute 'file' for %s node\n", node->name);
				return 1;
			}
			printf("\tFile: %s\n", attr->val.str);
			rb_insert(bitmap_props, attr->val.str, attr->val.str);
		}
	}

	rb_begin(bitmap_props);
	bitmap_node = rb_next(bitmap_props);
	while (bitmap_node) {
		filename = bitmap_node->data;
		printf("Encoding bitmap to rle: %s\n", filename);
		sprintf(tb, "data/grise/%s", filename);
		bitmap_node->data = rleFromFile(tb);
		printf("\tW: %d H: %d\n", ((RleBitmap*)bitmap_node->data)->w, ((RleBitmap*)bitmap_node->data)->h);
		bitmap_node = rb_next(bitmap_props);
	}


	// TODO: Free the config file and bitmap tree

	/* Allocate back buffer */
	backBuffer = (unsigned short *)calloc(BB_SIZE * BB_SIZE, sizeof(unsigned short));

	/* grise.png contains the background (horizon), baked reflection and normalmap for
	 * displacement */
	if (!(background =
		  img_load_pixels(BG_FILENAME, &backgroundW, &backgroundH, IMG_FMT_RGBA32))) {
		fprintf(stderr, "failed to load image " BG_FILENAME "\n");
		return -1;
	}

	/* Convert to 16bpp */
	convert32To16((unsigned int *)background, background,
		      backgroundW * NORMALMAP_SCANLINE); /* Normalmap will keep its 32 bit color */

	/* Load reflected objects */
	if (!(tmpBitmap =
		  img_load_pixels(GROBJ_01_FILENAME, &tmpBitmapW, &tmpBitmapH, IMG_FMT_GREY8))) {
		fprintf(stderr, "failed to load image " GROBJ_01_FILENAME "\n");
		return -1;
	}

	grobj = rleEncode(0, tmpBitmap, tmpBitmapW, tmpBitmapH);

	img_free_pixels(tmpBitmap);

	initScrollTables();

	processNormal();

	return 0;
}

static void destroy(void) {
	free(backBuffer);
	backBuffer = 0;

	img_free_pixels(background);

	rleDestroy(grobj);
}

static void start(long trans_time) { lastFrameTime = time_msec; }


struct
{
	float r[FB_WIDTH];
	float b[FB_WIDTH];
	float g[FB_WIDTH];
} tvBuffers;

#define XR(c) ((c >> 11) & 31)
#define XG(c) ((c >> 5) & 63)
#define XB(c) (c & 31)
#define CRGB(r, g, b) ((r << 11) | (g << 5) | b)

static INLINE float sample(float *buffer, float u)
{
	int i0, i1;

	u = u * FB_WIDTH;
	i0 = (int) u;
	i1 = i0 + 1;
	i0 = i0 < 0 ? 0 : i0;
	i1 = i1 >= FB_WIDTH ? (FB_WIDTH - 1) : i1;

	u -= i0;

	return buffer[i0] * (1.0f - u) + buffer[i1] * u;
}

static void oldTv(unsigned short *dst, unsigned short *src, float sr, float tr, float sg, float tg, float sb, float tb)
{
	int i;
	unsigned short *pixel = src;
	float *r = tvBuffers.r;
	float *g = tvBuffers.g;
	float *b = tvBuffers.b;
	float u, ur, ug, ub;
	int ir, ig, ib;

	for (i=0; i<FB_WIDTH; i++) {
		// decompose pixel
		*r++ = XR(*pixel);
		*g++ = XG(*pixel);
		*b++ = XB(*pixel);

		pixel++;
	}

	pixel = dst;
	for (i=0; i<FB_WIDTH; i++) {
		u = (float) i / (float) (FB_WIDTH - 1);
		u = 2.0f * u - 1.0f;
		ur = u * sr + tr;
		ug = u * sg + tg;
		ub = u * sb + tb;
		ur = 0.5f * ur + 0.5f;
		ug = 0.5f * ug + 0.5f;
		ub = 0.5f * ub + 0.5f;
		
		ir = sample(tvBuffers.r, ur) + 0.5f;
		ig = sample(tvBuffers.g, ug) + 0.5f;
		ib = sample(tvBuffers.b, ub) + 0.5f;


		*pixel++ = CRGB(ir, ig, ib);
	}
}

float lerp(float a, float b, float t)
{
	return (1.0f - t) * a + t * b;
}

float rScale(int scanline, float t)
{
	float l = noise2(scanline + 0.5, t);
	return lerp(0.9f, 1.0f, l);
}

float gScale(int scanline, float t)
{
	float l = noise2(scanline + 0.5, t * 1.7f);
	return lerp(0.9f, 1.0f, l);
}

float rShift(int scanline, float t)
{
	float l = noise2(1.7f * scanline + 0.5, t * 1.7f);
	return lerp(0.01f, 0.05f, l);
}

float gShift(int scanline, float t)
{
	float l = noise2(0.7f * scanline + 0.5, t * 1.7f);
	return lerp(0.01f, 0.05f, l);
}

float bShift(int scanline, float t)
{
	float l = noise2(0.87f * scanline + 0.5, t * 1.7f);
	return lerp(0.01f, 0.05f, l);
}

float bScale(int scanline, float t)
{
	float l = noise2(scanline + 0.5, t * 0.97f);
	return lerp(0.95f, 1.0f, l);
}

static void draw(void) {
	float time_sec = time_msec / 1000.0f;
	int scroll = MIN_SCROLL + (MAX_SCROLL - MIN_SCROLL) * mouse_x / FB_WIDTH;
	unsigned short *dst = backBuffer + PIXEL_PADDING;
	unsigned short *src = background + scroll;
	int scanline = 0;
	int i = 0;
	short *dispScanline;
	int d;
	int accum = 0;
	int md, sc;
	int scrolledIndex;
	struct rbnode *bitmap_node = 0;
	

	lastFrameDuration = (time_msec - lastFrameTime) / 1000.0f;
	lastFrameTime = time_msec;

	/* Update mini-effects here */
	updatePropeller(4.0f * time_msec / 1000.0f);

	/* First, render the horizon */
	for (scanline = 0; scanline < HORIZON_HEIGHT; scanline++) {
		memcpy(dst, src, FB_WIDTH * 2);
		src += backgroundW;
		dst += BB_SIZE;
	}

	/* Create scroll offsets for all scanlines of the normalmap */
	updateScrollTables(lastFrameDuration);

	/* Render the baked reflection one scanline below its place, so that
	 * the displacement that follows will be done in a cache-friendly way
	 */
	src -= PIXEL_PADDING; /* We want to also fill the PADDING pixels here */
	dst = backBuffer + (HORIZON_HEIGHT + 1) * BB_SIZE;
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		memcpy(dst, src, (FB_WIDTH + PIXEL_PADDING) * 2);
		src += backgroundW;
		dst += BB_SIZE;
	}

	/* Blit reflections first, to be  displaced */
	for (i = 0; i < 5; i++)
		/*rleBlitScaleInv(rlePropeller, backBuffer + PIXEL_PADDING, FB_WIDTH, FB_HEIGHT,
				BB_SIZE, 134 + (i - 3) * 60, 200, 1.0f, 1.8f);*/

	/* Perform displacement */
	dst = backBuffer + HORIZON_HEIGHT * BB_SIZE + PIXEL_PADDING;
	src = dst + BB_SIZE; /* The pixels to be displaced are 1 scanline below */
	dispScanline = displacementMap;
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {

		md = scrollModTable[scanline];
		sc = scrollTableRounded[scanline];
		accum = 0;

		for (i = 0; i < FB_WIDTH; i++) {
			/* Try to immitate modulo without the division */
			if (i == md)
				accum += md;
			scrolledIndex = i - accum + sc;
			if (scrolledIndex >= md)
				scrolledIndex -= md;

			/* Displace */
			d = dispScanline[scrolledIndex];
			*dst++ = src[i + d];
		}
		src += backgroundW;
		dst += BB_SIZE - FB_WIDTH;
		dispScanline += backgroundW;
	}

	rb_begin(bitmap_props);
	struct rbnode *node = rb_next(bitmap_props);
	while (node) {
		RleBitmap *rle = node->data;
		rleBlitScale(node->data, backBuffer + PIXEL_PADDING, FB_WIDTH, FB_HEIGHT, BB_SIZE,
			100, 120, 2, 2);
		node = rb_next(bitmap_props);
	}

	/* Then after displacement, blit the objects */
	for (i = 0; i < 5; i++)
		/*rleBlit(rlePropeller, backBuffer + PIXEL_PADDING, FB_WIDTH, FB_HEIGHT, BB_SIZE,
			134 + (i - 3) * 60, 100);*/

	/* Blit effect to framebuffer */
	src = backBuffer + PIXEL_PADDING;
	dst = fb_pixels;
	for (scanline = 0; scanline < FB_HEIGHT; scanline++) {
		memcpy(dst, src, FB_WIDTH * 2);
		/*
		oldTv(dst, src, 
			rScale(scanline, time_sec), rShift(scanline, time_sec),
			gScale(scanline, time_sec), gShift(scanline, time_sec),
			bScale(scanline, time_sec), bShift(scanline, time_sec));
			*/
		src += BB_SIZE;
		dst += FB_WIDTH;
	}

	swap_buffers(0);
}

/* src and dst can be the same */
static void convert32To16(unsigned int *src32, unsigned short *dst16, unsigned int pixelCount) {
	unsigned int p;
	while (pixelCount) {
		p = *src32++;
		*dst16++ = ((p << 8) & 0xF800)	   /* R */
			   | ((p >> 5) & 0x07E0)   /* G */
			   | ((p >> 19) & 0x001F); /* B */
		pixelCount--;
	}
}

/* Normal map preprocessing */
/* Scale normal with depth and unpack R component (horizontal component) */
static void processNormal() {
	int scanline;
	int i;
	int x;
	short maxDisplacement = 0;
	short minDisplacement = 256;
	unsigned short *dst;
	short *dst2;
	unsigned int *normalmap = (unsigned int *)background;
	normalmap += NORMALMAP_SCANLINE * backgroundW;
	dst = (unsigned short *)normalmap;
	displacementMap = (short *)dst;
	dst2 = displacementMap;

	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		scrollModTable[scanline] = (int)(backgroundW / scrollScaleTable[scanline] + 0.5f);
		for (i = 0; i < backgroundW; i++) {
			x = (int)(i * scrollScaleTable[scanline] + 0.5f);
			if (x < backgroundW) {
				*dst = (unsigned short)(normalmap[x] >> 8) & 0xFF;
				if ((short)*dst > maxDisplacement)
					maxDisplacement = (short)(*dst);
				if ((short)*dst < minDisplacement)
					minDisplacement = (short)(*dst);
			} else {
				*dst = 0;
			}
			dst++;
		}
		normalmap += backgroundW;
	}

	if (maxDisplacement == minDisplacement) {
		printf("Warning: grise normalmap fucked up\n");
		return; 
	}

	/* Second pass - subtract half maximum displacement to displace in both directions */
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		for (i = 0; i < backgroundW; i++) {
			/* Remember that MIN_SCROLL is the padding around the screen, so ti's the
			 * maximum displacement we can get (positive & negative) */
			*dst2 = 2 * MAX_DISPLACEMENT * (*dst2 - minDisplacement) /
				    (maxDisplacement - minDisplacement) -
				MAX_DISPLACEMENT;
			*dst2 = (short)((float)*dst2 / scrollScaleTable[scanline] +
					0.5f); /* Displacements must also scale with distance*/
			dst2++;
		}
	}
}

static float distanceScale(int scanline) {
	float farScale, t;
	farScale = (float)NEAR_SCROLL_SPEED / (float)FAR_SCROLL_SPEED;
	t = (float)scanline / ((float)REFLECTION_HEIGHT - 1);
	return 1.0f / (1.0f / farScale + (1.0f - 1.0f / farScale) * t);
}

static void initScrollTables() {
	int i = 0;
	for (i = 0; i < REFLECTION_HEIGHT; i++) {
		scrollScaleTable[i] = distanceScale(i);
		scrollTable[i] = 0.0f;
		scrollTableRounded[i] = 0;
	}
}

static void updateScrollTables(float dt) {
	int i = 0;

	nearScrollAmount += dt * NEAR_SCROLL_SPEED;
	nearScrollAmount = (float)fmod(nearScrollAmount, 512.0f);

	for (i = 0; i < REFLECTION_HEIGHT; i++) {
		scrollTable[i] = nearScrollAmount / scrollScaleTable[i];
		scrollTableRounded[i] = (int)(scrollTable[i] + 0.5f) % scrollModTable[i];
	}
}

/* -------------------------------------------------------------------------------------------------
 *                                   PROPELLER STUFF
 * -------------------------------------------------------------------------------------------------
 */

#define PROPELLER_CIRCLE_RADIUS 18
#define PROPELLER_CIRCLE_RADIUS_SQ (PROPELLER_CIRCLE_RADIUS * PROPELLER_CIRCLE_RADIUS)

static struct {
	int circleX[3];
	int circleY[3];
} propellerState;

static void updatePropeller(float t) {
	int i, j;
	int cx, cy, count = 0;
	unsigned char *dst;
	float x = 0.0f;
	float y = 18.0f;
	float nx, ny;
	float cost, sint;
	static float sin120 = 0.86602540378f;
	static float cos120 = -0.5f;

	/* Rotate */
	sint = sin(t);
	cost = cos(t);
	nx = x * cost - y * sint;
	ny = y * cost + x * sint;
	x = nx;
	y = ny;
	propellerState.circleX[0] = (int)(x + 0.5f) + 16;
	propellerState.circleY[0] = (int)(y + 0.5f) + 16;

	/* Rotate by 120 degrees, for the second circle */
	nx = x * cos120 - y * sin120;
	ny = y * cos120 + x * sin120;
	x = nx;
	y = ny;
	propellerState.circleX[1] = (int)(x + 0.5f) + 16;
	propellerState.circleY[1] = (int)(y + 0.5f) + 16;

	/* 3rd circle */
	nx = x * cos120 - y * sin120;
	ny = y * cos120 + x * sin120;
	x = nx;
	y = ny;
	propellerState.circleX[2] = (int)(x + 0.5f) + 16;
	propellerState.circleY[2] = (int)(y + 0.5f) + 16;

	/* Write effect to the mini fx buffer*/
	dst = miniFXBuffer;
	for (j = 0; j < 32; j++) {
		for (i = 0; i < 32; i++) {
			count = 0;

			/* First circle */
			cx = propellerState.circleX[0] - i;
			cy = propellerState.circleY[0] - j;
			if (cx * cx + cy * cy < PROPELLER_CIRCLE_RADIUS_SQ)
				count++;

			/* 2nd circle */
			cx = propellerState.circleX[1] - i;
			cy = propellerState.circleY[1] - j;
			if (cx * cx + cy * cy < PROPELLER_CIRCLE_RADIUS_SQ)
				count++;

			/* 3rd circle */
			cx = propellerState.circleX[2] - i;
			cy = propellerState.circleY[2] - j;
			if (cx * cx + cy * cy < PROPELLER_CIRCLE_RADIUS_SQ)
				count++;

			*dst++ = count >= 2;
		}
	}

	/* Then, encode to rle */
	rlePropeller = rleEncode(rlePropeller, miniFXBuffer, 32, 32);

}
