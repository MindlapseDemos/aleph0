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

static int *radius;
static int *latitude;
static int *longitude;

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

static void updateDotsVolumeBufferRadial(int t)
{
	int i;
	const int tt = t >> 4;
	const int size = VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH;
	const int thres1 = VERTICES_WIDTH / 2;
	const int thres2 = thres1 + 3;

	unsigned char *dst = getDotsVolumeBuffer();

	for (i=0; i<size; ++i) {
		const int r1 = latitude[i];
		const int r2 = longitude[i];
		const int r = radius[i] + (psin1[(r1-tt) & 2047] >> 5) + (psin2[(r2+tt) & 2047] >> 5);

		if (r >= thres1 && r <= thres2) {
			const int rr = 32 + ((r - thres1) << 4);
			*dst = rr;
		} else {
			*dst = 0;
		}

		++dst;
	}
}

static void updateDotsVolumeBufferRadialRays(int t)
{
	int i;
	const int tt = t >> 4;
	const int size = VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH;
	const int thres = 192;

	unsigned char *dst = getDotsVolumeBuffer();

	for (i=0; i<size; ++i) {
		const int r1 = latitude[i];
		const int r2 = longitude[i];
		const int r = radius[i];
		const int d = (psin1[(r1-tt) & 2047] + psin2[(r2+tt) & 2047] + psin3[(r1+r2+tt) & 2047]) & 255;

		if (d >= thres) {
			int rr = 255 - ((r*r) >> 1);
			if (rr < 0) rr = 0;
			*dst = rr;
		} else {
			*dst = 0;
		}

		++dst;
	}
}

static void updateDotsVolumeBufferFireball(int t)
{
	int i;
	const int size = VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH;
	const int thres1 = (VERTICES_WIDTH * VERTICES_WIDTH) / 8;
	const int thres2 = thres1 + 64;

	unsigned char *dst = getDotsVolumeBuffer();

	for (i=0; i<size; ++i) {
		const int r = radius[i];
		if (r >= thres1 && r <= thres2) {
			*dst = 255;
		} else {
			*dst = 0;
		}
		++dst;
	}
}

static void updateDotsVolumeBufferRandomWalk(int t)
{
}

static float qp[13*4] = {	-1.0f, 0.2f, 0.0f, 0.0f, 
						-0.291f, -0.399f, 0.339f, 0.437f, 
						-0.2f, 0.4f, -0.4f, -0.4f, 
						-0.213f, -0.041f, -0.563f, -0.56f,
						-0.2f, 0.6f, 0.2f, 0.2f, 
						-0.162f, 0.163f, 0.56f, -0.599f, 
						-0.2f, 0.8f, 0.0f, 0.0f, 
						-0.445f, 0.339f, -0.0889f, -0.562f, 
						0.185f, 0.478f, 0.125f, -0.392f, 
						-0.45f, -0.447f, 0.181f, 0.306f, 
						-0.218f, -0.113f, -0.181f, -0.496f, 
						-0.137f, -0.630f, -0.475f, -0.046f, 
						-0.125f, -0.256f, 0.847f, 0.0895f };

static void updateDotsVolumeBufferQuaternionJulia(int t)
{
	int x,y,z;

	unsigned char *dst = getDotsVolumeBuffer();

	const int index = (t >> 9) % 13;

	const float xp = qp[4*index];
	const float yp = qp[4*index+1];
	const float zp = qp[4*index+2];
	const float wp = qp[4*index+3];

	/*const float xp = sin(t * 0.0004f) * 0.4f;
	const float yp = sin(t * 0.0003f) * 0.4f;
	const float zp = sin(t * 0.0002f) * 0.4f;
	const float wp = sin(t * 0.0001f) * 0.6f;*/

	for (z=0; z<VERTICES_DEPTH; ++z) {
		for (y=0; y<VERTICES_HEIGHT; ++y) {
			for (x=0; x<VERTICES_WIDTH; ++x) {
				float xc = ((float)x - VERTICES_WIDTH/2) / (VERTICES_WIDTH/2);
				float yc = ((float)y - VERTICES_HEIGHT/2) / (VERTICES_HEIGHT/2);
				float zc = ((float)z - VERTICES_DEPTH/2) / (VERTICES_DEPTH/2);
				float wc = 0;//zc;

				int i;
				for (i=0; i<16; ++i) {
					const float xd = xc*xc - yc*yc - zc*zc - wc*wc + xp;
					const float yd = 2 * xc * yc + yp;
					const float zd = 2 * xc * zc + zp;
					const float wd = 2 * xc * wc + wp;

					const float r = xd*xd + yd*yd + zd*zd + wd*wd;
					if (r > 2.0f) break;
					
					xc = xd;
					yc = yd;
					zc = zd;
					wc = wd;
				}
				if (i < 12) i = 0;
				*dst++ = i << 2;
			}
		}
	}
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

static void initRadialEffects()
{
	int x,y,z,i;
	const int size = VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH;

	radius = (int*)malloc(size * sizeof(int));
	latitude = (int*)malloc(size * sizeof(int));
	longitude = (int*)malloc(size * sizeof(int));

	i = 0;
	for (z=0; z<VERTICES_DEPTH; ++z) {
		const float zc = (float)z - VERTICES_DEPTH/2;
		for (y=0; y<VERTICES_HEIGHT; ++y) {
			const float yc = (float)y - VERTICES_HEIGHT/2;
			for (x=0; x<VERTICES_WIDTH; ++x) {
				const float xc = (float)x - VERTICES_WIDTH/2;
				const float r = sqrt(xc*xc + yc*yc + zc*zc);

				radius[i] = (int)r;
				latitude[i] = (int)((atan2(yc,xc) * 256) / (2.0 * M_PI)) + 128;
				longitude[i] = (int)((acos(zc / r) * 256) / M_PI);
				++i;
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

	setPalGradient(0,127, 0,0,0, 31,63,63, polkaPal);
	setPalGradient(128,255, 31,63,31, 23,15,7, polkaPal);

	polkaPal32 = createColMap16to32(polkaPal);

	initPlasma3D();
	initBlobs3D();
	initRadialEffects();

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

	free(radius);
	free(latitude);
	free(longitude);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void draw(void)
{
	const int t = time_msec - startingTime;
	
	memset(polkaBuffer, 0, FB_WIDTH * FB_HEIGHT);

	//updateDotsVolumeBufferPlasma(t);
	//updateDotsVolumeBufferBlobs(t);
	//updateDotsVolumeBufferRadial(t);
	//updateDotsVolumeBufferRadialRays(t);

	//updateDotsVolumeBufferFireball(t);
	//updateDotsVolumeBufferRandomWalk(t);

	updateDotsVolumeBufferQuaternionJulia(t);

	Opt3Drun(polkaBuffer, t);

	buffer8bppToVram(polkaBuffer, polkaPal32);

	swap_buffers(0);
}
