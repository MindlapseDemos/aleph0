/* Blobgrid effect idea */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"

#include "opt_rend.h"


typedef struct BlobGridParams
{
	int effectIndex;
	int numPoints;
	int blobSizesNum;
	int connectionDist;
	int connectionBreaks;
} BlobGridParams;

#define FP_PT 16

//#define STARS_NORMAL

#define MAX_NUM_POINTS 256
#define NUM_STARS MAX_NUM_POINTS
#define STARS_CUBE_LENGTH 1024
#define STARS_CUBE_DEPTH 512

BlobGridParams bgParams0 = { 0, 64, 10, 8192, 32};
BlobGridParams bgParams1 = { 1, 80, 8, 4096, 32};
BlobGridParams bgParamsStars = { 2, NUM_STARS, 7, 4096, 16};


typedef struct Pos3D
{
	int x,y,z;
} Pos3D;

static Pos3D *origPos;
static Pos3D *screenPos;

static unsigned long startingTime;

static unsigned short *blobsPal;
static unsigned int *blobsPal32;
static unsigned char *blobBuffer;


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"blobgrid",
	init,
	destroy,
	start,
	0,
	draw
};

struct screen *blobgrid_screen(void)
{
	return &scr;
}

static void initStars(void)
{
	int i;
	Pos3D *dst = origPos;

	for (i=0; i<NUM_STARS; ++i) {
		dst->x = (rand() & (STARS_CUBE_LENGTH - 1)) - STARS_CUBE_LENGTH / 2;
		dst->y = (rand() & (STARS_CUBE_LENGTH - 1)) - STARS_CUBE_LENGTH / 2;
		dst->z = rand() & (STARS_CUBE_DEPTH - 1);
		++dst;
	}
}

static int init(void)
{
	int i,j,k;

	origPos = (Pos3D*)malloc(sizeof(Pos3D) * MAX_NUM_POINTS);
	screenPos = (Pos3D*)malloc(sizeof(Pos3D) * MAX_NUM_POINTS);

	blobBuffer = (unsigned char*)malloc(FB_WIDTH * FB_HEIGHT);
	blobsPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);

	for (i=0; i<128; i++) {
		int r = i >> 2;
		int g = i >> 1;
		int b = i >> 1;
		if (b > 31) b = 31;
		blobsPal[i] = (r<<11) | (g<<5) | b;
	}
	for (i=128; i<256; i++) {
		int r = 31 - ((i-128) >> 4);
		int g = 63 - ((i-128) >> 2);
		int b = 31 - ((i-128) >> 3);
		blobsPal[i] = (r<<11) | (g<<5) | b;
	}

	blobsPal32 = createColMap16to32(blobsPal);

	initBlobGfx();

	initStars();

	return 0;
}

static void destroy(void)
{
	free(blobsPal);
	free(blobsPal32);
	free(origPos);
	free(screenPos);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void drawConnections(BlobGridParams *params)
{
	int i,j,k;
	const int numPoints = params->numPoints;
	const int connectionDist = params->connectionDist;
	const int connectionBreaks = params->connectionBreaks;
	const int blobSizesNum = params->blobSizesNum;

	const int bp = connectionDist / connectionBreaks;

	for (j=0; j<numPoints; ++j) {
		const int xpj = screenPos[j].x;
		const int ypj = screenPos[j].y;
		for (i=j+1; i<numPoints; ++i) {
			const int xpi = screenPos[i].x;
			const int ypi = screenPos[i].y;

			if (i!=j) {
				const int lx = xpi - xpj;
				const int ly = ypi - ypj;
				const int dst = lx*lx + ly*ly;

				if (dst >= bp && dst < connectionDist) {
					const int steps = dst / bp;
					int px = xpi << FP_PT;
					int py = ypi << FP_PT;
					const int dx = ((xpj - xpi) << FP_PT) / steps;
					const int dy = ((ypj - ypi) << FP_PT) / steps;

					for (k=0; k<steps-1; ++k) {
						int iii = k >> 1;
						if (iii < 0) iii = 0; if (iii > blobSizesNum-1) iii = blobSizesNum-1;
						px += dx;
						py += dy;
						drawBlob(px>>FP_PT, py>>FP_PT,blobSizesNum - 1 - iii, 0, blobBuffer);
					}
				}
			}
		}
	}
}

static void drawConnections3D(BlobGridParams *params)
{
	int i,j,k;
	const int numPoints = params->numPoints;
	const int connectionDist = params->connectionDist;
	const int connectionBreaks = params->connectionBreaks;
	const int blobSizesNum = params->blobSizesNum;

	const int bp = connectionDist / connectionBreaks;

	for (j=0; j<numPoints; ++j) {
		const int xpj = origPos[j].x;
		const int ypj = origPos[j].y;
		const int zpj = origPos[j].z;
		for (i=j+1; i<numPoints; ++i) {
			const int xpi = origPos[i].x;
			const int ypi = origPos[i].y;
			const int zpi = origPos[i].z;

			if (i!=j || (zpi > 0 && zpj > 0)) {
				const int lx = xpi - xpj;
				const int ly = ypi - ypj;
				const int lz = zpi - zpj;
				const int dst = lx*lx + ly*ly + lz*lz;

				if (bp > 0 && dst < connectionDist) {
					const int xsi = screenPos[i].x;
					const int ysi = screenPos[i].y;
					const int xsj = screenPos[j].x;
					const int ysj = screenPos[j].y;

					//int steps = (((xsi - xsj)*(xsi - xsj) + (ysi - ysj)*(ysi - ysj)) / bp);
					int length = sqrt((xsi - xsj)*(xsi - xsj) + (ysi - ysj)*(ysi - ysj));

					int px = xsi << FP_PT;
					int py = ysi << FP_PT;
					//const int dx = ((xsj - xsi) << FP_PT) / steps;
					//const int dy = ((ysj - ysi) << FP_PT) / steps;

					int dx, dy, dl, ti;

					//length >>= 1;
					if (length<=1) continue;	// ==0 crashes, possibly overflow from sqrt giving a NaN?
					dl = (1 << FP_PT) / length;
					dx = (xsj - xsi) * dl;
					dy = (ysj - ysi) * dl;

					ti = 0;
					for (k=0; k<length-1; ++k) {
						int size = ((blobSizesNum-1) * 1 * ti) >> FP_PT;
						//if (size > blobSizesNum-1) size = blobSizesNum-1 - size;
						if (size < 1) size = 1;
						//if (size > blobSizesNum-1) size = blobSizesNum-1;
						px += dx;
						py += dy;
						ti += dl;
						drawBlob(px>>FP_PT, py>>FP_PT, size, 0, blobBuffer);
					}
				}
			}
		}
	}
}

static void drawStars(BlobGridParams *params)
{
	const int blobSizesNum = params->blobSizesNum;

	int count = params->numPoints;
	Pos3D *src = origPos;
	Pos3D *dst = screenPos;

	do {
		const int x = src->x;
		const int y = src->y;
		const int z = src->z;

		dst->z = z;
		if (z > 0) {
			const int xp = (FB_WIDTH / 2) + (x << 6) / z;
			const int yp = (FB_HEIGHT / 2) + (y << 6) / z;

			int size = ((blobSizesNum - 1) * (STARS_CUBE_DEPTH - z)) / STARS_CUBE_DEPTH;
			int shifter = (4 * (STARS_CUBE_DEPTH - z)) / STARS_CUBE_DEPTH;

			drawBlob(xp,yp,size,shifter,blobBuffer);

			dst->x = xp;
			dst->y = yp;
		}
		++src;
		++dst;
	}while(--count != 0);
}

static void drawPoints(BlobGridParams *params, int t)
{
	const int numPoints = params->numPoints;
	const int blobSizesNum = params->blobSizesNum;
	Pos3D *dst = screenPos;

	int count = numPoints;
	switch(params->effectIndex) {
		case 0:
			do {
				dst->x = FB_WIDTH / 2 + (int)(sin((t + 478*count)/2924.0) * 148);
				dst->y = FB_HEIGHT / 2 + (int)(sin((t + 524*count)/2638.0) * 96);
				drawBlob(dst->x,dst->y,blobSizesNum>>1,2,blobBuffer);
				++dst;
			} while(--count != 0);
			drawConnections(params);
		break;

		case 1:
			do {
				dst->x = FB_WIDTH / 2 + (int)(sin((t/2 + 768*count)/2624.0) * 148);
				dst->y = FB_HEIGHT / 2 + (int)(sin((t/2 + 624*count)/1238.0) * 96);
				drawBlob(dst->x,dst->y,blobSizesNum>>1,3,blobBuffer);
				++dst;
			} while(--count != 0);
			drawConnections(params);
		break;
		
		case 2:
			drawStars(params);
			drawConnections3D(params);
		break;
	}
}


static void moveStars(int t)
{
	int count = NUM_STARS;
	Pos3D *src = origPos;

	do {
		#ifndef STARS_NORMAL
			src->x += (((count) & 7) + 1);
			src->y += (((count) & 3) + 2);
			if (src->x >= STARS_CUBE_DEPTH / 2) src->x = - STARS_CUBE_DEPTH / 2;
			if (src->y >= STARS_CUBE_DEPTH / 2) src->y = - STARS_CUBE_DEPTH / 2;
		#endif			

		src->z = (src->z - ((count & 3) + 1)) & (STARS_CUBE_DEPTH - 1);
		++src;
	}while(--count != 0);
}

static void drawEffect(BlobGridParams *params, int t)
{
	static int prevT = 0;
	if (t - prevT > 20) {
		moveStars(t);
		prevT = t;
	}

	drawPoints(params, t);
}

static void draw(void)
{
	int i,y=0;
	const int t = time_msec - startingTime;

	memset(blobBuffer, 0, FB_WIDTH * FB_HEIGHT);

	//drawEffect(&bgParams1, t);
	drawEffect(&bgParamsStars, t);

	buffer8bppToVram(blobBuffer, blobsPal32);

	swap_buffers(0);
}
