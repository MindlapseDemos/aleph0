/* Blobgrid effect idea */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"

#define NUM_POINTS 128
#define FP_PT 8

#define BLOB_SIZES_NUM 5
#define BLOB_SIZEX_PAD 4
#define MAX_BLOB_COLOR 127

#define CONNECTION_DIST 1024
#define CONNECTION_BREAKS 16

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
	static float blobPowVals[BLOB_SIZES_NUM] = { 4.0f, 3.5f, 3.4f, 3.3f, 3.2f };

	int i,j,x,y,c;

	for (i=0; i<BLOB_SIZES_NUM; ++i) {
		const int n = 6 - i;
		const int blobSize = 1<<n;
		const int blobSizeX = blobSize + BLOB_SIZEX_PAD;    // We are padding this, as we generate four pixels offset (for dword rendering)

		const float powVal = blobPowVals[i];
		const float halfBlobSize = (float)(blobSize / 2);
		const float r2max = halfBlobSize * halfBlobSize;
		const float hmm = (float)blobSize / 4.0f;

		float diver = 1.75f;    // div by 2 for more halo, 1 for more blobby

		for (j=0; j<BLOB_SIZEX_PAD; ++j) {
			unsigned char *dst;

			blobData[i][j].sizeX = blobSizeX;
			blobData[i][j].sizeY = blobSize;

			blobData[i][j].data = (unsigned char*)malloc(blobSizeX * blobSize);
			dst = blobData[i][j].data;

			for (y=0; y<blobSize; ++y) {
				const float yc = (float)y - halfBlobSize + 0.5f;
				for (x=0; x<blobSizeX; ++x) {
					const int xc = (float)(x - j) - halfBlobSize + 0.5f;
					float r2, cf;

					r2 = xc*xc + yc*yc;
					if (r2 < hmm) r2 = hmm;
					cf = r2max / r2;

					c = (int)pow(cf, powVal / diver);
					if (c < 0) c = 0;
					if (c > MAX_BLOB_COLOR) c = MAX_BLOB_COLOR;

					*dst++ = c;
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
		int b = 31 - ((i-128) >> 2);
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

	//const int bp = 8*CONNECTION_BREAKS;
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
						px += dx;
						py += dy;
						drawBlob32(px>>FP_PT, py>>FP_PT,4);
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

		drawBlob32(xp[i],yp[i],2);
	}
}
//11
static void draw(void)
{
	const int t = time_msec - startingTime;

	memset(blobBuffer, 0, FB_WIDTH * FB_HEIGHT);
	//memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	drawPoints(t);
	drawConnections();

	blobBufferToVram32();

	swap_buffers(0);
}
