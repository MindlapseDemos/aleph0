/* Dummy part to test the performance of my mini 3d engine, also occupied for future 3D Polka dots idea */

#include "demo.h"
#include "screen.h"

#include "opt_3d.h"
#include "opt_rend.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


#define VERTICES_WIDTH 32
#define VERTICES_HEIGHT 32
#define VERTICES_DEPTH 32
#define MAX_VERTEX_ELEMENTS_NUM (VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH)
#define SET_OPT_VERTEX(xp,yp,zp,v) v->x = (xp); v->y = (yp); v->z = (zp);

#define AXIS_SHIFT 4
#define AXIS_WIDTH (VERTICES_WIDTH<<AXIS_SHIFT)
#define AXIS_HEIGHT (VERTICES_HEIGHT<<AXIS_SHIFT)
#define AXIS_DEPTH (VERTICES_DEPTH<<AXIS_SHIFT)

#define PSIN_SIZE 1024

#define DOT_COLOR 0xFFFF
#define VOLS_NUM 2


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static unsigned short *tempPal;
static unsigned int *polkaPal32[VOLS_NUM];
static unsigned char *polkaBuffer[VOLS_NUM];

static unsigned char *psin1, *psin2, *psin3;

static Vertex3D* objectGridVertices;
static Vertex3D* transformedGridVertices;
static Vertex3D* objectAxesVertices;
static Vertex3D* transformedAxesVertices;

static ScreenPoints screenPointsGrid;

static Vertex3D* axisVerticesX, * axisVerticesY, * axisVerticesZ;

static unsigned char* volumeData;

static int* recDivZ;

typedef struct PolarData
{
	unsigned char radius;
	unsigned char latitude;
	unsigned char longitude;    
} PolarData;

PolarData *polarData;


static Vector3D gridPos[2];


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



static void transformAndProjectAxesBoxDotsEffect(int objIndex)
{
	unsigned char* src = volumeData;

	Vertex3D* dst = screenPointsGrid.v;
	Vertex3D* axisZ = axisVerticesZ;
	int countZ = VERTICES_DEPTH;

	const int objPosX = gridPos[objIndex].x;
	const int objPosY = gridPos[objIndex].y;
	const int objPosZ = gridPos[objIndex].z;

	do {
		Vertex3D* axisY = axisVerticesY;
		int countY = VERTICES_HEIGHT;
		do {

			Vertex3D* axisX = axisVerticesX;
			int countX = VERTICES_WIDTH;
			do {
				const unsigned char c = *src++;
				if (c != 0) {
					const int sz = FIXED_TO_INT(axisX->z + axisY->z + axisZ->z, FP_CORE) + objPosZ;
					if (sz > 0 && sz < REC_DIV_Z_MAX) {
						const int recZ = recDivZ[(int)sz];
						const int sx = POLKA_BUFFER_WIDTH / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->x + axisY->x + axisZ->x, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosX;
						const int sy = POLKA_BUFFER_HEIGHT / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->y + axisY->y + axisZ->y, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosY;

						dst->xs = sx;
						dst->ys = sy;
						dst->z = c;
						screenPointsGrid.num++;
						++dst;
					}
				}
				++axisX;
			} while (--countX != 0);
			++axisY;
		} while (--countY != 0);
		++axisZ;
	} while (--countZ != 0);
}

static void generateAxisVertices(Vertex3D* rotatedAxis, Vertex3D* dstAxis, int count)
{
	const float dx = (float)rotatedAxis->x / (VERTICES_WIDTH - 1);
	const float dy = (float)rotatedAxis->y / (VERTICES_HEIGHT - 1);
	const float dz = (float)rotatedAxis->z / (VERTICES_DEPTH - 1);
	const float halfSteps = (float)count / 2;
	float px = -halfSteps * dx;
	float py = -halfSteps * dy;
	float pz = -halfSteps * dz;
	do {
		const int x = FLOAT_TO_INT(px, FP_CORE);
		const int y = FLOAT_TO_INT(py, FP_CORE);
		const int z = FLOAT_TO_INT(pz, FP_CORE);
		SET_OPT_VERTEX(x, y, z, dstAxis);
		px += dx;
		py += dy;
		pz += dz;
		++dstAxis;
	} while (--count != 0);
}

static void generateAxesVertices(Vertex3D* axesEnds)
{
	generateAxisVertices(&axesEnds[0], axisVerticesX, VERTICES_WIDTH);
	generateAxisVertices(&axesEnds[1], axisVerticesY, VERTICES_HEIGHT);
	generateAxisVertices(&axesEnds[2], axisVerticesZ, VERTICES_DEPTH);
}

static void initGrid3D()
{
	objectGridVertices = (Vertex3D*)malloc(MAX_VERTEX_ELEMENTS_NUM * sizeof(Vertex3D));
	transformedGridVertices = (Vertex3D*)malloc(MAX_VERTEX_ELEMENTS_NUM * sizeof(Vertex3D));

	volumeData = (unsigned char*)malloc(MAX_VERTEX_ELEMENTS_NUM);
	memset(volumeData, 0, MAX_VERTEX_ELEMENTS_NUM);
}

static void initAxes3D()
{
	Vertex3D* v;

	objectAxesVertices = (Vertex3D*)malloc(3 * sizeof(Vertex3D));
	transformedAxesVertices = (Vertex3D*)malloc(3 * sizeof(Vertex3D));

	axisVerticesX = (Vertex3D*)malloc(VERTICES_WIDTH * sizeof(Vertex3D));
	axisVerticesY = (Vertex3D*)malloc(VERTICES_HEIGHT * sizeof(Vertex3D));
	axisVerticesZ = (Vertex3D*)malloc(VERTICES_DEPTH * sizeof(Vertex3D));

	v = objectAxesVertices;
	SET_OPT_VERTEX(AXIS_WIDTH, 0, 0, v) ++v;
	SET_OPT_VERTEX(0, AXIS_HEIGHT, 0, v)    ++v;
	SET_OPT_VERTEX(0, 0, AXIS_DEPTH, v)
}

static void initScreenPointsGrid(Vertex3D* src)
{
	screenPointsGrid.num = 0;
	screenPointsGrid.v = src;
}

void OptGrid3Dinit()
{
	int z;

	initAxes3D();
	initGrid3D();

	recDivZ = (int*)malloc(REC_DIV_Z_MAX * sizeof(int));
	for (z = 1; z < REC_DIV_Z_MAX; ++z) {
		recDivZ[z] = FLOAT_TO_INT(1 / (float)z, FP_CORE);
	}
}

static void OptGrid3Dfree()
{
	free(objectAxesVertices);
	free(transformedAxesVertices);

	free(objectGridVertices);
	free(transformedGridVertices);
	free(volumeData);

	free(axisVerticesX);
	free(axisVerticesY);
	free(axisVerticesZ);

	free(polarData);
	free(recDivZ);
}



static void updateDotsVolumeBufferPlasma(int t)
{
	unsigned char *dst = volumeData;
	const int tt = t >> 5;

	int countZ = VERTICES_DEPTH;
	do {
		const int p3z = psin3[(3*countZ-tt) & (PSIN_SIZE-1)];
		int countY = VERTICES_HEIGHT;
		do {
			const int p2y = psin2[(2*countY+tt) & (PSIN_SIZE-1)];
			int countX = VERTICES_WIDTH;
			do {
				unsigned char c = psin1[(2*countX+tt) & (PSIN_SIZE-1)] + p2y + p3z;
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

	unsigned char* dst = volumeData;

	for (i=0; i<size; ++i) {
		PolarData *pData = &polarData[i];
		const int r0 = pData->radius;
		const int r1 = pData->latitude;
		const int r2 = pData->longitude;
		const int r = r0 + (psin1[(r1-tt) & (PSIN_SIZE-1)] >> 5) + (psin2[(r2+tt) & (PSIN_SIZE-1)] >> 5);

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

	unsigned char* dst = volumeData;

	for (i=0; i<size; ++i) {
		PolarData *pData = &polarData[i];
		const int r0 = pData->radius;
		const int r1 = pData->latitude;
		const int r2 = pData->longitude;
		/* const int d = (psin1[(r1 - tt) & (PSIN_SIZE - 1)] + psin2[(r2 + tt) & (PSIN_SIZE - 1)] + psin3[(r1 + r2 + tt) & (PSIN_SIZE - 1)]) & 255; */
		const int d = (psin1[(r1 - tt) & (PSIN_SIZE - 1)] + psin2[(r2 + tt) & (PSIN_SIZE - 1)]) & 255;

		if (d >= thres) {
			int rr = 255 - ((r0*r0) >> 1);
			if (rr < 0) rr = 0;
			*dst = rr;
		} else {
			*dst = 0;
		}

		++dst;
	}
}

static void drawQuadLines(Vertex3D* v0, Vertex3D* v1, Vertex3D* v2, Vertex3D* v3, unsigned char* buffer, int orderSign)
{
	if (orderSign * ((v0->xs - v1->xs) * (v2->ys - v1->ys) - (v2->xs - v1->xs) * (v0->ys - v1->ys)) <= 0) {
		const int shadeShift = 3 - orderSign;
		drawAntialiasedLine8bpp(v0, v1, shadeShift, buffer);
		drawAntialiasedLine8bpp(v1, v2, shadeShift, buffer);
		drawAntialiasedLine8bpp(v2, v3, shadeShift, buffer);
		drawAntialiasedLine8bpp(v3, v0, shadeShift, buffer);
	}
}

static void drawBoxLines(unsigned char* buffer, int orderSign, int objIndex)
{
	Vertex3D v[8];
	int x, y, z, i = 0;

	const int objPosX = gridPos[objIndex].x;
	const int objPosY = gridPos[objIndex].y;
	const int objPosZ = gridPos[objIndex].z;

	for (z = 0; z <= 1; ++z) {
		Vertex3D* axisZ = &axisVerticesZ[z * (VERTICES_DEPTH - 1)];
		for (y = 0; y <= 1; ++y) {
			Vertex3D* axisY = &axisVerticesY[y * (VERTICES_HEIGHT - 1)];
			for (x = 0; x <= 1; ++x) {
				Vertex3D* axisX = &axisVerticesX[x * (VERTICES_WIDTH - 1)];
				const int sz = AFTER_MUL_ADDS(axisX->z + axisY->z + axisZ->z, FP_CORE) + objPosZ;
				if (sz > 0 && sz < REC_DIV_Z_MAX) {
					const int recZ = recDivZ[(int)sz];
					v[i].xs = POLKA_BUFFER_WIDTH / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->x + axisY->x + axisZ->x, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosX;
					v[i].ys = POLKA_BUFFER_HEIGHT / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->y + axisY->y + axisZ->y, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosY;
					v[i].z = sz;
				}
				++i;
			}
		}
	}

	drawQuadLines(&v[0], &v[1], &v[3], &v[2], buffer, orderSign);
	drawQuadLines(&v[1], &v[5], &v[7], &v[3], buffer, orderSign);
	drawQuadLines(&v[5], &v[4], &v[6], &v[7], buffer, orderSign);
	drawQuadLines(&v[4], &v[0], &v[2], &v[6], buffer, orderSign);
	drawQuadLines(&v[2], &v[3], &v[7], &v[6], buffer, orderSign);
	drawQuadLines(&v[4], &v[5], &v[1], &v[0], buffer, orderSign);
}

static void OptGrid3Drun(int objIndex, unsigned char* buffer, int ticks)
{
	ticks >>= 1;

	initScreenPointsGrid(transformedGridVertices);
	rotateVertices(objectAxesVertices, transformedAxesVertices, 3, (2-objIndex) * ticks, (2+objIndex) * ticks, (3-objIndex) * ticks);
	generateAxesVertices(transformedAxesVertices);

	transformAndProjectAxesBoxDotsEffect(objIndex);

	/* drawBoxLines(buffer, -1, objIndex); */

	if (objIndex == 0) {
		drawBlobPointsPolkaSize2(screenPointsGrid.v, screenPointsGrid.num, buffer);
	} else {
		drawBlobPointsPolkaSize1(screenPointsGrid.v, screenPointsGrid.num, buffer);
	}

	/* drawBoxLines(buffer, 1, objIndex); */
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

static void initRadialEffects()
{
	int x,y,z,i;
	const int size = VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH;

	polarData = (PolarData*)malloc(size * sizeof(PolarData));

	i = 0;
	for (z=0; z<VERTICES_DEPTH; ++z) {
		const float zc = (float)z - VERTICES_DEPTH/2;
		for (y=0; y<VERTICES_HEIGHT; ++y) {
			const float yc = (float)y - VERTICES_HEIGHT/2;
			for (x=0; x<VERTICES_WIDTH; ++x) {
				const float xc = (float)x - VERTICES_WIDTH/2;

				float r = sqrt(xc*xc + yc*yc + zc*zc);
				if (r<0.001f) r = 0.001f;

				polarData[i].radius = (unsigned char)r;
				polarData[i].latitude = (unsigned char)((atan2(yc,xc) * 256) / (2.0 * M_PI)) + 128;
				polarData[i].longitude = (unsigned char)((acos(zc / r) * 256) / M_PI);

				++i;
			}
		}
	}
}

static void setGridPos(Vector3D* pos, int x, int y, int z)
{
	pos->x = x;
	pos->y = y;
	pos->z = z;
}

static int init(void)
{
	int i;

	OptGrid3Dinit();
	initBlobGfx();


	tempPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);

	for (i = 0; i < VOLS_NUM; ++i) {
		polkaBuffer[i] = (unsigned char*)malloc(POLKA_BUFFER_WIDTH * POLKA_BUFFER_HEIGHT);

		switch(i) {
			case 0:
				setPalGradient(0, 127, 0, 0, 0, 0, 31, 63, tempPal);
				setPalGradient(128, 255, 0, 31, 63, 15, 47, 63, tempPal);
			break;

			case 1:
				setPalGradient(0, 127, 0, 0, 0, 63, 31, 7, tempPal);
				setPalGradient(128, 255, 63, 31, 7, 63, 47, 23, tempPal);
			break;
		}

		polkaPal32[i] = createColMap16to32(tempPal);
	}

	free(tempPal);

	initPlasma3D();
	initRadialEffects();
	
	return 0;
}

static void destroy(void)
{
	int i;

	OptGrid3Dfree();
	freeBlobGfx();

	free(psin1);
	free(psin2);
	free(psin3);

	for (i = 0; i < VOLS_NUM; ++i) {
		free(polkaBuffer[i]);
		free(polkaPal32[i]);
	}
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	int i;
	for (i = 0; i < VOLS_NUM; ++i) {
		clearBlobBuffer(polkaBuffer[i]);

		if (i == 0) {
			updateDotsVolumeBufferRadial(t);
		} else {
			updateDotsVolumeBufferRadialRays(t);
			/* updateDotsVolumeBufferPlasma(t); */
		}
		setGridPos(&gridPos[i], sin((3550*i + (i+1)*t) / 2277.0f) * 56, sin((4950 * i + (2-i)*t) / 1567.0f) * 32, 1024);

		OptGrid3Drun(i, polkaBuffer[i], t);
	}

	buffer8bppToVram(polkaBuffer[0], polkaPal32[0]);
	buffer8bppORwithVram(polkaBuffer[1], polkaPal32[1]);

	swap_buffers(0);
}
