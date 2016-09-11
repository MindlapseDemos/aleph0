#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "imago2.h"
#include "demo.h"
#include "screen.h"

/* APPROX. 170 FPS Minimum */

typedef struct {
	unsigned int w, h;
	unsigned char *scans;
} RLEBitmap;

static RLEBitmap rleCreate(unsigned int w, unsigned int h);
static void rleDestroy(RLEBitmap b);
static void rleBlit(unsigned short *dst, int dstW, int dstH, int dstStride, 
	RLEBitmap bitmap, int blitX, int blitY);
static void rleBlitScale(unsigned short *dst, int dstW, int dstH, int dstStride,
	RLEBitmap bitmap, int blitX, int blitY, float scaleX, float scaleY);
static void rleBlitScaleInv(unsigned short *dst, int dstW, int dstH, int dstStride,
	RLEBitmap bitmap, int blitX, int blitY, float scaleX, float scaleY);
static RLEBitmap rleEncode(unsigned char *pixels, unsigned int w, unsigned int h);

#define BG_FILENAME "data/grise.png"
#define GROBJ_01_FILENAME "data/grobj_01.png"

#define BB_SIZE 512	/* Let's use a power of 2. Maybe we'll zoom/rotate the effect */

/* Every backBuffer scanline is guaranteed to have that many dummy pixels before and after */
#define PIXEL_PADDING 32

/* Make sure this is less than PIXEL_PADDING*/
#define MAX_DISPLACEMENT 16 

#define MIN_SCROLL PIXEL_PADDING
#define MAX_SCROLL (backgroundW - fb_width - MIN_SCROLL)

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
static unsigned int backgroundW = 0;
static unsigned int backgroundH = 0;

static unsigned int lastFrameTime = 0;
static float lastFrameDuration = 0.0f;

static short *displacementMap;

static unsigned short *backBuffer;

static float scrollScaleTable[REFLECTION_HEIGHT];
static float scrollTable[REFLECTION_HEIGHT];
static int scrollTableRounded[REFLECTION_HEIGHT];
static int scrollModTable[REFLECTION_HEIGHT];
static float nearScrollAmount = 0.0f;

static RLEBitmap grobj;

static struct screen scr = {
	"galaxyrise",
	init,
	destroy,
	start,
	stop,
	draw
};

struct screen *grise_screen(void)
{
	return &scr;
}


static int init(void)
{
	unsigned char *tmpBitmap;
	int tmpBitmapW, tmpBitmapH;

	/* Allocate back buffer */
	backBuffer = (unsigned short*) malloc(BB_SIZE * BB_SIZE * sizeof(unsigned short));

	/* grise.png contains the background (horizon), baked reflection and normalmap for displacement */
	if (!(background = img_load_pixels(BG_FILENAME, &backgroundW, &backgroundH, IMG_FMT_RGBA32))) {
		fprintf(stderr, "failed to load image " BG_FILENAME "\n");
		return -1;
	}

	/* Convert to 16bpp */
	convert32To16((unsigned int*)background, background, backgroundW * NORMALMAP_SCANLINE); /* Normalmap will keep its 32 bit color */

	/* Load reflected objects */
	if (!(tmpBitmap = img_load_pixels(GROBJ_01_FILENAME, &tmpBitmapW, &tmpBitmapH, IMG_FMT_GREY8))) {
		fprintf(stderr, "failed to load image " GROBJ_01_FILENAME "\n");
		return -1;
	}

	grobj = rleEncode(tmpBitmap, tmpBitmapW, tmpBitmapH);

	img_free_pixels(tmpBitmap);

	initScrollTables();

	processNormal();

#ifdef MIKE_PC
	return 0xCAFE;
#else
	return 0;
#endif
}

static void destroy(void)
{
	free(backBuffer);
	backBuffer = 0;

	img_free_pixels(background);

	rleDestroy(grobj);
}

static void start(long trans_time)
{
	lastFrameTime = time_msec;
}

static void stop(long trans_time)
{
}

static void draw(void)
{	
	int scroll = MIN_SCROLL + (MAX_SCROLL - MIN_SCROLL) * mouse_x / fb_width;
	unsigned short *dst = backBuffer + PIXEL_PADDING;
	unsigned short *src = background + scroll;
	int scanline = 0;
	int i = 0;
	short *dispScanline;
	int d;

	lastFrameDuration = (time_msec - lastFrameTime) / 1000.0f;
	lastFrameTime = time_msec;

	/* First, render the horizon */
	for (scanline = 0; scanline < HORIZON_HEIGHT; scanline++) {
		memcpy(dst, src, fb_width * 2);
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
		memcpy(dst, src, (fb_width + PIXEL_PADDING) * 2);
		src += backgroundW;
		dst += BB_SIZE;
	}

	/* Blit reflections first, to be  displaced */
	for (i = 0; i < 5; i++) rleBlitScaleInv(backBuffer + PIXEL_PADDING, fb_width, fb_height, BB_SIZE, grobj, 134 + (i-3) * 60, 235, 1.0f, 1.8f);

	/* Perform displacement */
	dst = backBuffer + HORIZON_HEIGHT * BB_SIZE + PIXEL_PADDING;
	src = dst + BB_SIZE; /* The pixels to be displaced are 1 scanline below */
	dispScanline = displacementMap;
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		for (i = 0; i < fb_width; i++) {
			d = dispScanline[(i + scrollTableRounded[scanline]) % scrollModTable[scanline]];
			*dst++ = src[i + d];
		}
		src += backgroundW;
		dst += BB_SIZE - fb_width;
		dispScanline += backgroundW;
	}

	/* Then after displacement, blit the objects */
	for (i = 0; i < 5; i++) rleBlit(backBuffer + PIXEL_PADDING, fb_width, fb_height, BB_SIZE, grobj, 134 + (i-3) * 60, 100);

	/* Blit effect to framebuffer */
	src = backBuffer + PIXEL_PADDING;
	dst = fb_pixels;
	for (scanline = 0; scanline < fb_height; scanline++) {
		memcpy(dst, src, fb_width * 2);
		src += BB_SIZE; 
		dst += fb_width;
	}

	swap_buffers(fb_pixels);
}

/* src and dst can be the same */
static void convert32To16(unsigned int *src32, unsigned short *dst16, unsigned int pixelCount) {
	unsigned int p;
	while (pixelCount) {
		p = *src32++;
		*dst16++ =	((p << 8) & 0xF800)		/* R */
			|		((p >> 5) & 0x07E0)		/* G */
			|		((p >> 19) & 0x001F);	/* B */
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
	unsigned int *normalmap = (unsigned int*)background;
	normalmap += NORMALMAP_SCANLINE * backgroundW;
	dst = (unsigned short*)normalmap;
	displacementMap = (short*)dst;
	dst2 = displacementMap;

	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		scrollModTable[scanline] = (int) (backgroundW / scrollScaleTable[scanline] + 0.5f);
		for (i = 0; i < backgroundW; i++) {
			x = (int)(i * scrollScaleTable[scanline] + 0.5f);
			if (x < backgroundW) {
				*dst = (unsigned short)(normalmap[x] >> 8) & 0xFF;
				if ((short)*dst > maxDisplacement) maxDisplacement = (short)(*dst);
				if ((short)*dst < minDisplacement) minDisplacement = (short)(*dst);
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
			/* Remember that MIN_SCROLL is the padding around the screen, so ti's the maximum displacement we can get (positive & negative) */
			*dst2 = 2 * MAX_DISPLACEMENT * (*dst2 - minDisplacement) / (maxDisplacement - minDisplacement) - MAX_DISPLACEMENT;
			*dst2 = (short)((float)*dst2 / scrollScaleTable[scanline] + 0.5f); /* Displacements must also scale with distance*/
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
	nearScrollAmount = (float) fmod(nearScrollAmount, 512.0f);

	for (i = 0; i < REFLECTION_HEIGHT; i++) {
		scrollTable[i] = nearScrollAmount / scrollScaleTable[i];
		scrollTableRounded[i] = (int)(scrollTable[i] + 0.5f) % scrollModTable[i];
	}
}

/* -------------------------------------------------------------------------------------------------
 *                                   RLE STUFF                                                                           
 * -------------------------------------------------------------------------------------------------
 */
/* Limit streak count per scanline so we can directly jump to specific scanline */
#define RLE_STREAKS_PER_SCANLINE 4
/* Every streak is encoded by 2 bytes: offset and count of black pixels in the streak */
#define RLE_BYTES_PER_SCANLINE RLE_STREAKS_PER_SCANLINE * 2
#define RLE_FILL_COLOR 0
#define RLE_FILL_COLOR_32 ((RLE_FILL_COLOR << 16) | RLE_FILL_COLOR)

static RLEBitmap rleCreate(unsigned int w, unsigned int h) {
	RLEBitmap ret;
	ret.w = w;
	ret.h = h;

	/* Add some padding at the end of the buffer, with the worst case for a scanline (w/2 streaks) */
	ret.scans = (unsigned char*) calloc(h * RLE_BYTES_PER_SCANLINE + w, 1);

	return ret;
}

static void rleDestroy(RLEBitmap b) {
	free(b.scans);
}

static RLEBitmap rleEncode(unsigned char *pixels, unsigned int w, unsigned int h) {
	int scanline;
	int i;
	int penActive = 0;
	int counter = 0;
	int accum = 0;
	RLEBitmap ret;
	unsigned char *output;

	/* https://www.youtube.com/watch?v=RKMR02o1I88&feature=youtu.be&t=55 */
	ret = rleCreate(w, h);

	for (scanline = 0; scanline < h; scanline++) {
		output = ret.scans + scanline * RLE_BYTES_PER_SCANLINE;
		accum = 0;
		for (i = 0; i < w; i++) {
			if (*pixels++) {
				if (penActive) {
					if (counter >= PIXEL_PADDING) {
						*output++ = (unsigned char) counter;
						counter = 0;
						*output++ = (unsigned char)accum;
					}
					counter++;
					accum++;
				} else {
					*output++ = (unsigned char)accum;
					counter = 1;
					accum++;
					penActive = 1;
				}
			} else {
				if (penActive) {
					*output++ = (unsigned char)counter;
					counter = 1;
					accum++;
					penActive = 0;
				} else {
					counter++;
					accum++;
				}
			}
		}

		if (penActive) {
			*output++ = (unsigned char)counter;
		}
		penActive = 0;
		counter = 0;
	}

	return ret;
}

static void rleBlit(unsigned short *dst, int dstW, int dstH, int dstStride,
	RLEBitmap bitmap, int blitX, int blitY) 
{
	int scanline = 0;
	int streakPos = 0;
	int streakLength = 0;
	int streak = 0;
	unsigned char *input = bitmap.scans;
	unsigned short *output;
	unsigned int *output32;

	dst += blitX + blitY * dstStride;

	for (scanline = blitY; scanline < blitY + bitmap.h; scanline++) {
		if (scanline < 0 || scanline >= dstH) continue;
		for (streak = 0; streak < RLE_STREAKS_PER_SCANLINE; streak++) {
			streakPos = *input++;
			streakLength = *input++;

			if ((streakPos + blitX) <= 0) continue;

			output = dst + streakPos;

			/* Check if we need to write the first pixel as 16bit */
			if (streakLength % 2) {
				*output++ = RLE_FILL_COLOR;
			}

			/* Then, write 2 pixels at a time */
			streakLength >>= 1;
			output32 = (unsigned int*) output;
			while (streakLength--) {
				*output32++ = RLE_FILL_COLOR_32;
			}
		}

		dst += dstStride;
	}
}

static void interpolateScan(unsigned char *output, unsigned char *a, unsigned char *b, float t) {
	static int div = 1 << 23;
	int ti, i;

	t += 1.0f;
	ti = (*((unsigned int*)&t)) & 0x7FFFFF;
	
	for (i = 0; i < RLE_BYTES_PER_SCANLINE; i++) {
		*output++ = ((*b++ * ti) + (*a++ * (div - ti))) >> 23;
	}
}

static void rleBlitScale(unsigned short *dst, int dstW, int dstH, int dstStride,
	RLEBitmap bitmap, int blitX, int blitY, float scaleX, float scaleY)
{
	int scanline = 0;
	int streakPos = 0;
	int streakLength = 0;
	int streak = 0;
	unsigned short *output;
	unsigned int *output32;
	unsigned char *input;
	int scanlineCounter = 0;
	static unsigned char scan[512];

	int blitW = (int) (bitmap.w * scaleX + 0.5f);
	int blitH = (int)(bitmap.h * scaleY + 0.5f);
	
	dst += blitX + blitY * dstStride;

	for (scanline = blitY; scanline < blitY + blitH; scanline++) {
		float normalScan = scanlineCounter / scaleY;
		unsigned char *scan0 = bitmap.scans + RLE_BYTES_PER_SCANLINE * (int)normalScan;
		unsigned char *scan1 = scan0 + RLE_BYTES_PER_SCANLINE;
		normalScan -= (int)normalScan;
		interpolateScan(scan, scan0, scan1, normalScan);
		input = scan;
		scanlineCounter++;

		if (scanline < 0 || scanline >= dstH) continue;
		for (streak = 0; streak < RLE_STREAKS_PER_SCANLINE; streak++) {
			streakPos = (int) ((*input++) * scaleX + 0.5f);
			streakLength = (int)((*input++) * scaleX + 0.5f);

			if ((streakPos + blitX) <= 0) continue;

			output = dst + streakPos;

			/* Check if we need to write the first pixel as 16bit */
			if (streakLength % 2) {
				*output++ = RLE_FILL_COLOR;
			}

			/* Then, write 2 pixels at a time */
			streakLength >>= 1;
			output32 = (unsigned int*)output;
			while (streakLength--) {
				*output32++ = RLE_FILL_COLOR_32;
			}
		}

		dst += dstStride;
	}
}

static void rleBlitScaleInv(unsigned short *dst, int dstW, int dstH, int dstStride,
	RLEBitmap bitmap, int blitX, int blitY, float scaleX, float scaleY)
{
	int scanline = 0;
	int streakPos = 0;
	int streakLength = 0;
	int streak = 0;
	unsigned short *output;
	unsigned int *output32;
	unsigned char *input;
	int scanlineCounter = 0;
	static unsigned char scan[512];

	int blitW = (int)(bitmap.w * scaleX + 0.5f);
	int blitH = (int)(bitmap.h * scaleY + 0.5f);

	dst += blitX + blitY * dstStride;

	for (scanline = blitY; scanline > blitY - blitH; scanline--) {
		float normalScan = scanlineCounter / scaleY;
		unsigned char *scan0 = bitmap.scans + RLE_BYTES_PER_SCANLINE * (int)normalScan;
		unsigned char *scan1 = scan0 + RLE_BYTES_PER_SCANLINE;
		normalScan -= (int)normalScan;
		interpolateScan(scan, scan0, scan1, normalScan);
		input = scan;
		scanlineCounter++;

		if (scanline < 0 || scanline >= dstH) continue;
		for (streak = 0; streak < RLE_STREAKS_PER_SCANLINE; streak++) {
			streakPos = (int)((*input++) * scaleX + 0.5f);
			streakLength = (int)((*input++) * scaleX + 0.5f);

			if ((streakPos + blitX) <= 0) continue;

			output = dst + streakPos;

			/* Check if we need to write the first pixel as 16bit */
			if (streakLength % 2) {
				*output++ = RLE_FILL_COLOR;
			}

			/* Then, write 2 pixels at a time */
			streakLength >>= 1;
			output32 = (unsigned int*)output;
			while (streakLength--) {
				*output32++ = RLE_FILL_COLOR_32;
			}
		}

		dst -= dstStride;
	}
}