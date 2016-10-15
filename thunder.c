/* thunder. c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "imago2.h"
#include "demo.h"
#include "screen.h"

/* Render blur in half x half dimenstions. Add one pixel padding in all 
 * directions (2 pixels horizontally, 2 pixels vertically).
 */
#define BLUR_BUFFER_WIDTH (320/2 + 2)
#define BLUR_BUFFER_HEIGHT (240/2 + 2)
#define BLUR_BUFFER_SIZE (BLUR_BUFFER_WIDTH * BLUR_BUFFER_HEIGHT)
static unsigned char *blurBuffer, *blurBuffer2;

/* TODO: Load palette from file */
static unsigned short palette[256];

void clearBlurBuffer();
void drawPatternOnBlurBuffer();
void applyBlur();
void blitEffect();

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static unsigned int lastFrameTime = 0;
static float lastFrameDuration = 0.0f;
static struct screen scr = {
	"thunder",
	init,
	destroy,
	start,
	0,
	draw
};

struct screen *thunder_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i = 0;

	blurBuffer = malloc(BLUR_BUFFER_SIZE);
	blurBuffer2 = malloc(BLUR_BUFFER_SIZE);

	clearBlurBuffer();

	/* For now, map to blue */
	for (i = 0; i < 256; i++) {
		palette[i] = i >> 3;
	}

	return 0;
}

static void destroy(void)
{
	free(blurBuffer);
	blurBuffer = 0;
	
	free(blurBuffer2);
	blurBuffer2 = 0;
}

static void start(long trans_time)
{
	lastFrameTime = time_msec;
}


static void draw(void)
{

	lastFrameDuration = (time_msec - lastFrameTime) / 1000.0f;
	lastFrameTime = time_msec;
	
	drawPatternOnBlurBuffer();
	applyBlur();
	blitEffect();

	swap_buffers(0);
}

void clearBlurBuffer() {
	/* Clear the whole buffer (including its padding ) */
	memset(blurBuffer, 0, BLUR_BUFFER_SIZE);
	memset(blurBuffer2, 0, BLUR_BUFFER_SIZE);
}

void drawPatternOnBlurBuffer() {
	int i, j;
	unsigned char *dst = blurBuffer + BLUR_BUFFER_WIDTH + 1;
	int starty, stopy, startx, stopx;

	starty = rand() % 60;
	stopy = starty + rand() % 60;
	startx = rand() % 80;
	stopx = startx + rand() % 80;

	if (rand() % 2) return;

	for (j = starty; j < stopy; j++) {
		for (i = startx; i < stopx; i++) {
			dst[i + j * BLUR_BUFFER_WIDTH] = 255;
		}
	}
}

void applyBlur() {
	int i, j;
	unsigned char *tmp;
	unsigned char *src = blurBuffer + BLUR_BUFFER_WIDTH + 1;
	unsigned char *dst = blurBuffer2 + BLUR_BUFFER_WIDTH + 1;

	for (j = 0; j < 120; j++) {
		for (i = 0; i < 160; i++) {
			*dst = (*(src - 1) + *(src + 1) + *(src - BLUR_BUFFER_WIDTH) + *(src + BLUR_BUFFER_WIDTH)) >> 2;
			dst++;
			src++;
		}
		/* Just skip the padding since we went through the scanline in the inner loop (except from the padding) */
		src += 2;
		dst += 2;
	}

	/* Swap blur buffers */
	tmp = blurBuffer;
	blurBuffer = blurBuffer2;
	blurBuffer2 = tmp;
}

void blitEffect() {
	unsigned int *dst1 = (unsigned int*) vmem_back;
	unsigned int *dst2 = dst1 + 160; /* We're writing two pixels at once */
	unsigned char *src1 = blurBuffer + BLUR_BUFFER_WIDTH + 1;
	unsigned char *src2 = src1 + BLUR_BUFFER_WIDTH;
	unsigned char tl, tr, bl, br;
	int i, j;

	for (j = 0; j < 120; j++) {
		for (i = 0; i < 160; i++) {
			tl = *src1;
			tr = (*src1 + *(src1 + 1)) >> 1;
			bl = (*src1 + *src2) >> 1;
			br = tr + ((*src2 + *(src2 + 1)) >> 1) >> 1;

			/* Pack 2 pixels in each 32 bit word */
			*dst1 = (palette[tr] << 16) | palette[tl];
			*dst2 = (palette[br] << 16) | palette[bl];

			dst1++;
			src1++;
			dst2++;
			src2++;
		}
		/* Again, skip padding */
		src1 += 2;
		src2 += 2;

		/* For now, skip a scanline */
		dst1 += 160;
		dst2 += 160;
	}

}





