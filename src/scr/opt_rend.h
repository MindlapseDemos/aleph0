#ifndef OPT_REND_H
#define OPT_REND_H

#include "opt_3d.h"
#include "demo.h"

#define CLAMP(v,a,b) if (v < a) v = a; if (v > b) v = b;
#define CLAMP01(v) if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f;

#define TEX_SHADES_NUM 256

#define FP_SCALE 16

#define POLKA_BUFFER_PAD 32
#define POLKA_BUFFER_WIDTH (FB_WIDTH + 2 * POLKA_BUFFER_PAD)
#define POLKA_BUFFER_HEIGHT (FB_HEIGHT + 2 * POLKA_BUFFER_PAD)

enum {
	OPT_RAST_FLAT, 
	OPT_RAST_TEXTURED_GOURAUD_CLIP_Y
};

typedef struct BlobData
{
	int wordSizeX, sizeY;
	int centerX, centerY;
	unsigned char *data;
} BlobData;


void clearBlobBuffer(unsigned char* buffer);
void buffer8bppToVram(unsigned char *buffer, unsigned int *colMap16to32);
void buffer8bppORwithVram(unsigned char* buffer, unsigned int* colMap16to32);
unsigned int *createColMap16to32(unsigned short *srcPal);

void initOptRasterizer(int maxPolys);
void freeOptRasterizer();

void initBlobGfx();
void freeBlobGfx();
void drawBlob(int posX, int posY, int size, unsigned char *blobBuffer);
void drawBlobPointsPolkaSize1(Vertex3D* v, int count, unsigned char* blobBuffer);
void drawBlobPointsPolkaSize2(Vertex3D* v, int count, unsigned char* blobBuffer);
void drawAntialiasedLine8bpp(Vertex3D *v1, Vertex3D *v2, int shadeShift, unsigned char *buffer);
void drawAntialiasedLine16bpp(Vertex3D* v1, Vertex3D* v2, int shadeShift, unsigned short* vram);

void setPalGradient(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, unsigned short* pal);
void setMainTexture(int width, int height, unsigned char* texData);
void setTexShadePal(uint16_t* pal);

void renderPolygons(Object3D* obj, Vertex3D* screenVertices);

void setRenderingMode(int mode);
int getRenderingMode();
void setClipValY(int y);

#endif
