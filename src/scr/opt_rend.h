#ifndef OPT_REND_H
#define OPT_REND_H

#define CLAMP01(v) if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f;

typedef struct BlobData
{
	int sizeX, sizeY;
	unsigned char *data;
} BlobData;


void buffer8bppToVram(unsigned char *buffer, unsigned int *colMap16to32);
unsigned int *createColMap16to32(unsigned short *srcPal);

void initBlobGfx();
void freeBlobGfx();
void drawBlob(int posX, int posY, int size, int shift, unsigned char *blobBuffer);

#endif
