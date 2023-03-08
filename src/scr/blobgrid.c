/* Blobgrid effect idea */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"

typedef struct BlobGridParams
{
	int effectIndex;
	int numPoints;
	int blobSizesNum;
	int connectionDist;
	int connectionBreaks;
} BlobGridParams;

#define FP_PT 8

#define BLOB_SIZES_NUM_MAX 16
#define BLOB_SIZEX_PAD 4
#define MAX_BLOB_COLOR 15
#define BLOB_SHIFT_MAX 2
#define MAX_NUM_POINTS 256

#define NUM_STARS (MAX_NUM_POINTS / 2)
#define STARS_CUBE_LENGTH 256
#define STARS_CUBE_DEPTH 1024

#define CLAMP01(v) if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f;

BlobGridParams bgParams0 = { 0, 64, 10, 8192, 32};
BlobGridParams bgParams1 = { 1, 80, 8, 4096, 32};
BlobGridParams bgParamsStars = { 2, NUM_STARS, 8, 4096, 64};


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

typedef struct BlobData
{
	int sizeX, sizeY;
	unsigned char *data;
} BlobData;

BlobData blobData[BLOB_SIZES_NUM_MAX][BLOB_SIZEX_PAD];


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

static void initBlobGfx()
{
	int i,j,x,y;
	float r;

	for (i=0; i<BLOB_SIZES_NUM_MAX; ++i) {
		const int blobSizeY = i+3;	// 3 to 15
		const int blobSizeX = (((blobSizeY+3) >> 2) << 2) + BLOB_SIZEX_PAD;    // We are padding this, as we generate four pixels offset (for dword rendering)
		const float blobSizeYhalf = blobSizeY / 2.0f;
		const float blobSizeXhalf = blobSizeX / 2.0f;

		for (j=0; j<BLOB_SIZEX_PAD; ++j) {
			unsigned char *dst;

			blobData[i][j].sizeX = blobSizeX;
			blobData[i][j].sizeY = blobSizeY;

			blobData[i][j].data = (unsigned char*)malloc(blobSizeX * blobSizeY);
			dst = blobData[i][j].data;

			for (y=0; y<blobSizeY; ++y) {
				const float yc = (float)y - blobSizeYhalf + 0.5f;
				float yci = fabs(yc / (blobSizeYhalf - 0.5f));

				for (x=0; x<blobSizeX; ++x) {
					const int xc = (float)(x - j) - blobSizeXhalf + 0.5f;
					float xci = fabs(xc / (blobSizeYhalf - 0.5f));

					r = 1.0f - (xci*xci + yci*yci);
					CLAMP01(r)
					*dst++ = (int)(pow(r, 2.0f) * MAX_BLOB_COLOR);
				}
			}
		}
	}
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

	blobsPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);
	blobsPal32 = (unsigned int*)malloc(sizeof(unsigned int) * 256 * 256);

	origPos = (Pos3D*)malloc(sizeof(Pos3D) * MAX_NUM_POINTS);
	screenPos = (Pos3D*)malloc(sizeof(Pos3D) * MAX_NUM_POINTS);

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

	k = 0;
	for (j=0; j<256; ++j) {
		const int c0 = blobsPal[j];
		for (i=0; i<256; ++i) {
			const int c1 = blobsPal[i];
			blobsPal32[k++] = (c0 << 16) | c1;
		}
	}

	blobBuffer = (unsigned char*)malloc(FB_WIDTH * FB_HEIGHT);

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

static void drawBlob(int posX, int posY, int size, int shift)
{
	int x,y;
	
	BlobData *bd = &blobData[size][0];
	const int sizeX = bd->sizeX;
	const int sizeY = bd->sizeY;

	unsigned char *dst = (unsigned char*)blobBuffer + (posY - sizeY / 2) * FB_WIDTH + posX - sizeX / 2;

	unsigned char *src = bd->data;

	for (y=0; y<sizeY; ++y) {
		for (x=0; x<sizeX; ++x) {
			int c = *(dst + x) + *(src + x);
			if (c > 255) c = 255;
			*(dst + x) = c;
		}
		src += sizeX;
		dst += FB_WIDTH;
	}
}

static void drawBlob32(int posX, int posY, int size, int shift)
{
	int x,y;
	const int posX32 = posX & ~(BLOB_SIZEX_PAD-1);
	
	BlobData *bd = &blobData[size][posX & (BLOB_SIZEX_PAD-1)];
	const int sizeX = bd->sizeX;
	const int sizeY = bd->sizeY;

	unsigned int *dst = (unsigned int*)(blobBuffer + (posY - sizeY / 2) * FB_WIDTH + (posX32 - sizeX / 2));

	const int wordsX = sizeX / 4;
	unsigned int *src = (unsigned int*)bd->data;

	if (posX <= sizeX / 2 || posX >= FB_WIDTH - sizeX / 2 || posY <= sizeY / 2 || posY >= FB_HEIGHT - sizeY / 2) return;

	for (y=0; y<sizeY; ++y) {
		for (x=0; x<wordsX; ++x) {
			unsigned int c = *(dst+x) + (*(src+x) << shift);
			*(dst+x) = c;
		}
		src += wordsX;
		dst += FB_WIDTH / 4;
	}
}

static void blobBufferToVram32()
{
	int i;
	unsigned short *src = (unsigned short*)blobBuffer;
	unsigned int *dst = (unsigned int*)fb_pixels;

	for (i=0; i<FB_WIDTH * FB_HEIGHT / 2; ++i) {
		*dst++ = blobsPal32[*src++];
	}
}

static void blobBufferToVram()
{
	int i;
	unsigned char *src = blobBuffer;
	unsigned short *dst = (unsigned short*)fb_pixels;

	for (i=0; i<FB_WIDTH * FB_HEIGHT; ++i) {
		*dst++ = blobsPal[*src++];
	}
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
						drawBlob32(px>>FP_PT, py>>FP_PT,blobSizesNum - 1 - iii, 0);
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

				if (dst >= bp && dst < connectionDist) {
					const int steps = dst / bp;
					const int xsi = screenPos[i].x;
					const int ysi = screenPos[i].y;
					const int xsj = screenPos[j].x;
					const int ysj = screenPos[j].y;
					int px = xsi << FP_PT;
					int py = ysi << FP_PT;
					const int dx = ((xsj - xsi) << FP_PT) / steps;
					const int dy = ((ysj - ysi) << FP_PT) / steps;

					for (k=0; k<steps-1; ++k) {
						int iii = k >> 1;
						if (iii < 0) iii = 0; if (iii > blobSizesNum-1) iii = blobSizesNum-1;
						px += dx;
						py += dy;
						drawBlob32(px>>FP_PT, py>>FP_PT, blobSizesNum - 1 - iii, 0);
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
			const int xp = (FB_WIDTH / 2) + (x << 8) / z;
			const int yp = (FB_HEIGHT / 2) + (y << 8) / z;
			
			int k;
			drawBlob32(xp,yp,blobSizesNum>>1,2);

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
				drawBlob32(dst->x,dst->y,blobSizesNum>>1,2);
				++dst;
			} while(--count != 0);
			drawConnections(params);
		break;

		case 1:
			do {
				dst->x = FB_WIDTH / 2 + (int)(sin((t/2 + 768*count)/2624.0) * 148);
				dst->y = FB_HEIGHT / 2 + (int)(sin((t/2 + 624*count)/1238.0) * 96);
				drawBlob32(dst->x,dst->y,blobSizesNum>>1,3);
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
		//src->x = (src->x - ((count & 3) + 1)) & (STARS_CUBE_DEPTH - 1);
		//src->y = (src->y - ((count & 3) + 1)) & (STARS_CUBE_DEPTH - 1);
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
	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	//drawEffect(&bgParams1, t);
	drawEffect(&bgParamsStars, t);

	blobBufferToVram32();

	swap_buffers(0);
}
