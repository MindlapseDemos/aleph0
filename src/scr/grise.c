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

#define ART_FILENAME "data/grise/art.png"
#define NORMAL_FILENAME "data/grise/normal.png"
#define MAX_DISPLACEMENT 16 /* Maximum horizontal displacement for a reflection pixel. */
#define EFFECT_BUFFER_PADDING MAX_DISPLACEMENT
#define EFFECT_BUFFER_W (FB_WIDTH + 2 * EFFECT_BUFFER_PADDING)
#define EFFECT_BUFFER_H (FB_HEIGHT + 1)
#define REFLECTION_HEIGHT 91
#define HORIZON_HEIGHT (FB_HEIGHT - REFLECTION_HEIGHT)
#define MIN_SCROLL EFFECT_BUFFER_PADDING
#define MAX_SCROLL (artW- FB_WIDTH - MIN_SCROLL)
#define FAR_SCROLL_SPEED 15.0f
#define NEAR_SCROLL_SPEED 120.0f

static int allSystemsGo = 0;
static unsigned short *artBuffer = 0; /* Loaded from file - contains the background */
static unsigned short *effectBuffer = 0; /* FB-sized buffer plus some padding to optimize the effect. */
static short *displacementMap = 0; /* Preprocessed normal - only x component distance-scaled. */

/* Scrolling */
static float scrollScaleTable[REFLECTION_HEIGHT];
static float scrollTable[REFLECTION_HEIGHT];
static int scrollTableRounded[REFLECTION_HEIGHT];
static int scrollModTable[REFLECTION_HEIGHT];
static float nearScrollAmount = 0.0f;

static int loadAndProcessNormal();
static void updateScrollTables(float dt);













static void updatePropeller(float t);

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static void convert32To16(unsigned int *src32, unsigned short *dst16, unsigned int pixelCount);
static void initScrollTables();
static int artW = 0;
static int artH = 0;

static unsigned int lastFrameTime = 0;
static float lastFrameDuration = 0.0f;



static unsigned char miniFXBuffer[1024];

static RleBitmap *rlePropeller = 0;

static struct screen scr = {"galaxyrise", init, destroy, start, 0, draw};

struct rbtree *bitmap_props = 0;

struct screen *grise_screen(void) {
	return &scr;
}

#ifdef _WIN32
#define INIT_ERROR exit(0); return 1
#else
#define INIT_ERROR return 1
#endif /* WIN32 */

static int init(void) {
	char *propsFile = "data/grise/props.txt";
	struct ts_node *node = 0;
	struct ts_node *props = 0;
	struct ts_attr *attr = 0;
	int i=0;
	
	struct rbnode *bitmap_node = 0;
	char *filename = 0;
	char tb[100];

	/* Load props file */
	props = ts_load(propsFile);
	if (!props) {
		printf("Failed to load props file: %s\n", propsFile);
		INIT_ERROR;
	}

	/* Keep all bitmaps to be loaded */
	bitmap_props = rb_create(RB_KEY_STRING);

	for (node = props->child_list; node; node = node->next) {
		printf("Child: %s\n", node->name);
		if (!strcmp(node->name, "image")) {
			attr = ts_lookup(node, "image.file");
			if (!attr) {
				printf("Cannot find attribute 'file' for %s node\n", node->name);
				INIT_ERROR;
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
	effectBuffer = (unsigned short *)calloc(EFFECT_BUFFER_W * EFFECT_BUFFER_H, sizeof(unsigned short));

	if (!(artBuffer =
		  img_load_pixels(ART_FILENAME, &artW, &artH, IMG_FMT_RGB565))) {
		fprintf(stderr, "failed to load image " ART_FILENAME "\n");
		INIT_ERROR;
	}

	
	initScrollTables();
	if (loadAndProcessNormal()) {
		fprintf(stderr, "failed to process normalmap\n");
		INIT_ERROR;
	}

	allSystemsGo = 1;
	return 0;
}

static void destroy(void) {
	free(artBuffer);
	artBuffer = 0;

	free(effectBuffer);
	effectBuffer = 0;

	free(displacementMap);
	displacementMap = 0;

	img_free_pixels(artBuffer);
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
	int scroll = 0;
	unsigned short *dst = 0;
	unsigned short *src = 0;
	int scanline = 0;
	int i = 0;
	short *dispScanline;
	int d;
	int accum = 0;
	int md, sc;
	int scrolledIndex;
	struct rbnode *bitmap_node = 0;

	/*******************************************************************************/
	if (!allSystemsGo) {
		for (i=0; i<FB_WIDTH * FB_HEIGHT; i++) {
			fb_pixels[i] = rand();
		}
		swap_buffers(0);
		return;
	}
	/*******************************************************************************/

	lastFrameDuration = (time_msec - lastFrameTime) / 1000.0f;
	lastFrameTime = time_msec;

	/* Update scroll */
	scroll = MIN_SCROLL + (MAX_SCROLL - MIN_SCROLL) * mouse_x / FB_WIDTH;

	/* First, render the horizon */
	dst = effectBuffer + EFFECT_BUFFER_PADDING;
	src = artBuffer + scroll;
	for (scanline = 0; scanline < HORIZON_HEIGHT; scanline++) {
		memcpy(dst, src, FB_WIDTH * 2);
		src += artW;
		dst += EFFECT_BUFFER_W;
	}

	/* Create scroll offsets for all scanlines of the normalmap */
	updateScrollTables(lastFrameDuration);

	/* Render the baked reflection one scanline below its place, so that
	 * the displacement that follows will be done in a cache-friendly way
	 */
	src -= EFFECT_BUFFER_PADDING; /* We want to also fill the PADDING pixels here */
	dst = effectBuffer + (HORIZON_HEIGHT + 1) * EFFECT_BUFFER_W;
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		memcpy(dst, src, EFFECT_BUFFER_W * 2);
		src += artW;
		dst += EFFECT_BUFFER_W;
	}

	/* Update mini-effects here */
	updatePropeller(4.0f * time_msec / 1000.0f);

	/* Blit minifx reflections first, to be  displaced */
	for (i = 0; i < 5; i++) {
		rleBlitScale(rlePropeller, effectBuffer + EFFECT_BUFFER_PADDING, FB_WIDTH, FB_HEIGHT, EFFECT_BUFFER_W,
                  134 + (i - 3) * 60, 200, 1.0f, -1.8f);
	}

	/* Perform displacement */
	dst = effectBuffer + HORIZON_HEIGHT * EFFECT_BUFFER_W + EFFECT_BUFFER_PADDING;
	src = dst + EFFECT_BUFFER_W; /* The pixels to be displaced are 1 scanline below */
	dispScanline = displacementMap;
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {

		md = scrollModTable[scanline];
		sc = scrollTableRounded[scanline];
		accum = 0;

		for (i = 0; i < FB_WIDTH; i++) {
			/* Try to immitate modulo without the division */
			if (i == md) {
				accum += md;
			}
			scrolledIndex = i - accum + sc;
			if (scrolledIndex >= md) {
				scrolledIndex -= md;
			}

			/* Displace */
			d = dispScanline[scrolledIndex];
			*dst++ = src[i + d];
		}
		src += EFFECT_BUFFER_W;
		dst += EFFECT_BUFFER_W - FB_WIDTH;
		dispScanline += artW;
	}

	/* Then after displacement, blit the objects */
	for (i = 0; i < 5; i++) {
		rleBlitScale(rlePropeller, effectBuffer + EFFECT_BUFFER_PADDING, FB_WIDTH, FB_HEIGHT, EFFECT_BUFFER_W,
                  134 + (i - 3) * 60, 150, 1.0f, 1.0f);
	}

	/*******************************************************************************/
	/* Blit effect to framebuffer */
	src = effectBuffer + EFFECT_BUFFER_PADDING;
	dst = fb_pixels;
	for (scanline = 0; scanline < FB_HEIGHT; scanline++) {
		memcpy(dst, src, FB_WIDTH * 2);
		/*
		oldTv(dst, src, 
			rScale(scanline, time_sec), rShift(scanline, time_sec),
			gScale(scanline, time_sec), gShift(scanline, time_sec),
			bScale(scanline, time_sec), bShift(scanline, time_sec));
			*/
		src += EFFECT_BUFFER_W;
		dst += FB_WIDTH;
	}
	swap_buffers(0);
	return;
	/*******************************************************************************/
	
	

	rb_begin(bitmap_props);
	struct rbnode *node = rb_next(bitmap_props);
	while (node) {
		RleBitmap *rle = node->data;
		rleBlitScale(node->data, effectBuffer + EFFECT_BUFFER_PADDING, FB_WIDTH, FB_HEIGHT, EFFECT_BUFFER_W,
			100, 120, 2, 2);
		node = rb_next(bitmap_props);
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

/* Normal map loading & preprocessing.
/* Scale down normalmap scans by depth scale. Maximum scale down around the horizon (e.g. 8x scale down)
 * and minimum (1x) at the bottom (closest to the viewer). Rest of the scan set to black.
 * This way close scans appear smoother. Also unpack R (horizontal) component of the normal.
 */
static int loadAndProcessNormal() {
	int scanline;
	int i;
	int x;
	short maxDisplacement = 0;
	short minDisplacement = 256;
	unsigned short *dst;
	short *dst2;
	unsigned int *normalmap = 0;
	unsigned int *src = 0;
	int normalmapW = 0;
	int normalmapH = 0;

	/* Load normalmap */
	if (!(normalmap =
		  img_load_pixels(NORMAL_FILENAME, &normalmapW, &normalmapH, IMG_FMT_RGBA32))) {
		fprintf(stderr, "failed to load image " NORMAL_FILENAME "\n");
		return 1;
	}

	/* Make sure the dimensions are acceptable */
	if (normalmapW != artW) {
		fprintf(stderr, "Normalmap width is not the same as art width: %d != %d\n", normalmapW, artW);
		return 1;
	}
	if (normalmapH != REFLECTION_HEIGHT) {
		fprintf(stderr, "Normalmap height is not correct: %d !< %d\n", normalmapH, REFLECTION_HEIGHT);
		return 1;
	}


	/* Allocate displacement map */
	displacementMap = (short *)calloc(normalmapW * normalmapH, sizeof(short));
	

	
	/* Set up destination pointers for pass 1 and 2 */
	src = normalmap;
	dst = (unsigned short *)displacementMap;
	dst2 = displacementMap;

	/* Pass 1 - scale normal with depth. */
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		for (i = 0; i < normalmapW; i++) {
			x = (int)(i * scrollScaleTable[scanline] + 0.5f);
			if (x < artW) {
				/* Extract R (horizontal component of the normal) */
				*dst = (unsigned short)(src[x] >> 8) & 0xFF;
				
				/* Find bounds */
				if ((short)*dst > maxDisplacement)
					maxDisplacement = (short)(*dst);
				if ((short)*dst < minDisplacement)
					minDisplacement = (short)(*dst);
			} else {
				*dst = 0;
			}
			dst++;
		}
		src += artW;
	}

	if (maxDisplacement == minDisplacement) {
		printf("Grise normalmap fucked up\n");
		return 1;
	}

	free(normalmap);

	/* Second pass - subtract half maximum displacement to displace in both directions */
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		for (i = 0; i < artW; i++) {
			/* Scale displacement to fit our MAX_DISPLACEMENT requirement. */
			*dst2 = 2 * MAX_DISPLACEMENT * (*dst2 - minDisplacement) /
				    (maxDisplacement - minDisplacement) -
					MAX_DISPLACEMENT;
			
			/* Further scale down displacements with distance. */
			*dst2 = (short)((float)*dst2 / scrollScaleTable[scanline] +
					0.5f);
			dst2++;
		}
	}

	

	return 0;
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
		scrollModTable[i] = (int)(artW / scrollScaleTable[i] + 0.5f);
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
