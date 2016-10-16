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

#define BLUR_DARKEN 4

#define THUNDER_RECT_SIZE 2
#define THUNDER_RANDOMNESS 16
#define THUNDER_SECONDS 0.1f

#define VERTEX_COUNT 12
#define PERSPECTIVE_NEUTRAL_DEPTH 0.5f
#define NEAR_PLANE 0.01f
#define ROTATION_SPEED 1.5f

#define PI 3.14159f

/* TODO: Load palette from file */
static unsigned short palette[256];

typedef struct {
	float x,y,z;
} MyVertex ;

MyVertex vertexBuffer[VERTEX_COUNT];
MyVertex vertexBufferAnimated[VERTEX_COUNT];
MyVertex vertexBufferProjected[VERTEX_COUNT];

void clearBlurBuffer();
void applyBlur();
void blitEffect();
void thunder(int x0, int y0, int x1, int y1, int seed, int randomness, int depth);

void initMesh();
void projectMesh();
void animateMesh();
void renderMesh(int seed);

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
		palette[i] = (i * i) >> 11;
	}

	initMesh();

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


static float remainingThunderDuration = THUNDER_SECONDS;
static int thunderPattern = 0;

static void draw(void)
{
	lastFrameDuration = (time_msec - lastFrameTime) / 1000.0f;
	lastFrameTime = time_msec;

	remainingThunderDuration -= lastFrameDuration;
	if (remainingThunderDuration <= 0) {
		thunderPattern++;
		remainingThunderDuration = THUNDER_SECONDS;
	}
	
	animateMesh();
	projectMesh();
	renderMesh(thunderPattern);
	
	
	applyBlur();
	blitEffect();

	

	swap_buffers(0);
}

void clearBlurBuffer() {
	/* Clear the whole buffer (including its padding ) */
	memset(blurBuffer, 0, BLUR_BUFFER_SIZE);
	memset(blurBuffer2, 0, BLUR_BUFFER_SIZE);
}


void applyBlur() {
	int i, j;
	unsigned char *tmp;
	unsigned char *src = blurBuffer + BLUR_BUFFER_WIDTH + 1;
	unsigned char *dst = blurBuffer2 + BLUR_BUFFER_WIDTH + 1;

	for (j = 0; j < 120; j++) {
		for (i = 0; i < 160; i++) {
			*dst = (*(src - 1) + *(src + 1) + *(src - BLUR_BUFFER_WIDTH) + *(src + BLUR_BUFFER_WIDTH)) >> 2;
			
			if (*dst > BLUR_DARKEN) *dst -= BLUR_DARKEN;
			else *dst = 0;

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

void thunder(int x0, int y0, int x1, int y1, int seed, int randomness, int depth) {
	int mx, my, i, j;
	unsigned char *dst;

	if (randomness <= 0) randomness = 1;
	mx = ((x0 + x1) >> 1) + rand() % randomness - randomness / 2;
	my = ((y0 + y1) >> 1) + rand() % randomness - randomness / 2;
	dst = blurBuffer + BLUR_BUFFER_WIDTH + 1 + mx + my * BLUR_BUFFER_WIDTH;

	if (depth <= 0) return;
	if (mx < THUNDER_RECT_SIZE || mx >= 160 - THUNDER_RECT_SIZE || my < THUNDER_RECT_SIZE || my >= 120 - THUNDER_RECT_SIZE) return;

	srand(seed);

	for (j = 0; j < THUNDER_RECT_SIZE; j++) {
		memset(dst, 255, THUNDER_RECT_SIZE);
		dst += BLUR_BUFFER_WIDTH;
	}

	thunder(x0, y0, mx, my, rand(), randomness >> 1, depth-1);
	thunder(mx, my, x1, y1, rand(), randomness >> 1, depth - 1);
}

MyVertex randomVertex() {
	MyVertex ret;
	float l;

	ret.x = rand() % 200 - 100; if (ret.x == 0) ret.x = 1;
	ret.y = rand() % 200 - 100; if (ret.y == 0) ret.y = 1;
	ret.z = rand() % 200 - 100; if (ret.z == 0) ret.z = 1;
	
	// Normalize
	l = sqrt(ret.x * ret.x + ret.y * ret.y + ret.z * ret.z);
	ret.x /= l;
	ret.y /= l;
	ret.z /= l;

	return ret;
}

void initMesh() {
	int i;

	for (i = 0; i < VERTEX_COUNT; i++) {
		vertexBuffer[i] = randomVertex();
	}
}

void animateMesh() {
	int i = 0;
	MyVertex bx, by, bz;
	float yRot;

	yRot = ROTATION_SPEED * time_msec / 1000.0f;

	/* Create rotated basis */
	bx.x = cos(yRot);
	bx.y = 0.0f;
	bx.z = sin(yRot);

	by.x = 0.0f;
	by.y = 1.0f;
	by.z = 0.0f;

	bz.x = cos(yRot + PI/2.0f);
	bz.y = 0.0f;
	bz.z = sin(yRot + PI/2.0f);

	for (i = 0; i < VERTEX_COUNT; i++) {
		MyVertex v1, v2;
		v1 = vertexBuffer[i];

		/* O re panaia mou */
		v2.x = v1.x * bx.x + v1.y * by.x + v1.z * bz.x;
		v2.y = v1.x * bx.y + v1.y * by.y + v1.z * bz.y;
		v2.z = v1.x * bx.z + v1.y * by.z + v1.z * bz.z;
		

		v2.z += 1.00f;

		vertexBufferAnimated[i] = v2;
	}
}

void projectMesh() {
	int i = 0;

	for (i = 0; i < VERTEX_COUNT; i++) {

		if (vertexBufferAnimated[i].z <= NEAR_PLANE) {
			vertexBufferProjected[i].x = vertexBufferProjected[i].y = 1000.0f;
			vertexBufferProjected[i].z = -1.0f;
			continue;
		}

		vertexBufferProjected[i].x = vertexBufferAnimated[i].x * PERSPECTIVE_NEUTRAL_DEPTH / vertexBufferAnimated[i].z;
		vertexBufferProjected[i].y = vertexBufferAnimated[i].y * PERSPECTIVE_NEUTRAL_DEPTH / vertexBufferAnimated[i].z;
	}
}

void renderMesh(int seed) {
	int vertex, j;
	int sx, sy;
	unsigned char *dst;

	srand(seed);

	for (vertex = 0; vertex < VERTEX_COUNT; vertex++) {
		sx = (int)(vertexBufferProjected[vertex].x * 80) + 80;
		sy = (int)(vertexBufferProjected[vertex].y * 60) + 60;

		/*if (sx < THUNDER_RECT_SIZE || sx >= 160 - THUNDER_RECT_SIZE || sy < THUNDER_RECT_SIZE || sy >= 120 - THUNDER_RECT_SIZE) return;*/

		thunder(80, 60, sx, sy, rand(), THUNDER_RANDOMNESS, 5);		
	}
}