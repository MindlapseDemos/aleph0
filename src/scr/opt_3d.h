#ifndef OPT_3D_H
#define OPT_3D_H

#define VERTICES_WIDTH 32
#define VERTICES_HEIGHT 32
#define VERTICES_DEPTH 32
#define MAX_VERTEX_ELEMENTS_NUM (VERTICES_WIDTH * VERTICES_HEIGHT * VERTICES_DEPTH)
#define SET_OPT_VERTEX(xp,yp,zp,v) v->x = (xp); v->y = (yp); v->z = (zp);


typedef struct Vertex3D
{
	int x,y,z;
}Vertex3D;

typedef struct ScreenPoints
{
	Vertex3D *v;
	int num;
}ScreenPoints;


void Opt3Dinit(void);
void Opt3Dfree(void);
void Opt3Drun(int ticks);

unsigned char *getDotsVolumeBuffer();

#endif
