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

#define DOT_COLOR				0xFFFF


static Vertex3D *objectGridVertices;
static Vertex3D *transformedGridVertices;
static Vertex3D *objectAxesVertices;
static Vertex3D *transformedAxesVertices;

static ScreenPoints screenPointsGrid;
static ScreenPoints screenPointsObject;

static Vertex3D *axisVerticesX, *axisVerticesY, *axisVerticesZ;

static int *recDivZ;

static unsigned char *volumeData;

static Vertex3D* transformedObjectVertices;
static int* currentIndexPtr;

static int isOpt3Dinit = 0;



int isqrt(int x)
{
    long long int q = 1;	/* very high numbers over((1 << 30) - 1) will freeze in while if this wasn't 64bit */
	int r = 0;
    while (q <= x) {
        q <<= 2;
    }
    while (q > 1) {
        int t;
        q >>= 2;
        t = (int)(x - r - q);
        r >>= 1;
        if (t >= 0) {
            x = t;
            r += (int)q;
        }
    }
    return r;
} 


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

static void translateAndProjectVertices(Vertex3D *src, Vertex3D *dst, int count, int posX, int posY, int posZ)
{
	int i;

	for (i=0; i<count; i++) {
		const int vx = src[i].x + posX;
		const int vy = src[i].y + posY;
		const int vz = src[i].z + posZ;
		dst[i].x = vx;
		dst[i].y = vy;
		dst[i].z = vz;
		if (vz > 0) {
			dst[i].xs = FB_WIDTH / 2 + (vx * PROJ_MUL) / vz;
			dst[i].ys = FB_HEIGHT / 2 - (vy * PROJ_MUL) / vz;
		}
	}
}

static void rotateVertices(Vertex3D *src, Vertex3D *dst, int count, int rx, int ry, int rz)
{
	static int rotMat[9];

	createRotationMatrixValues(rx, ry, rz, rotMat);

	MulManyVec3Mat33(dst, src, rotMat, count);
}

static void renderVertices(unsigned char *buffer)
{
	Vertex3D *src = screenPointsGrid.v;
	const int count = screenPointsGrid.num;

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

	Vertex3D *dst = screenPointsGrid.v;
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
						const int sx = FB_WIDTH / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->x + axisY->x + axisZ->x, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE);
						const int sy = FB_HEIGHT / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->y + axisY->y + axisZ->y, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE);

						dst->xs = sx;
						dst->ys = sy;
						dst->z = c;
						screenPointsGrid.num++;
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

static void initScreenPointsGrid(Vertex3D *src)
{
	screenPointsGrid.num = 0;
	screenPointsGrid.v = src;
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

static void drawQuadLines(Vertex3D *v0, Vertex3D *v1, Vertex3D *v2, Vertex3D *v3, unsigned char *buffer, int orderSign)
{
	if (orderSign * ((v0->xs - v1->xs) * (v2->ys - v1->ys) - (v2->xs - v1->xs) * (v0->ys - v1->ys)) <= 0) {
		const int shadeShift = 3 - orderSign;
		drawAntialiasedLine8bpp(v0, v1, shadeShift, buffer);
		drawAntialiasedLine8bpp(v1, v2, shadeShift, buffer);
		drawAntialiasedLine8bpp(v2, v3, shadeShift, buffer);
		drawAntialiasedLine8bpp(v3, v0, shadeShift, buffer);
	}
}

void drawBoxLines(unsigned char *buffer, int orderSign)
{
	Vertex3D v[8];
	int x,y,z,i=0;

	for (z=0; z<=1; ++z) {
		Vertex3D *axisZ = &axisVerticesZ[z * (VERTICES_DEPTH-1)];
		for (y=0; y<=1; ++y) {
			Vertex3D *axisY = &axisVerticesY[y * (VERTICES_HEIGHT-1)];
			for (x=0; x<=1; ++x) {
				Vertex3D *axisX = &axisVerticesX[x * (VERTICES_WIDTH-1)];
				const int sz = AFTER_MUL_ADDS(axisX->z + axisY->z + axisZ->z, FP_CORE) + OBJECT_POS_Z;
				if (sz > 0 && sz < REC_DIV_Z_MAX) {
					const int recZ = recDivZ[(int)sz];
					v[i].xs = FB_WIDTH / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->x + axisY->x + axisZ->x, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE);
					v[i].ys = FB_HEIGHT / 2 + AFTER_RECZ_MUL(((AFTER_MUL_ADDS(axisX->y + axisY->y + axisZ->y, FP_CORE)) * PROJ_MUL) * recZ, FP_CORE);
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

void Opt3Drun(unsigned char *buffer, int ticks)
{
	ticks >>= 1;

	initScreenPointsGrid(transformedGridVertices);
	rotateVertices(objectAxesVertices, transformedAxesVertices, 3, ticks, 2 * ticks, 3 * ticks);
	generateAxesVertices(transformedAxesVertices);

	transformAndProjectAxesBoxDotsEffect();

	drawBoxLines(buffer, -1);

	drawBlobs(screenPointsGrid.v, screenPointsGrid.num, buffer);

	drawBoxLines(buffer, 1);
}

static void setVertexPos(int x, int y, int z, Vertex3D* v)
{
	v->x = x;
	v->y = y;
	v->z = z;
}

static void setElementCol(int c, Element* e)
{
	e->c = c;
}

static void setElementTexCoords(int u, int v, Element* e)
{
	e->u = u;
	e->v = v;
}


static void addIndexedTriangle(int p0, int p1, int p2)
{
	*currentIndexPtr++ = p0;
	*currentIndexPtr++ = p1;
	*currentIndexPtr++ = p2;
}

static Mesh3D* newMesh(int vNum, int iNum)
{
	Mesh3D* mesh = (Mesh3D*)malloc(sizeof(Mesh3D));

	mesh->verticesNum = vNum;
	mesh->indicesNum = iNum;
	mesh->vertex = (Vertex3D*)malloc(vNum * sizeof(Vertex3D));
	mesh->index = (int*)malloc(iNum * sizeof(int));
	mesh->element = (Element*)malloc(iNum * sizeof(Element));

	return mesh;
}

void freeMesh(Mesh3D* mesh)
{
	free(mesh->vertex);
	free(mesh->index);
	free(mesh->element);

	free(mesh);
}

Mesh3D* genMesh(int type, int length)
{
	int x, y, z;
	const int halfLength = length / 2;

	Mesh3D* mesh = 0;
	Vertex3D* v;
	Element* e;

	switch (type) {
		case GEN_OBJ_CUBE:
		{
			mesh = newMesh(8, 36);
			v = mesh->vertex;
			e = mesh->element;
			for (z = -1; z <= 1; z += 2) {
				for (y = -1; y <= 1; y += 2) {
					for (x = -1; x <= 1; x += 2) {
						setVertexPos(x * halfLength, y * halfLength, z * halfLength, v);
						setElementCol(rand() & 255, e);
						/* setVertexTexCoords() */
						++v;
						++e;
					}
				}
			}


			currentIndexPtr = mesh->index;

			/*
						6		7
					2		3
						4		5
					0		1
			*/

			addIndexedTriangle(0, 3, 2);
			addIndexedTriangle(0, 1, 3);
			addIndexedTriangle(1, 7, 3);
			addIndexedTriangle(1, 5, 7);
			addIndexedTriangle(5, 6, 7);
			addIndexedTriangle(5, 4, 6);
			addIndexedTriangle(4, 2, 6);
			addIndexedTriangle(4, 0, 2);
			addIndexedTriangle(2, 7, 6);
			addIndexedTriangle(2, 3, 7);
			addIndexedTriangle(4, 1, 0);
			addIndexedTriangle(4, 5, 1);
		}
		break;

		default:
			break;
	}

	return mesh;
}

void setObjectPos(int x, int y, int z, Object3D* obj)
{
	obj->pos.x = x;
	obj->pos.y = y;
	obj->pos.z = z;
}

void setObjectRot(int x, int y, int z, Object3D* obj)
{
	obj->rot.x = x;
	obj->rot.y = y;
	obj->rot.z = z;
}

static void setVerticesMaterial(Object3D* obj)
{
	const int count = obj->mesh->verticesNum;
	Element* src = obj->mesh->element;
	Vertex3D* dst = screenPointsObject.v;

	int i;
	for (i = 0; i < count; i++) {
		dst->c = src->c;
		dst->u = src->u;
		dst->v = src->v;
		++src;
		++dst;
	}
}

void transformObject3D(Object3D* obj)
{
	rotateVertices(obj->mesh->vertex, screenPointsObject.v, obj->mesh->verticesNum, obj->rot.x, obj->rot.y, obj->rot.z);
	translateAndProjectVertices(screenPointsObject.v, screenPointsObject.v, obj->mesh->verticesNum, obj->pos.x, obj->pos.y, obj->pos.z);
	setVerticesMaterial(obj);
}

void renderObject3D(Object3D* obj)
{
	renderPolygons(obj, screenPointsObject.v);
}

void initOptEngine(int maxPoints)
{
	screenPointsObject.v = (Vertex3D*)malloc(maxPoints * sizeof(Vertex3D));

	initOptRasterizer();
}

void freeOptEngine()
{
	free(screenPointsObject.v);

	freeOptRasterizer();
}