#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "imago2.h"
#include "demo.h"
#include "screen.h"

#define BG_FILENAME "data/grise.png"

#define MIN_SCROLL 32
#define MAX_SCROLL (backgroundW - fb_width - MIN_SCROLL)

#define FAR_SCROLL_SPEED 50.0f
#define NEAR_SCROLL_SPEED 400.0f

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

static float scrollSpeedTable[REFLECTION_HEIGHT];
static float scrollTable[REFLECTION_HEIGHT];
static int scrollTableRounded[REFLECTION_HEIGHT];
static int scrollModTable[REFLECTION_HEIGHT];

static struct screen scr = {
	"mike",
	init,
	destroy,
	start,
	stop,
	draw
};

struct screen *mike_screen(void)
{
	return &scr;
}


static int init(void)
{
	if (!(background = img_load_pixels(BG_FILENAME, &backgroundW, &backgroundH, IMG_FMT_RGBA32))) {
		fprintf(stderr, "failed to load image " BG_FILENAME "\n");
		return -1;
	}

	/* Convert to 16bpp */
	convert32To16((unsigned int*)background, background, backgroundW * NORMALMAP_SCANLINE); /* Normalmap will keep its 32 bit color */

	processNormal();

	initScrollTables();

	return 0;
}

static void destroy(void)
{
	//img_free_pixels(background);
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
	unsigned short *dst = fb_pixels;
	unsigned short *src = background + scroll;
	int scanline = 0;
	int i = 0;
	short *disp;
	int d;

	lastFrameDuration = (time_msec - lastFrameTime) / 1000.0f;
	lastFrameTime = time_msec;

	for (scanline = 0; scanline < fb_height; scanline++) {
		memcpy(dst, src, fb_width * 2);
		src += backgroundW;
		dst += fb_width;
	}
	
	updateScrollTables(lastFrameDuration);

	dst = (unsigned short*) fb_pixels + HORIZON_HEIGHT * fb_width;
	src = background + HORIZON_HEIGHT * backgroundW;
	disp = displacementMap;
	for (scanline = 0; scanline < REFLECTION_HEIGHT; scanline++) {
		for (i = 0; i < fb_width; i++) {
			d = disp[(i + scrollTableRounded[scanline]) % scrollModTable[scanline]];
			*dst++ = src[i + scroll + d];
		}
		src += backgroundW;
		disp += backgroundW;
	}
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
	float scale;
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
		scale = 2.0f - (float)scanline / ((float)REFLECTION_HEIGHT - 1);
		scrollModTable[scanline] = (int) (backgroundW / scale + 0.5f);
		for (i = 0; i < backgroundW; i++) {
			x = (int)(i * scale + 0.5f);
			if (x < backgroundW) {
				*dst = (unsigned short)normalmap[x] & 0xFF;
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
			*dst2 = 2 * MIN_SCROLL * (*dst2 - minDisplacement) / (maxDisplacement - minDisplacement) - MIN_SCROLL;
			dst2++;
		}
	}
}

static void initScrollTables() {
	int i = 0;
	float scrollSpeed = FAR_SCROLL_SPEED;
	float speedIncrement = (NEAR_SCROLL_SPEED - FAR_SCROLL_SPEED) / ((float) (REFLECTION_HEIGHT - 1));
	for (i = 0; i < REFLECTION_HEIGHT; i++) {
		scrollSpeedTable[i] = scrollSpeed;
		scrollSpeed += speedIncrement;
		scrollTable[i] = 0.0f;
		scrollTableRounded[i] = 0;
	}
}


static void updateScrollTables(float dt) {
	int i = 0;
	for (i = 0; i < REFLECTION_HEIGHT; i++) {
		scrollTable[i] += scrollSpeedTable[i] * dt;
		scrollTableRounded[i] = (int)(scrollTable[i] + 0.5f) % scrollModTable[i];
	}
}
