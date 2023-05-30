/* Dummy part to test the performance of my mini 3d engine, also occupied for future 3D Polka dots idea */

#include "demo.h"
#include "screen.h"

#include "opt_3d.h"
#include "opt_rend.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


#define PSIN_SIZE 2048

#define BLOB_SIZE 12
#define NUM_BLOBS 16

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static unsigned short *polkaPal;
static unsigned int *polkaPal32;
static unsigned char *polkaBuffer;

static unsigned char *psin1, *psin2, *psin3;

static unsigned char *blob3D;
static Vertex3D blobPos[NUM_BLOBS];


static struct screen scr = {
	"polka",
	init,
	destroy,
	start,
	0,
	draw
};

static unsigned long startingTime;

struct screen *polka_screen(void)
{
	return &scr;
}


static void updateDotsVolumeBufferPlasma(int t)
{
	unsigned char *dst = getDotsVolumeBuffer();
	const int tt = t >> 5;

	int countZ = VERTICES_DEPTH;
	do {
		const int p3z = psin3[(3*countZ-tt) & 2047];
		int countY = VERTICES_HEIGHT;
		do {
			const int p2y = psin2[(2*countY+tt) & 2047];
			int countX = VERTICES_WIDTH;
			do {
				unsigned char c = psin1[(2*countX+tt) & 2047] + p2y + p3z;
				if (c < 192) {
					c = 0;
				} else {
					c = (c - 192) << 2;
				}
				*dst++ = c;
			} while(--countX != 0);
		} while(--countY != 0);
	} while(--countZ != 0);
}

static void drawBlob3D(Vertex3D *pos, unsigned char *buffer)
{
	int x,y,z;
	unsigned char *src = blob3D;
	unsigned char *dst = buffer;

	for (z=0; z<BLOB_SIZE; ++z) {
		const int zp = pos->z + z - BLOB_SIZE/2;
		if (zp>=0 && zp<VERTICES_DEPTH) {
			const int zi = zp * VERTICES_WIDTH * VERTICES_HEIGHT;
			for (y=0; y<BLOB_SIZE; ++y) {
				const int yp = pos->y + y - BLOB_SIZE/2;
				if (yp>=0 && yp<VERTICES_HEIGHT) {
					const int yi = yp * VERTICES_WIDTH;
					for (x=0; x<BLOB_SIZE; ++x) {
						const int xp = pos->x + x - BLOB_SIZE/2;
						if (xp>=0 && xp<VERTICES_WIDTH) {
							const int i = zi + yi + xp;
							int c = *(dst + i) + *src;
							if (c > 255) c =  255;
							*(dst + i) = c;
						}
						++src;
					}
				}
			}
		}
	}
}

static void updateDotsVolumeBufferBlobs(int t)
{
	int i;
	unsigned char *dst = getDotsVolumeBuffer();
	const int tt = t >> 3;

	memset(dst, 0, VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH);

	for (i=0; i<NUM_BLOBS; ++i) {
		blobPos[i].x = (int)(VERTICES_WIDTH / 2 + sin((72*i+1*tt) / 45.0f) * ((VERTICES_WIDTH / 2) - 2*BLOB_SIZE/3));
		blobPos[i].y = (int)(VERTICES_HEIGHT / 2 + sin((91*i+2*tt) / 63.0f) * ((VERTICES_WIDTH / 2) - 2*BLOB_SIZE/3));
		blobPos[i].z = (int)(VERTICES_DEPTH / 2 + sin((114*i+3*tt) / 127.0f) * ((VERTICES_WIDTH / 2) - 2*BLOB_SIZE/3));

		drawBlob3D(&blobPos[i], dst);
	}
}


static void initPlasma3D()
{
	int i;

	psin1 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);
	psin2 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);
	psin3 = (unsigned char*)malloc(sizeof(unsigned char) * PSIN_SIZE);

	for (i = 0; i < PSIN_SIZE; i++) {
		const float s = 0.7f;
		const float l = 0.9f;
		psin1[i] = (unsigned char)(sin((l * (double)i) / 7.0) * s*123.0 + s*123.0);
		psin2[i] = (unsigned char)(sin((l * (double)i) / 11.0) * s*176.0 + s*176.0);
		psin3[i] = (unsigned char)(sin((l * (double)i) / 9.0) * s*118.0 + s*118.0);
	}
}

static void initBlobs3D()
{
	int x,y,z;
	int i = 0;

	blob3D = (unsigned char*)malloc(BLOB_SIZE * BLOB_SIZE * BLOB_SIZE);

	for (z=0; z<BLOB_SIZE; ++z) {
		const float zc = (float)z - BLOB_SIZE/2 + 0.5f;
		const float zci = zc / (BLOB_SIZE/2 - 0.5f);
		for (y=0; y<BLOB_SIZE; ++y) {
			const float yc = (float)y - BLOB_SIZE/2 + 0.5f;
			const float yci = yc / (BLOB_SIZE/2 - 0.5f);
			for (x=0; x<BLOB_SIZE; ++x) {
				const float xc = (float)x - BLOB_SIZE/2 + 0.5f;
				const float xci = xc / (BLOB_SIZE/2 - 0.5f);

				unsigned int c = 0;
				float r = 1.0f - (xci*xci + yci*yci + zci*zci);
				CLAMP01(r)
				if (r > 0.0f) c = (int)(64.0f/pow(r, 2.0f));
				if (c > 64) c = 64;
				blob3D[i++] = c;
			}
		}
	}
}

static int init(void)
{
	int i;

	Opt3Dinit();
	initBlobGfx();

	polkaBuffer = (unsigned char*)malloc(FB_WIDTH * FB_HEIGHT);
	polkaPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);

	for (i=0; i<128; i++) {
		int r = i >> 2;
		int g = i >> 1;
		int b = i >> 1;
		if (b > 31) b = 31;
		polkaPal[i] = (r<<11) | (g<<5) | b;
	}
	for (i=128; i<256; i++) {
		int r = 31 - ((i-128) >> 4);
		int g = 63 - ((i-128) >> 2);
		int b = 31 - ((i-128) >> 3);
		polkaPal[i] = (r<<11) | (g<<5) | b;
	}

	polkaPal32 = createColMap16to32(polkaPal);

	initPlasma3D();
	initBlobs3D();

	return 0;
}

static void destroy(void)
{
	Opt3Dfree();
	freeBlobGfx();

	free(polkaBuffer);
	free(polkaPal);

	free(psin1);
	free(psin2);
	free(psin3);

	free(blob3D);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void draw(void)
{
	const int t = time_msec - startingTime;
	
	memset(polkaBuffer, 0, FB_WIDTH * FB_HEIGHT);

	updateDotsVolumeBufferPlasma(t);
	//updateDotsVolumeBufferBlobs(t);

	Opt3Drun(polkaBuffer, t);

	buffer8bppToVram(polkaBuffer, polkaPal32);

	swap_buffers(0);
}
