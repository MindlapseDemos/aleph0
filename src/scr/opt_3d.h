#ifndef OPT_3D_H
#define OPT_3D_H

#define FLOAT_TO_FIXED(f,b) ((int)((f) * (1 << (b))))
#define INT_TO_FIXED(i,b) ((i) * (1 << (b)))
#define UINT_TO_FIXED(i,b) ((i) << (b))
#define FIXED_TO_INT(x,b) ((x) >> (b))
#define FIXED_TO_FLOAT(x,b) ((float)(x) / (1 << (b)))
#define FIXED_MUL(x,y,b) (((x) * (y)) >> (b))
#define FIXED_DIV(x,y,b) (((x) << (b)) / (y))
#define FIXED_SQRT(x,b) (isqrt((x) << (b)))

#define AFTER_MUL_ADDS(x,b)		FIXED_TO_INT(x,b)
#define AFTER_RECZ_MUL(x,b)		FIXED_TO_INT(x,b)

#define DEG_TO_RAD_256(x) (((2 * M_PI) * (x)) / 256)
#define RAD_TO_DEG_256(x) ((256 * (x)) / (2 * M_PI))

#define REC_DIV_Z_MAX 2048

#define FP_CORE 16
#define FP_BASE 12
#define FP_BASE_TO_CORE (FP_CORE - FP_BASE)
#define FP_NORM 8

#define PROJ_SHR 8
#define PROJ_MUL (1 << PROJ_SHR)

#define D2R (180.0 / M_PI)



enum {
	GEN_OBJ_CUBE,
	GEN_OBJ_SPHERICAL
};


typedef struct Vector3D
{
	int x, y, z;
}Vector3D;


typedef struct Vertex3D
{
	int xs,ys;
	int x,y,z;
	unsigned char c, a, u, v;
}Vertex3D;

typedef struct Element
{
	unsigned char c, a, u, v;
}Element;

typedef struct ScreenPoints
{
	Vertex3D *v;
	Vector3D* vNormals;
	int num;
}ScreenPoints;

typedef struct Mesh3D
{
	int verticesNum;
	int indicesNum;

	Vertex3D* vertex;
	Vector3D* vNormal;
	Vector3D* pNormal;

	int* index;
	Element* element;
}Mesh3D;

typedef struct Object3D
{
	Mesh3D* mesh;
	Vector3D pos, rot;
}Object3D;

int isqrt(int x);

Mesh3D* genMesh(int type, int length, float param1);
void freeMesh(Mesh3D* mesh);

void setObjectPos(int x, int y, int z, Object3D* obj);
void setObjectRot(int x, int y, int z, Object3D* obj);

void transformObject3D(Object3D* obj);
void renderObject3D(Object3D* obj);
void rotateVertices(Vertex3D* src, Vertex3D* dst, int count, int rx, int ry, int rz);

ScreenPoints* getObjectScreenPoints();

void initOptEngine(int maxPoints);
void freeOptEngine();

#endif
