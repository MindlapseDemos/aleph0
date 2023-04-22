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

#define OBJECT_POS_Z 1024

#define AXIS_SHIFT 4
#define AXIS_WIDTH (VERTICES_WIDTH<<AXIS_SHIFT)
#define AXIS_HEIGHT (VERTICES_HEIGHT<<AXIS_SHIFT)
#define AXIS_DEPTH (VERTICES_DEPTH<<AXIS_SHIFT)


#define FP_CORE 16
#define FP_BASE 12
#define FP_BASE_TO_CORE (FP_CORE - FP_BASE)
#define PROJ_MUL (1 << 8)

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


//#define FIXED_TEST

//#define DO_ALL_TOGETHER
#define AXES_TEST

#ifdef FIXED_TEST
	#define VECTYPE_AFTER_MUL_ADDS(x,b)		FIXED_TO_INT(x,b)
	#define VECTYPE_ROUND(x)				(x)
	#define FLOAT_TO_VECTYPE(x,b)			FLOAT_TO_FIXED(x,b)
#else
	#define VECTYPE_AFTER_MUL_ADDS(x,b)		(x)
	#define VECTYPE_ROUND(x)				(cround64(x))
	#define FLOAT_TO_VECTYPE(x,b)			(x)
#endif

typedef float real;

#ifdef FIXED_TEST
	typedef int vecType;
#else
	typedef real vecType;
#endif


typedef struct OptVertex
{
	vecType x,y,z;
}OptVertex;

#define SET_OPT_VERTEX(xp,yp,zp,v) v->x = (xp); v->y = (yp); v->z = (zp);


static OptVertex *objectVertices;
static OptVertex *transformedVertices;

static OptVertex *axisVerticesX, *axisVerticesY, *axisVerticesZ;


static void createRotationMatrixValues(vecType rotX, vecType rotY, vecType rotZ, vecType *mat)
{
	vecType *rotVecs = (vecType*)mat;

	const real aX = (real)(DEG_TO_RAD_256((real)rotX / 64.0f));
	const real aY = (real)(DEG_TO_RAD_256((real)rotY / 64.0f));
	const real aZ = (real)(DEG_TO_RAD_256((real)rotZ / 64.0f));

	const vecType cosxr = (vecType)(FLOAT_TO_VECTYPE(cos(aX), FP_BASE));
	const vecType cosyr = (vecType)(FLOAT_TO_VECTYPE(cos(aY), FP_BASE));
	const vecType coszr = (vecType)(FLOAT_TO_VECTYPE(cos(aZ), FP_BASE));
	const vecType sinxr = (vecType)(FLOAT_TO_VECTYPE(sin(aX), FP_BASE));
	const vecType sinyr = (vecType)(FLOAT_TO_VECTYPE(sin(aY), FP_BASE));
	const vecType sinzr = (vecType)(FLOAT_TO_VECTYPE(sin(aZ), FP_BASE));

	#ifdef FIXED_TEST
		*rotVecs++ = (FIXED_MUL(cosyr, coszr, FP_BASE)) << FP_BASE_TO_CORE;
		*rotVecs++ = (FIXED_MUL(FIXED_MUL(sinxr, sinyr, FP_BASE), coszr, FP_BASE) - FIXED_MUL(cosxr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
		*rotVecs++ = (FIXED_MUL(FIXED_MUL(cosxr, sinyr, FP_BASE), coszr, FP_BASE) + FIXED_MUL(sinxr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
		*rotVecs++ = (FIXED_MUL(cosyr, sinzr, FP_BASE)) << FP_BASE_TO_CORE;
		*rotVecs++ = (FIXED_MUL(cosxr, coszr, FP_BASE) + FIXED_MUL(FIXED_MUL(sinxr, sinyr, FP_BASE), sinzr, FP_BASE)) << FP_BASE_TO_CORE;
		*rotVecs++ = (FIXED_MUL(-sinxr, coszr, FP_BASE) + FIXED_MUL(FIXED_MUL(cosxr, sinyr, FP_BASE), sinzr, FP_BASE)) << FP_BASE_TO_CORE;
		*rotVecs++ = (-sinyr) << FP_BASE_TO_CORE;
		*rotVecs++ = (FIXED_MUL(sinxr, cosyr, FP_BASE)) << FP_BASE_TO_CORE;
		*rotVecs = (FIXED_MUL(cosxr, cosyr, FP_BASE)) << FP_BASE_TO_CORE;
	#else
		*rotVecs++ = cosyr * coszr;
		*rotVecs++ = sinxr * sinyr * coszr - cosxr * sinzr;
		*rotVecs++ = cosxr * sinyr * coszr + sinxr * sinzr;

		*rotVecs++ = cosyr * sinzr;
		*rotVecs++ = cosxr * coszr + sinxr * sinyr * sinzr;
		*rotVecs++ = -sinxr * coszr + cosxr * sinyr * sinzr;
		*rotVecs++ = -sinyr;
		*rotVecs++ = sinxr * cosyr;
		*rotVecs = cosxr * cosyr;
	#endif
}

static void MulManyVec3Mat33(OptVertex *dst, OptVertex *src, vecType *m, int count)
{
	int i;
	for (i = 0; i < count; ++i)
	{
		const vecType x = src->x;
		const vecType y = src->y;
		const vecType z = src->z;

		dst->x = VECTYPE_AFTER_MUL_ADDS(x * m[0] + y * m[3] + z * m[6], FP_CORE);
		dst->y = VECTYPE_AFTER_MUL_ADDS(x * m[1] + y * m[4] + z * m[7], FP_CORE);
		dst->z = VECTYPE_AFTER_MUL_ADDS(x * m[2] + y * m[5] + z * m[8], FP_CORE);

		++src;
		++dst;
	}
}

static void translateAndProjectVertices(int count)
{
	int i;
	const vecType offsetX = (vecType)(FB_WIDTH >> 1);
	const vecType offsetY = (vecType)(FB_HEIGHT >> 1);

	for (i=0; i<count; i++) {
		const vecType vz = transformedVertices[i].z + OBJECT_POS_Z;
		transformedVertices[i].z = vz;
		if (vz > 0) {
			transformedVertices[i].x = offsetX + (transformedVertices[i].x * PROJ_MUL) / vz;
			transformedVertices[i].y = offsetY + (transformedVertices[i].y * PROJ_MUL) / vz;
		}
	}
}

static void rotateVertices(int count, int t)
{
	static vecType rotMat[9];

	createRotationMatrixValues(t, 2*t, 3*t, rotMat);

	MulManyVec3Mat33(transformedVertices, objectVertices, rotMat, count);
}

static void renderVertices(int count)
{
	int i;
	unsigned short* dst = (unsigned short*)fb_pixels;

	OptVertex* v = transformedVertices;
	for (i = 0; i < count; i++) {
		const int x = VECTYPE_ROUND(v->x);
		const int y = VECTYPE_ROUND(v->y);
		const int z = VECTYPE_ROUND(v->z);

		if (z > 0 && x >= 0 && x < FB_WIDTH && y >= 0 && y < FB_HEIGHT) {
			*(dst + y * FB_WIDTH + x) = 0xFFFF;
		}
		++v;
	}
}

static void doAllTogether(OptVertex *v, int count, int t)
{
	static vecType m[9];

	unsigned short* dst = (unsigned short*)fb_pixels;
	int i;

	const vecType offsetX = (vecType)(FB_WIDTH >> 1);
	const vecType offsetY = (vecType)(FB_HEIGHT >> 1);

	createRotationMatrixValues(t, 2*t, 3*t, m);

	for (i = 0; i < count; ++i) {
		const vecType x = v->x;
		const vecType y = v->y;
		const vecType z = v->z;

		const vecType sz = VECTYPE_AFTER_MUL_ADDS(x * m[2] + y * m[5] + z * m[8], FP_CORE) + OBJECT_POS_Z;
		if (sz > 0) {
			const int sx = VECTYPE_ROUND(offsetX + ((VECTYPE_AFTER_MUL_ADDS(x * m[0] + y * m[3] + z * m[6], FP_CORE)) * PROJ_MUL) / sz);
			const int sy = VECTYPE_ROUND(offsetY + ((VECTYPE_AFTER_MUL_ADDS(x * m[1] + y * m[4] + z * m[7], FP_CORE)) * PROJ_MUL) / sz);

			if (sx >= 0 && sx < FB_WIDTH && sy >= 0 && sy < FB_HEIGHT) {
				*(dst + sy * FB_WIDTH + sx) = 0xFFFF;
			}
		}
		++v;
	}
}

static void initObject3D()
{
	int x,y,z;
	OptVertex* v;

	objectVertices = (OptVertex*)malloc(MAX_VERTEX_ELEMENTS_NUM * sizeof(OptVertex));
	transformedVertices = (OptVertex*)malloc(MAX_VERTEX_ELEMENTS_NUM * sizeof(OptVertex));

	v = objectVertices;
	for (z = -VERTICES_DEPTH / 2; z < VERTICES_DEPTH / 2; ++z) {
		for (y = -VERTICES_HEIGHT / 2; y < VERTICES_HEIGHT / 2; ++y) {
			for (x = -VERTICES_WIDTH / 2; x < VERTICES_WIDTH / 2; ++x) {
				SET_OPT_VERTEX(x<<AXIS_SHIFT,y<<AXIS_SHIFT,z<<AXIS_SHIFT,v)
				++v;
			}
		}
	}
}

static void renderAxesBoxDots()
{
	unsigned short* dst = (unsigned short*)fb_pixels;

	const vecType offsetX = (vecType)(FB_WIDTH >> 1);
	const vecType offsetY = (vecType)(FB_HEIGHT >> 1);

	OptVertex *axisZ = axisVerticesZ;
	int countZ = VERTICES_DEPTH;
	do {
		const vecType aZx = axisZ->x;
		const vecType aZy = axisZ->y;
		const vecType aZz = axisZ->z;

		OptVertex *axisY = axisVerticesY;
		int countY = VERTICES_HEIGHT;
		do {
			const vecType aYx = axisY->x;
			const vecType aYy = axisY->y;
			const vecType aYz = axisY->z;
			
			OptVertex *axisX = axisVerticesX;
			int countX = VERTICES_WIDTH;
			do {
				const vecType aXx = axisX->x;
				const vecType aXy = axisX->y;
				const vecType aXz = axisX->z;

				const vecType sz = VECTYPE_AFTER_MUL_ADDS(aXz + aYz + aZz, FP_CORE) + OBJECT_POS_Z;
				if (sz > 0) {
					const int sx = VECTYPE_ROUND(offsetX + ((VECTYPE_AFTER_MUL_ADDS(aXx + aYx + aZx, FP_CORE)) * PROJ_MUL) / sz);
					const int sy = VECTYPE_ROUND(offsetY + ((VECTYPE_AFTER_MUL_ADDS(aXy + aYy + aZy, FP_CORE)) * PROJ_MUL) / sz);

					if (sx >= 0 && sx < FB_WIDTH && sy >= 0 && sy < FB_HEIGHT) {
						*(dst + sy * FB_WIDTH + sx) = 0xFFFF;
					}
				}
				++axisX;
			} while(--countX != 0);
			++axisY;
		} while(--countY != 0);
		++axisZ;
	} while(--countZ != 0);
}

static void generateAxisVertices(OptVertex *rotatedAxis, OptVertex *dstAxis, int count)
{
	const real dx = (real)rotatedAxis->x / (VERTICES_WIDTH - 1);
	const real dy = (real)rotatedAxis->y / (VERTICES_HEIGHT - 1);
	const real dz = (real)rotatedAxis->z / (VERTICES_DEPTH - 1);
	const real halfSteps = (real)count / 2;
	real px = -halfSteps * dx;
	real py = -halfSteps * dy;
	real pz = -halfSteps * dz;
	do {
		const vecType x = FLOAT_TO_VECTYPE(px, FP_CORE);
		const vecType y = FLOAT_TO_VECTYPE(py, FP_CORE);
		const vecType z = FLOAT_TO_VECTYPE(pz, FP_CORE);
		SET_OPT_VERTEX(x,y,z,dstAxis);
		px += dx;
		py += dy;
		pz += dz;
		++dstAxis;
	} while(--count != 0);
}

static void generateAxesVertices(OptVertex *axesEnds)
{
	generateAxisVertices(&axesEnds[0], axisVerticesX, VERTICES_WIDTH);
	generateAxisVertices(&axesEnds[1], axisVerticesY, VERTICES_HEIGHT);
	generateAxisVertices(&axesEnds[2], axisVerticesZ, VERTICES_DEPTH);
}

static void initAxes3D()
{
	OptVertex* v;

	objectVertices = (OptVertex*)malloc(3 * sizeof(OptVertex));
	transformedVertices = (OptVertex*)malloc(3 * sizeof(OptVertex));

	axisVerticesX = (OptVertex*)malloc(VERTICES_WIDTH * sizeof(OptVertex));
	axisVerticesY = (OptVertex*)malloc(VERTICES_HEIGHT * sizeof(OptVertex));
	axisVerticesZ = (OptVertex*)malloc(VERTICES_DEPTH * sizeof(OptVertex));

	v = objectVertices;
	SET_OPT_VERTEX(AXIS_WIDTH,0,0,v)	++v;
	SET_OPT_VERTEX(0,AXIS_HEIGHT,0,v)	++v;
	SET_OPT_VERTEX(0,0,AXIS_DEPTH,v)
}

void Opt3DinitPerfTest()
{
	#ifdef AXES_TEST
		initAxes3D();
	#else
		initObject3D();
	#endif
}

void Opt3DfreePerfTest()
{
	free(objectVertices);
	free(transformedVertices);

	free(axisVerticesX);
	free(axisVerticesY);
	free(axisVerticesZ);
}

void Opt3DrunPerfTest(int ticks)
{
	memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);

	#ifdef AXES_TEST
		rotateVertices(3, ticks);
		generateAxesVertices(transformedVertices);
		renderAxesBoxDots();
	#else
		#ifdef DO_ALL_TOGETHER
			doAllTogether(objectVertices, MAX_VERTEX_ELEMENTS_NUM, ticks);
		#else
			rotateVertices(MAX_VERTEX_ELEMENTS_NUM, ticks);
			translateAndProjectVertices(MAX_VERTEX_ELEMENTS_NUM);
			renderVertices(MAX_VERTEX_ELEMENTS_NUM);
		#endif
	#endif
}
