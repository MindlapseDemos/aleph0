#ifndef OPT_3D_H
#define OPT_3D_H

#define FIXED_TEST

#define VERTICES_WIDTH 32
#define VERTICES_HEIGHT 32
#define VERTICES_DEPTH 32
#define MAX_VERTEX_ELEMENTS_NUM (VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH)
#define SET_OPT_VERTEX(xp,yp,zp,v) v->x = (xp); v->y = (yp); v->z = (zp);


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


void Opt3DinitPerfTest(void);
void Opt3DfreePerfTest(void);
void Opt3DrunPerfTest(int ticks);

unsigned char *getDotsVolumeBuffer();

#endif
