#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "opt_rend.h"
#include "demo.h"
#include "screen.h"

#define BLOB_SIZES_NUM_MAX 16
#define BLOB_SIZEX_PAD 4
#define MAX_BLOB_COLOR 15

#define LN_BASE 8
#define LN_AND ((1 << LN_BASE) - 1)


static BlobData blobData[BLOB_SIZES_NUM_MAX][BLOB_SIZEX_PAD];

static int isBlobGfxInit = 0;

// Needs 8bpp chunky buffer and then permutations of two pixels (256*256) to 32bit (two 16bit pixels)
void buffer8bppToVram(unsigned char *buffer, unsigned int *colMap16to32)
{
	int i;
	unsigned short *src = (unsigned short*)buffer;
	unsigned int *dst = (unsigned int*)fb_pixels;

	for (i=0; i<FB_WIDTH * FB_HEIGHT / 2; ++i) {
		*dst++ = colMap16to32[*src++];
	}
}

unsigned int *createColMap16to32(unsigned short *srcPal)
{
	unsigned int *dstPal = (unsigned int*)malloc(sizeof(unsigned int) * 256 * 256);

	int i,j,k = 0;
	for (j=0; j<256; ++j) {
		const int c0 = srcPal[j];
		for (i=0; i<256; ++i) {
			const int c1 = srcPal[i];
			dstPal[k++] = (c0 << 16) | c1;
		}
	}

	return dstPal;
}

void initBlobGfx()
{
	if (!isBlobGfxInit) {
		int i,j,x,y;

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
					const float yci = yc / (blobSizeYhalf - 0.5f);

					for (x=0; x<blobSizeX; ++x) {
						const int xc = (float)(x - j) - blobSizeXhalf + 0.5f;
						const float xci = xc / (blobSizeYhalf - 0.5f);

						float r = 1.0f - (xci*xci + yci*yci);
						CLAMP01(r)
						*dst++ = (int)(pow(r, 2.0f) * MAX_BLOB_COLOR);
					}
				}
			}
		}
		isBlobGfxInit = 1;
	}
}

void freeBlobGfx()
{
	if (isBlobGfxInit) {
		int i,j;
		for (i=0; i<BLOB_SIZES_NUM_MAX; ++i) {
			for (j=0; j<BLOB_SIZEX_PAD; ++j) {
				free(blobData[i][j].data);
			}
		}
		isBlobGfxInit = 0;
	}
}

void drawBlobs(Vertex3D *v, int count, unsigned char *blobBuffer)
{
	if (count <=0) return;

	do {
		const int size = 1 + (v->z >> 6);
		if (size < BLOB_SIZES_NUM_MAX) {
			const int posX = v->x;
			const int posY = v->y;
			BlobData *bd = &blobData[size][posX & (BLOB_SIZEX_PAD-1)];
			const int sizeX = bd->sizeX;
			const int sizeY = bd->sizeY;

			if (!(posX <= sizeX / 2 || posX >= FB_WIDTH - sizeX / 2 || posY <= sizeY / 2 || posY >= FB_HEIGHT - sizeY / 2))
			{
				const int posX32 = posX & ~(BLOB_SIZEX_PAD-1);
				const int wordsX = sizeX / 4;

				unsigned int *dst = (unsigned int*)(blobBuffer + (posY - sizeY / 2) * FB_WIDTH + (posX32 - sizeX / 2));
				unsigned int *src = (unsigned int*)bd->data;

				int y;
				for (y=0; y<sizeY; ++y) {
					int x;
					for (x=0; x<wordsX; ++x) {
						*(dst+x) += *(src+x);
					}
					src += wordsX;
					dst += FB_WIDTH / 4;
				}
			}
		}
		++v;
	}while(--count != 0);
}

void drawBlob(int posX, int posY, int size, int shift, unsigned char *blobBuffer)
{
	if (size < BLOB_SIZES_NUM_MAX) {
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
				const unsigned int c = *(src+x) << shift;
				*(dst+x) += c;
			}
			src += wordsX;
			dst += FB_WIDTH / 4;
		}
	}
}

void drawAntialiasedLine(Vertex3D *v1, Vertex3D *v2, int shadeShift, unsigned char *buffer)
{
	int x1 = v1->x;
	int y1 = v1->y;
	int x2 = v2->x;
	int y2 = v2->y;

	int vramofs;
	int frac, shade;

    int chdx, chdy;

	int dx = x2 - x1;
	int dy = y2 - y1;

	if (dx==0 && dy==0) return;

    chdx = dx;
	chdy = dy;
    if (dx<0) chdx = -dx;
    if (dy<0) chdy = -dy;

	if (chdy < chdx) {
		int x, yy, ddy;
		if (x1 > x2) {
			int temp = x1; x1 = x2; x2 = temp;
			y1 = y2;
		}

		if (dx==0) return;
        ddy = (dy << LN_BASE) / dx;
        yy = y1 << LN_BASE;
		for (x=x1; x<x2; x++) {
			const int yp = yy >> LN_BASE;

			if (x >= 0 && x < FB_WIDTH && yp >=0 && yp < FB_HEIGHT - 1) {
				vramofs = yp*FB_WIDTH + x;
				frac = yy & LN_AND;

				shade = (LN_AND - frac) >> shadeShift;
				*(buffer + vramofs) |= shade;

				shade = frac >> shadeShift;
				*(buffer + vramofs+FB_WIDTH) |= shade;
			}
            yy+=ddy;
		}
	}
	else {
		int y, xx, ddx;
		if (y1 > y2) {
			int temp = y1; y1 = y2; y2 = temp;
			x1 = x2;
		}

		if (dy==0) return;
        ddx = (dx << LN_BASE) / dy;
        xx = x1 << LN_BASE;

		for (y=y1; y<y2; y++) {
			const int xp = xx >> LN_BASE;

			if (y >= 0 && y < FB_HEIGHT && xp >=0 && xp < FB_WIDTH - 1) {
				vramofs = y*FB_WIDTH + xp;
				frac = xx & LN_AND;

				shade = (LN_AND - frac) >> shadeShift;
				*(buffer + vramofs) |= shade;

				shade = frac >> shadeShift;
				*(buffer + vramofs + 1) |= shade;
			}
            xx+=ddx;
		}
	}
}

void setPalGradient(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, unsigned short* pal)
{
	int i;
	const int dc = (c1 - c0);
	const int dr = ((r1 - r0) << 16) / dc;
	const int dg = ((g1 - g0) << 16) / dc;
	const int db = ((b1 - b0) << 16) / dc;

	r0 <<= 16;
	g0 <<= 16;
	b0 <<= 16;

	for (i = c0; i <= c1; i++)
	{
		int r = r0 >> 16;
		int g = g0 >> 16;
		int b = b0 >> 16;

		if (r < 0) r = 0; if (r > 31) r = 31;
		if (g < 0) g = 0; if (g > 63) g = 63;
		if (b < 0) b = 0; if (b > 31) b = 31;

		pal[i] = (r<<11) | (g<<5) | b;

		r0 += dr;
		g0 += dg;
		b0 += db;
	}
}
