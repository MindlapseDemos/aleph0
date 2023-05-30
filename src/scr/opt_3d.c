#include <math.h>
#include <string.h>

#include "opt_3d.h"
#include "opt_rend.h"

#include "demo.h"
#include "screen.h"
#include "util.h"

#define OBJECT_POS_Z 1024
#define REC_DIV_Z_MAX 2048

#define AXIS_SHIFT 4
#define AXIS_WIDTH (VERTICES_WIDTH<<AXIS_SHIFT)
#define AXIS_HEIGHT (VERTICES_HEIGHT<<AXIS_SHIFT)
#define AXIS_DEPTH (VERTICES_DEPTH<<AXIS_SHIFT)


#define FP_CORE 16
#define FP_BASE 12
#define FP_BASE_TO_CORE (FP_CORE - FP_BASE)
#define PROJ_SHR 8
#define PROJ_MUL (1 << PROJ_SHR)

#define FLOAT_TO_FIXED(f,b) ((int)((f) * (1 << (b))))
#define INT_TO_FIXED(i,b) ((i) * (1 << (b)))
#define UINT_TO_FIXED(i,b) ((i) << (b))
#define FIXED_TO_INT(x,b) ((x) >> (b))
#define FIXED_TO_FLOAT(x,b) ((float)(x) / (1 << (b)))
#define FIXED_MUL(x,y,b) (((x) * (y)) >> (b))
#define FIXED_DIV(x,y,b) (((x) << (b)) / (y))
#define FIXED_SQRT(x,b) (isqrt((x) << (b)))

#define DEG_TO_RAD_256(x) (((2 * M_PI) * (x)) / 256)
#define RAD_TO_DEG_256(x) ((256 * (x)) / (2 * M_PI))


#define AFTER_MUL_ADDS(x,b)		FIXED_TO_INT(x,b)
#define AFTER_RECZ_MUL(x,b)		FIXED_TO_INT(x,b)
#define FLOAT_TO_int(x,b)		FLOAT_TO_FIXED(x,b)
#define DOT_COLOR				0xFFFF


static Vertex3D *objectGridVertices;
static Vertex3D *transformedGridVertices;
static Vertex3D *objectAxesVertices;
static Vertex3D *transformedAxesVertices;
static ScreenPoints screenPoints;

static Vertex3D *axisVerticesX, *axisVerticesY, *axisVerticesZ;

static int *recDivZ;

static unsigned char *volumeData;

static int isOpt3Dinit = 0;


unsigned char *getDotsVolumeBuffer()
{
	return volumeData;
}

static void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *mat)
{
	int *rotVecs = (int*)mat;

	const float aX = (float)(DEG_TO_RAD_256((float)rotX / 64.0f));
	const float aY = (float)(DEG_TO_RAD_256((float)rotY / 64.0f));
	const float aZ = (float)(DEG_TO_RAD_256((float)rotZ / 64.0f));

	const int cosxr = (int)(FLOAT_TO_int(cos(aX), FP_BASE));
	const int cosyr = (int)(FLOAT_TO_int(cos(aY), FP_BASE));
	const int coszr = (int)(FLOAT_TO_int(cos(aZ), FP_BASE));
	const int sinxr = (int)(FLOAT_TO_int(sin(aX), FP_BASE));
	const int sinyr = (int)(FLOAT_TO_int(sin(aY), FP_BASE));
	const int sinzr = (int)(FLOAT_TO_int(sin(aZ), FP_BASE));

	*rotVecs++ = (FIXED_MUL(cosyr, coszr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(FIXED_MUL(sinxr, sinyr, FP_BASE), coszr, FP_BASE) - FIXED_MUL(cosxr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(FIXED_MUL(cosxr, sinyr, FP_BASE), coszr, FP_BASE) + FIXED_MUL(sinxr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(cosyr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(cosxr, coszr, FP_BASE) + FIXED_MUL(FIXED_MUL(sinxr, sinyr, FP_BASE), sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(-sinxr, coszr, FP_BASE) + FIXED_MUL(FIXED_MUL(cosxr, sinyr, FP_BASE), sinzr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs++ = (-sinyr) << FP_BASE_TO_CORE;
	*rotVecs++ = (FIXED_MUL(sinxr, cosyr, FP_BASE)) << FP_BASE_TO_CORE;
	*rotVecs = (FIXED_MUL(cosxr, cosyr, FP_BASE)) << FP_BASE_TO_CORE;
}

static void MulManyVec3Mat33(Vertex3D *dst, Vertex3D *src, int *m, int count)
{
	int i;
	for (i = 0; i < count; ++i)
	{
		const int x = src->x;
		const int y = src->y;
		const int z = src->z;

		dst->x = AFTER_MUL_ADDS(x * m[0] + y * m[3] + z * m[6], FP_CORE);
		dst->y = AFTER_MUL_ADDS(x * m[1] + y * m[4] + z * m[7], FP_CORE);
		dst->z = AFTER_MUL_ADDS(x * m[2] + y * m[5] + z * m[8], FP_CORE);

		++src;
		++dst;
	}
}

static void translateAndProjectVertices(Vertex3D *src, Vertex3D *dst, int count)
{
	int i;
	const int offsetX = (int)(FB_WIDTH >> 1);
	const int offsetY = (int)(FB_HEIGHT >> 1);

	for (i=0; i<count; i++) {
		const int vz = src[i].z + OBJECT_POS_Z;
		dst[i].z = vz;
		if (vz > 0) {
			dst[i].x = offsetX + (src[i].x * PROJ_MUL) / vz;
			dst[i].y = offsetY + (src[i].y * PROJ_MUL) / vz;
		}
	}
}

static void rotateVertices(Vertex3D *src, Vertex3D *dst, int count, int t)
{
	static int rotMat[9];

	createRotationMatrixValues(t, 2*t, 3*t, rotMat);

	MulManyVec3Mat33(dst, src, rotMat, count);
}

static void renderVertices(unsigned char *buffer)
{
	Vertex3D *src = screenPoints.v;
	const int count = screenPoints.num;

	int i;
	for (i = 0; i < count; i++) {
		const int x = src->x;
		const int y = src->y;
		const int z = src->z;

		if (z > 0 && x >= 0 && x < FB_WIDTH && y >= 0 && y < FB_HEIGHT) {
			int c = (OBJECT_POS_Z + 256 - z) >> 4;
			if (c < 0) c = 0;
			if (c > 31) c = 31;
			*(buffer + y * FB_WIDTH + x) = c;
		}
		++src;
	}
}

static void transformAndProjectAxesBoxDotsEffect()
{
	unsigned char *src = volumeData;

	const int offsetX = (int)(FB_WIDTH >> 1);
	const int offsetY = (int)(FB_HEIGHT >> 1);

	Vertex3D *dst = screenPoints.v;
	Vertex3D *axisZ = axisVerticesZ;
	int countZ = VERTICES_DEPTH;
	do {
		Vertex3D *axisY = axisVerticesY;
		int countY = VERTICES_HEIGHT;
		do {
		
			Vertex3D *axisX = axisVerticesX;
			int countX = VERTICES_WIDTH;
			do {
				const unsigned char c = *src++;
				if (c != 0) {
					const int sz = AFTER_MUL_ADDS(axisX->z + axisY->z + axisZ->z, FP_CORE) + OBJECT_POS_Z;
					if (sz > 0 && sz < REC_DIV_Z_MAX) {
						const int recZ = recDivZ[(int)sz];
						const int sx = offsetX + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->x + axisY->x + axisZ->x, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE);
						const int sy = offsetY + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->y + axisY->y + axisZ->y, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE);

						dst->x = sx;
						dst->y = sy;
						dst->z = c;
						screenPoints.num++;
						++dst;
					}
				}
				++axisX;
			} while(--countX != 0);
			++axisY;
		} while(--countY != 0);
		++axisZ;
	} while(--countZ != 0);
}

static void generateAxisVertices(Vertex3D *rotatedAxis, Vertex3D *dstAxis, int count)
{
	const float dx = (float)rotatedAxis->x / (VERTICES_WIDTH - 1);
	const float dy = (float)rotatedAxis->y / (VERTICES_HEIGHT - 1);
	const float dz = (float)rotatedAxis->z / (VERTICES_DEPTH - 1);
	const float halfSteps = (float)count / 2;
	float px = -halfSteps * dx;
	float py = -halfSteps * dy;
	float pz = -halfSteps * dz;
	do {
		const int x = FLOAT_TO_int(px, FP_CORE);
		const int y = FLOAT_TO_int(py, FP_CORE);
		const int z = FLOAT_TO_int(pz, FP_CORE);
		SET_OPT_VERTEX(x,y,z,dstAxis);
		px += dx;
		py += dy;
		pz += dz;
		++dstAxis;
	} while(--count != 0);
}

static void generateAxesVertices(Vertex3D *axesEnds)
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
	SET_OPT_VERTEX(AXIS_WIDTH,0,0,v)	++v;
	SET_OPT_VERTEX(0,AXIS_HEIGHT,0,v)	++v;
	SET_OPT_VERTEX(0,0,AXIS_DEPTH,v)
}

static void initScreenPoints(Vertex3D *src)
{
	screenPoints.num = 0;
	screenPoints.v = src;
}

void Opt3Dinit()
{
	if (!isOpt3Dinit) {
		int z;

		initAxes3D();
		initGrid3D();

		recDivZ = (int*)malloc(REC_DIV_Z_MAX * sizeof(int));
		for (z=1; z<REC_DIV_Z_MAX; ++z) {
			recDivZ[z] = FLOAT_TO_int(1/(float)z, FP_CORE);
		}
		isOpt3Dinit = 1;
	}
}

void Opt3Dfree()
{
	if (isOpt3Dinit) {
		free(objectAxesVertices);
		free(transformedAxesVertices);

		free(objectGridVertices);
		free(transformedGridVertices);
		free(volumeData);

		free(axisVerticesX);
		free(axisVerticesY);
		free(axisVerticesZ);

		free(recDivZ);

		isOpt3Dinit = 0;
	}
}

void Opt3Drun(unsigned char *buffer, int ticks)
{
	ticks >>= 1;

	initScreenPoints(transformedGridVertices);
	rotateVertices(objectAxesVertices, transformedAxesVertices, 3, ticks);
	generateAxesVertices(transformedAxesVertices);

	transformAndProjectAxesBoxDotsEffect();

	drawBlobs(screenPoints.v, screenPoints.num, buffer);
}
