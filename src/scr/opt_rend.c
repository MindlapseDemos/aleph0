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

void drawBlob(int posX, int posY, int size, int shift, unsigned char *blobBuffer)
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
