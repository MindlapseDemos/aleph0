#include "demo.h"
#include "imago2.h"
#include "screen.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rlebmap.h"

/* APPROX. 170 FPS Minimum */

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static void updatePropeller(float t, RleBitmap *rle);

static unsigned short *backBuffer;

static unsigned char miniFXBuffer[1024];

static long lastFrameTime = 0;

static struct screen scr = {"minifx", init, destroy, start, 0, draw};

struct screen *minifx_screen(void) {
	return &scr;
}

static int init(void) {
	/* Allocate back buffer */
	backBuffer = calloc(FB_WIDTH * FB_HEIGHT, sizeof(unsigned short));

	return 0;
}

static void destroy(void) {
	free(backBuffer);
	backBuffer = 0;
}

static void start(long trans_time) { lastFrameTime = time_msec; }

static void draw(void) {
	/*long lastFrameDuration;*/
	int i, stride;
	RleBitmap *rle;
	int clearColor;
	unsigned short clearColor16;

	/*lastFrameDuration = (time_msec - lastFrameTime) / 1000.0f;*/
	lastFrameTime = time_msec;

	clearColor = 0x888888;
	clearColor16 = ((clearColor << 8) & 0xF800)	/* R */
			   | ((clearColor >> 5) & 0x07E0)   /* G */
			   | ((clearColor >> 19) & 0x001F); /* B */

	for (i=0; i<FB_WIDTH * FB_HEIGHT; i++) {
		backBuffer[i] = clearColor16;
	}

	/* For now create / destroy in each frame. We will manage these later */
	rle = rleCreate(32, 32);

	updatePropeller(time_msec / 1000.0f, rle);
	stride = FB_WIDTH;
	/*
	rleBlit(rle, backBuffer, FB_WIDTH, FB_HEIGHT, stride,
			100, 100);
	*/

	rleBlitScale(rle, backBuffer, FB_WIDTH, FB_HEIGHT, stride, 50,
		  50, 3.0, 3.0);

	rleDestroy(rle);

	/* Blit effect to framebuffer */
	memcpy(fb_pixels, backBuffer, FB_WIDTH * FB_HEIGHT * sizeof(unsigned short));
	swap_buffers(0);
}


#define PROPELLER_CIRCLE_RADIUS 18
#define PROPELLER_CIRCLE_RADIUS_SQ (PROPELLER_CIRCLE_RADIUS * PROPELLER_CIRCLE_RADIUS)

static struct {
	int circleX[3];
	int circleY[3];
} propellerState;

static void updatePropeller(float t, RleBitmap *rle) {

	int i, j;
	int cx, cy, count = 0;
	unsigned char *dst;
	float x = 0.0f;
	float y = 18.0f;
	float nx, ny;
	float cost, sint;
	static float sin120 = 0.86602540378f;
	static float cos120 = -0.5f;

	t *= 0.1; /* Slow-mo to see what happens */

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
	rleEncode(rle, miniFXBuffer, 32, 32);

	/* Distribute the produced streaks so that they don't produce garbage when interpolated */
	rleDistributeStreaks(rle);
}
