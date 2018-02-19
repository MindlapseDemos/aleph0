#include <string.h>
#include <GL/gl.h>
#include "3dgfx.h"

void g3d_matrix_mode(int mmode)
{
	switch(mmode) {
	case G3D_MODELVIEW:
		glMatrixMode(GL_MODELVIEW);
		break;
	case G3D_PROJECTION:
		glMatrixMode(GL_PROJECTION);
		break;
	}
}

void g3d_push_matrix(void)
{
	glPushMatrix();
}

void g3d_pop_matrix(void)
{
	glPopMatrix();
}

void g3d_load_identity(void)
{
	glLoadIdentity();
}

const float *g3d_get_matrix(int which, float *mret)
{
	static float tmp[16];

	switch(which) {
	case G3D_MODELVIEW:
		glGetFloatv(GL_MODELVIEW_MATRIX, tmp);
		break;

	case G3D_PROJECTION:
		glGetFloatv(GL_PROJECTION_MATRIX, tmp);
		break;
	}

	if(mret) {
		memcpy(mret, tmp, sizeof tmp);
	}
	return tmp;
}

void g3d_translate(float x, float y, float z)
{
	glTranslatef(x, y, z);
}

void g3d_rotate(float angle, float x, float y, float z)
{
	glRotatef(angle, x, y, z);
}

void g3d_scale(float x, float y, float z)
{
	glScalef(x, y, z);
}

void g3d_draw(int prim, const struct g3d_vertex *varr, int varr_size)
{
	g3d_draw_indexed(prim, varr, varr_size, 0, 0);
}

void g3d_draw_indexed(int prim, const struct g3d_vertex *varr, int varr_size,
		const uint16_t *iarr, int iarr_size)
{
	int i, j, nfaces;
	int glprim;
	const struct g3d_vertex *vptr = varr;
	const uint16_t *iptr = iarr;

	switch(prim) {
	case G3D_TRIANGLES:
		glprim = GL_TRIANGLES;
		break;
	case G3D_QUADS:
		glprim = GL_QUADS;
		break;
	default:
		glprim = GL_POLYGON;
	}

	nfaces = (iarr ? iarr_size : varr_size) / prim;
	if(prim <= 4) {
		glBegin(glprim);
	}

	for(i=0; i<nfaces; i++) {
		if(glprim == GL_POLYGON) {
			glBegin(GL_POLYGON);
		}

		for(j=0; j<prim; j++) {
			if(iarr) {
				vptr = varr + *iptr++;
			}
			glNormal3f(vptr->nx, vptr->ny, vptr->nz);
			glColor4b(vptr->r, vptr->g, vptr->b, vptr->a);
			glTexCoord2f(vptr->u, vptr->v);
			glVertex4f(vptr->x, vptr->y, vptr->z, vptr->w);
			++vptr;
		}

		if(glprim == GL_POLYGON) {
			glEnd();
		}
	}

	if(prim <= 4) {
		glEnd();
	}
}
