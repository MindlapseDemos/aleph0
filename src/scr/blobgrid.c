/* Blobgrid effect idea */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"

#define NUM_POINTS 128
#define FP_PT 8

#define BLOB_SIZES_NUM 4
#define BLOB_SIZEX_PAD 4
#define MAX_BLOB_COLOR 127

static int *xp;
static int *yp;

static unsigned long startingTime;

static unsigned short *blobsPal;

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
	static float blobPowVals[4] = { 4.0f, 3.5f, 3.4f, 3.3f };

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

					// hacks to make more optimal data for unrolled codes. Those low values appear even more than the inner white and are almost invisible to the eye.
					//if (i==0 && c<=2) c = 0;
					//if (i==1 && c<=1) c = 0;

					*dst++ = c;
				}
			}
		}
	}
}

static int init(void)
{
	int i;

	blobsPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);
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

	initBlobGfx();

	return 0;
}

static void destroy(void)
{
	free(blobsPal);
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

	unsigned short *dst = (unsigned short*)fb_pixels + (posY - sizeY / 2) * FB_WIDTH + posX - sizeX / 2;

	unsigned char *src = bd->data;

	for (y=0; y<sizeY; ++y) {
		for (x=0; x<sizeX; ++x) {
			*(dst + x) |= blobsPal[*(src + x)];
		}
		src += sizeX;
		dst += FB_WIDTH;
	}
}

/*static void drawBlobNew32(int posX, int posY, int size)
{
	int x,y;
	unsigned short *dst = (unsigned short*)fb_pixels + posY * FB_WIDTH + posX;
	
	BlobData *bd;
	unsigned int *src;
	int wordsX;

	if (size < 0) size = 0;
	if (size > BLOB_SIZES_NUM-1) size = BLOB_SIZES_NUM-1;

	bd = &blobData[size][posX & (BLOB_SIZEX_PAD-1)];
	wordsX = bd->sizeX / 4;
	sizeX = bd->sizeX;

	sizeY = bd->sizeY;
	src = (unsigned int*)bd->data + (posY - bd->sizeY / 2) * wordsX + (((posX & ~3) - bd->sizeX / 2) / 4);

	for (y=0; y<sizeY; ++y) {
		for (x=0; x<wordsX; ++x) {
			unsigned short color = 
			*(dst + x) = blobsPal[color];
		}
		dst += FB_WIDTH;
	}
}*/

static void drawConnections()
{
	int i,j,k;
	const int breaks = 16;
	const int max = 512;
	const int bp = max / breaks;

	for (j=0; j<NUM_POINTS; ++j) {
		for (i=0; i<NUM_POINTS; ++i) {
			if (i!=j) {
				const int lx = xp[i] - xp[j];
				const int ly = yp[i] - yp[j];
				const int dst = lx*lx + ly*ly;

				if (dst >= bp && dst < max) {
					const int steps = dst / bp;
					int px = xp[i] << FP_PT;
					int py = yp[i] << FP_PT;
					const int dx = ((xp[j] - xp[i]) << FP_PT) / steps;
					const int dy = ((yp[j] - yp[i]) << FP_PT) / steps;

					for (k=0; k<steps-1; ++k) {
						px += dx;
						py += dy;
						drawBlob(px>>FP_PT, py>>FP_PT,3);
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

		drawBlob(xp[i],yp[i],2);
	}
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	drawPoints(t);
	drawConnections();

	swap_buffers(0);
}
