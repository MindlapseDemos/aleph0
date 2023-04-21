#include <math.h>
#include <string.h>

#include "opt_3d.h"

#include "demo.h"
#include "screen.h"
#include "util.h"

#define VERTICES_WIDTH 32
#define VERTICES_HEIGHT 32
#define VERTICES_DEPTH 32
#define MAX_VERTEX_ELEMENTS_NUM (VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH)

#define FP_CORE 16
#define FP_BASE 12
#define FP_BASE_MUL (1 << FP_BASE)
#define FP_BASE_TO_CORE (FP_CORE - FP_BASE)
#define PROJ_SHR 8

#define FLOAT_TO_FIXED(f,b) ((int)((f) * (1 << b)))
#define INT_TO_FIXED(i,b) ((i) * (1 << b))
#define UINT_TO_FIXED(i,b) ((i) << b)
#define FIXED_TO_INT(x,b) ((x) >> b)
#define FIXED_TO_FLOAT(x,b) ((float)(x) / (1 << b))
#define FIXED_MUL(x,y,b) (((x) * (y)) >> b)
#define FIXED_DIV(x,y,b) (((x) << b) / (y))
#define FIXED_SQRT(x,b) (isqrt((x) << b))

#define DEG_TO_RAD_256(x) (((2 * M_PI) * (x)) / 256)
#define RAD_TO_DEG_256(x) ((256 * (x)) / (2 * M_PI))


#define FIXED_TEST

#define OBJECT_POS_Z 1024


typedef double real;

typedef struct OptVertex
{
	#ifdef FIXED_TEST
		int x,y,z;
	#else
		real x, y, z;
	#endif
}OptVertex;


static OptVertex objectVertices[MAX_VERTEX_ELEMENTS_NUM];
static OptVertex screenVertices[MAX_VERTEX_ELEMENTS_NUM];


#ifdef FIXED_TEST
	static void createRotationMatrixValues(int rotX, int rotY, int rotZ, int *mat)
	{
		int *rotVecs = (int*)mat;

		real aX = (real)(DEG_TO_RAD_256((real)rotX / 64.0f));
		real aY = (real)(DEG_TO_RAD_256((real)rotY / 64.0f));
		real aZ = (real)(DEG_TO_RAD_256((real)rotZ / 64.0f));

		const int cosxr = (int)(cos(aX) * FP_BASE_MUL);
		const int cosyr = (int)(cos(aY) * FP_BASE_MUL);
		const int coszr = (int)(cos(aZ) * FP_BASE_MUL);
		const int sinxr = (int)(sin(aX) * FP_BASE_MUL);
		const int sinyr = (int)(sin(aY) * FP_BASE_MUL);
		const int sinzr = (int)(sin(aZ) * FP_BASE_MUL);

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
#else
	static void createRotationMatrixValues(real rotX, real rotY, real rotZ, real *mat)
	{
		real *rotVecs = (real*)mat;

		const real cosxr = (real)cos(rotX);
		const real cosyr = (real)cos(rotY);
		const real coszr = (real)cos(rotZ);
		const real sinxr = (real)sin(rotX);
		const real sinyr = (real)sin(rotY);
		const real sinzr = (real)sin(rotZ);

		*rotVecs++ = cosyr * coszr;
		*rotVecs++ = sinxr * sinyr * coszr - cosxr * sinzr;
		*rotVecs++ = cosxr * sinyr * coszr + sinxr * sinzr;

		*rotVecs++ = cosyr * sinzr;
		*rotVecs++ = cosxr * coszr + sinxr * sinyr * sinzr;
		*rotVecs++ = -sinxr * coszr + cosxr * sinyr * sinzr;
		*rotVecs++ = -sinyr;
		*rotVecs++ = sinxr * cosyr;
		*rotVecs = cosxr * cosyr;
	}
#endif

#ifdef FIXED_TEST
	static void MulManyVec3Mat33(OptVertex *dst, OptVertex *src, int *m, int count)
	{
		int i;
		for (i = 0; i < count; ++i)
		{
			const int x = src->x;
			const int y = src->y;
			const int z = src->z;

			dst->x = FIXED_TO_INT(x * m[0] + y * m[3] + z * m[6], FP_CORE);
			dst->y = FIXED_TO_INT(x * m[1] + y * m[4] + z * m[7], FP_CORE);
			dst->z = FIXED_TO_INT(x * m[2] + y * m[5] + z * m[8], FP_CORE);

			++src;
			++dst;
		}
	}
#else
	static void MulManyVec3Mat33(OptVertex *dst, OptVertex *src, real *m, int count)
	{
		int i;
		for (i = 0; i < count; ++i)
		{
			const real x = src->x;
			const real y = src->y;
			const real z = src->z;

			dst->x = x * m[0] + y * m[3] + z * m[6];
			dst->y = x * m[1] + y * m[4] + z * m[7];
			dst->z = x * m[2] + y * m[5] + z * m[8];

			++src;
			++dst;
		}
	}
#endif

#ifdef FIXED_TEST
static void translateAndProjectVertices()
{
	int i;
	const int offsetX = FB_WIDTH >> 1;
	const int offsetY = FB_HEIGHT >> 1;

	for (i=0; i<MAX_VERTEX_ELEMENTS_NUM; i++) {
		const int vz = screenVertices[i].z + OBJECT_POS_Z;
		screenVertices[i].z = vz;
		if (vz > 0) {
			screenVertices[i].x = offsetX + (screenVertices[i].x << PROJ_SHR) / vz;
			screenVertices[i].y = offsetY + (screenVertices[i].y << PROJ_SHR) / vz;
		}
	}
}
#else
static void translateAndProjectVertices()
{
	int i;
	const real offsetX = (real)(FB_WIDTH >> 1);
	const real offsetY = (real)(FB_HEIGHT >> 1);

	for (i=0; i<MAX_VERTEX_ELEMENTS_NUM; i++) {
		const real vz = screenVertices[i].z + OBJECT_POS_Z;
		screenVertices[i].z = vz;
		if (vz > 0) {
			screenVertices[i].x = offsetX + (screenVertices[i].x * (1 << PROJ_SHR)) / vz;
			screenVertices[i].y = offsetY + (screenVertices[i].y * (1 << PROJ_SHR)) / vz;
		}
	}
}
#endif

static void rotateVertices(int ticks)
{
	const int t = ticks >> 1;

	#ifdef FIXED_TEST
		static int rotMat[9];

		createRotationMatrixValues(t, 2*t, 3*t, rotMat);
	#else
		static real rotMat[9];

		createRotationMatrixValues((real)DEG_TO_RAD_256(t) / 64.0f, (real)DEG_TO_RAD_256(2*t) / 64.0f, (real)DEG_TO_RAD_256(3*t) / 64.0f, rotMat);
	#endif

	MulManyVec3Mat33(screenVertices, objectVertices, rotMat, MAX_VERTEX_ELEMENTS_NUM);
}

static void renderVertices()
{
	int i;
	unsigned short* dst = (unsigned short*)fb_pixels;

	OptVertex* v = screenVertices;
	for (i = 0; i < MAX_VERTEX_ELEMENTS_NUM; i++) {
		#ifdef FIXED_TEST
			const int x = v->x;
			const int y = v->y;
			const int z = v->z;
		#else
			const int x = cround64(v->x);
			const int y = cround64(v->y);
			const int z = cround64(v->z);
		#endif
		if (z > 0 && x >= 0 && x < FB_WIDTH && y >= 0 && y < FB_HEIGHT) {
			*(dst + y * FB_WIDTH + x) = 0xFFFF;
		}
		++v;
	}
}


static void initObject3D()
{
	int x,y,z;
	OptVertex* v = objectVertices;
	for (z = -VERTICES_DEPTH / 2; z < VERTICES_DEPTH / 2; ++z) {
		for (y = -VERTICES_HEIGHT / 2; y < VERTICES_HEIGHT / 2; ++y) {
			for (x = -VERTICES_WIDTH / 2; x < VERTICES_WIDTH / 2; ++x) {
				v->x = x<<4;
				v->y = y<<4;
				v->z = z<<4;
				++v;
			}
		}
	}
}

void Opt3DinitPerfTest(void)
{
	initObject3D();
}

void Opt3DrunPerfTest(int ticks)
{
	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	rotateVertices(ticks);
	translateAndProjectVertices();
	renderVertices();
}
