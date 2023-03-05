/* Blobgrid effect idea */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"

#define NUM_POINTS 64
#define FP_PT 8

#define BLOB_SIZES_NUM 12
#define BLOB_SIZEX_PAD 4
#define MAX_BLOB_COLOR 31

#define CONNECTION_DIST 2048
#define CONNECTION_BREAKS 32

#define CLAMP01(v) if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f;

static int *xp;
static int *yp;

static unsigned long startingTime;

static unsigned short *blobsPal;
static unsigned int *blobsPal32;
static unsigned char *blobBuffer;

typedef struct BlobData
{
	int sizeX, sizeY;
	unsigned char *data;
} BlobData;

BlobData blobData[BLOB_SIZES_NUM][BLOB_SIZEX_PAD];


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

	for (i=0; i<BLOB_SIZES_NUM; ++i) {
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
				//CLAMP01(yci)

				for (x=0; x<blobSizeX; ++x) {
					const int xc = (float)(x - j) - blobSizeXhalf + 0.5f;
					float xci = fabs(xc / (blobSizeYhalf - 0.5f));
					//CLAMP01(xci)

					r = 1.0f - (xci*xci + yci*yci);
					CLAMP01(r)
					*dst++ = (int)(pow(r, 2.0f) * MAX_BLOB_COLOR);
				}
			}
		}
	}
}

static int init(void)
{
	int i,j,k;

	blobsPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);
	blobsPal32 = (unsigned int*)malloc(sizeof(unsigned int) * 256 * 256);
	xp = (int*)malloc(sizeof(int) * NUM_POINTS);
	yp = (int*)malloc(sizeof(int) * NUM_POINTS);

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

	return 0;
}

static void destroy(void)
{
	free(blobsPal);
	free(blobsPal32);
	free(xp);
	free(yp);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void drawBlobOld(int posX, int posY, int size, int color)
{
	int i,j;
	unsigned short *dst = (unsigned short*)fb_pixels + posY * FB_WIDTH + posX;

	for (j=0; j<size; ++j) {
		for (i=0; i<size; ++i) {
			*(dst + i) = blobsPal[color];
		}
		dst += FB_WIDTH;
	}
}

static void drawBlob(int posX, int posY, int size)
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

static void drawBlob32(int posX, int posY, int size)
{
	int x,y;
	const int posX32 = posX & ~(BLOB_SIZEX_PAD-1);
	
	BlobData *bd = &blobData[size][posX & (BLOB_SIZEX_PAD-1)];
	const int sizeX = bd->sizeX;
	const int sizeY = bd->sizeY;

	unsigned int *dst = (unsigned int*)(blobBuffer + (posY - sizeY / 2) * FB_WIDTH + (posX32 - sizeX / 2));

	const int wordsX = sizeX / 4;
	unsigned int *src = (unsigned int*)bd->data;

	for (y=0; y<sizeY; ++y) {
		for (x=0; x<wordsX; ++x) {
			unsigned int c = *(dst+x) + *(src+x);
			*(dst+x) = c; // | ((c & 0x80808080) >> 1);
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

static void drawConnections()
{
	int i,j,k;

	const int bp = CONNECTION_DIST / CONNECTION_BREAKS;

	for (j=0; j<NUM_POINTS; ++j) {
		for (i=0; i<NUM_POINTS; ++i) {
			if (i!=j) {
				const int lx = xp[i] - xp[j];
				const int ly = yp[i] - yp[j];
				const int dst = lx*lx + ly*ly;

				if (dst >= bp && dst < CONNECTION_DIST) {
					const int steps = dst / bp;
					int px = xp[i] << FP_PT;
					int py = yp[i] << FP_PT;
					const int dx = ((xp[j] - xp[i]) << FP_PT) / steps;
					const int dy = ((yp[j] - yp[i]) << FP_PT) / steps;

					for (k=0; k<steps-1; ++k) {
						int iii = k << 0;
						if (iii < 0) iii = 0; if (iii > BLOB_SIZES_NUM-1) iii = BLOB_SIZES_NUM-1;
						px += dx;
						py += dy;
						drawBlob32(px>>FP_PT, py>>FP_PT,BLOB_SIZES_NUM - 1 - iii);
					}
				}
			}
		}
	}
}

static void drawPoints(int t)
{
	int i;

	for (i=0; i<NUM_POINTS; ++i) {
		xp[i] = FB_WIDTH / 2 + (int)(sin((t + 9768*i)/7924.0) * 148);
		yp[i] = FB_HEIGHT / 2 + (int)(sin((t + 7024*i)/13838.0) * 96);

		drawBlob32(xp[i],yp[i],BLOB_SIZES_NUM-1);
	}
}

static void draw(void)
{
	int i,y=0;
	const int t = time_msec - startingTime;

	memset(blobBuffer, 0, FB_WIDTH * FB_HEIGHT);
	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	drawPoints(t);
	drawConnections();

	blobBufferToVram32();

	swap_buffers(0);
}
