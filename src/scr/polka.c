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

#define PSIN_SIZE 2048

#define BLOB_SIZE 12
#define NUM_BLOBS 16

#define DOT_COLOR 0xFFFF


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

static Vertex3D* objectGridVertices;
static Vertex3D* transformedGridVertices;
static Vertex3D* objectAxesVertices;
static Vertex3D* transformedAxesVertices;

static ScreenPoints screenPointsGrid;

static Vertex3D* axisVerticesX, * axisVerticesY, * axisVerticesZ;

static unsigned char* volumeData;

static int* recDivZ;

static int *radius;
static int *latitude;
static int *longitude;

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
						const int sx = FB_WIDTH / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->x + axisY->x + axisZ->x, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosX;
						const int sy = FB_HEIGHT / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->y + axisY->y + axisZ->y, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosY;

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
	SET_OPT_VERTEX(AXIS_WIDTH, 0, 0, v)	++v;
	SET_OPT_VERTEX(0, AXIS_HEIGHT, 0, v)	++v;
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

	free(recDivZ);
}



static void updateDotsVolumeBufferPlasma(int t)
{
	unsigned char *dst = volumeData;
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

	unsigned char* dst = volumeData;

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

	unsigned char* dst = volumeData;

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

/*static void updateDotsVolumeBufferRandomWalk(int t)
{
}*/

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
	unsigned char* dst = volumeData;
	const int tt = t >> 3;

	memset(dst, 0, VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH);

	for (i=0; i<NUM_BLOBS; ++i) {
		blobPos[i].x = (int)(VERTICES_WIDTH / 2 + sin((72*i+1*tt) / 45.0f) * ((VERTICES_WIDTH / 2) - 2*BLOB_SIZE/3));
		blobPos[i].y = (int)(VERTICES_HEIGHT / 2 + sin((91*i+2*tt) / 63.0f) * ((VERTICES_WIDTH / 2) - 2*BLOB_SIZE/3));
		blobPos[i].z = (int)(VERTICES_DEPTH / 2 + sin((114*i+3*tt) / 127.0f) * ((VERTICES_WIDTH / 2) - 2*BLOB_SIZE/3));

		drawBlob3D(&blobPos[i], dst);
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
					v[i].xs = FB_WIDTH / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->x + axisY->x + axisZ->x, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosX;
					v[i].ys = FB_HEIGHT / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->y + axisY->y + axisZ->y, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE) + objPosY;
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

static void OptGrid3Drun(unsigned char* buffer, int ticks)
{
	const int objIndex = 1;

	ticks >>= 1;

	initScreenPointsGrid(transformedGridVertices);
	rotateVertices(objectAxesVertices, transformedAxesVertices, 3, ticks, 2 * ticks, 3 * ticks);
	generateAxesVertices(transformedAxesVertices);

	transformAndProjectAxesBoxDotsEffect(objIndex);

	drawBoxLines(buffer, -1, objIndex);

	drawBlobs(screenPointsGrid.v, screenPointsGrid.num, buffer);

	drawBoxLines(buffer, 1, objIndex);
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

				float r = sqrt(xc*xc + yc*yc + zc*zc);
				if (r<0.001f) r = 0.001f;

				radius[i] = (int)r;
				latitude[i] = (int)((atan2(yc,xc) * 256) / (2.0 * M_PI)) + 128;
				longitude[i] = (int)((acos(zc / r) * 256) / M_PI);
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
	OptGrid3Dinit();
	initBlobGfx();

	polkaBuffer = (unsigned char*)malloc(FB_WIDTH * FB_HEIGHT);
	polkaPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);

	setPalGradient(0,127, 0,0,0, 31,63,63, polkaPal);
	setPalGradient(128,255, 31,63,31, 23,15,7, polkaPal);

	polkaPal32 = createColMap16to32(polkaPal);

	initPlasma3D();
	initBlobs3D();
	initRadialEffects();

	setGridPos(&gridPos[0], 0, 0, 1024);
	setGridPos(&gridPos[1], 128, 0, 1024);
	
	return 0;
}

static void destroy(void)
{
	OptGrid3Dfree();
	freeBlobGfx();

	free(polkaBuffer);
	free(polkaPal);
	free(polkaPal32);

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
	const int tt = 0;// (t >> 13) & 1;
	
	memset(polkaBuffer, 0, FB_WIDTH * FB_HEIGHT);

	switch(tt) {
		case 0:
			updateDotsVolumeBufferRadial(t);
		break;

		case 1:
			updateDotsVolumeBufferRadialRays(t);
		break;

		case 2:
			updateDotsVolumeBufferPlasma(t);
		break;

		case 3:
			//updateDotsVolumeBufferRandomWalk(t);
		break;

		default:
		break;
	}

	OptGrid3Drun(polkaBuffer, t);

	buffer8bppToVram(polkaBuffer, polkaPal32);

	swap_buffers(0);
}
