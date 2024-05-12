#ifndef OPT_3D_H
#define OPT_3D_H

#define VERTICES_WIDTH 32
#define VERTICES_HEIGHT 32
#define VERTICES_DEPTH 32
#define MAX_VERTEX_ELEMENTS_NUM (VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH)
#define SET_OPT_VERTEX(xp,yp,zp,v) v->x = (xp); v->y = (yp); v->z = (zp);


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


enum {
	GEN_OBJ_CUBE
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
	int num;
}ScreenPoints;

typedef struct Mesh3D
{
	Vertex3D* vertex;
	int verticesNum;

	int* index;
	Element* element;
	int indicesNum;
}Mesh3D;

typedef struct Object3D
{
	Mesh3D* mesh;
	Vector3D pos, rot;
}Object3D;


void Opt3Dinit(void);
void Opt3Dfree(void);
void Opt3Drun(unsigned char *buffer, int ticks);

unsigned char *getDotsVolumeBuffer();
void drawBoxLines();

int isqrt(int x);

Mesh3D* genMesh(int type, int length);
void freeMesh(Mesh3D* mesh);

void setObjectPos(int x, int y, int z, Object3D* obj);
void setObjectRot(int x, int y, int z, Object3D* obj);

void transformObject3D(Object3D* obj);
void renderObject3D(Object3D* obj);

void initOptEngine(int maxPoints);
void freeOptEngine();

#endif
