#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mesh.h"
#include "3dgfx.h"

int load_mesh(struct g3d_mesh *mesh, const char *fname)
{
	return -1;	/* TODO */
}

static struct {
	struct g3d_vertex *varr;
	const float *xform;
} zsort_cls;

static int zsort_cmp(const void *aptr, const void *bptr)
{
	const float *m = zsort_cls.xform;

	const struct g3d_vertex *va = (const struct g3d_vertex*)aptr;
	const struct g3d_vertex *vb = (const struct g3d_vertex*)bptr;

	float za = m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	float zb = m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	++va;
	++vb;

	za += m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	zb += m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	return za - zb;
}

static int zsort_indexed_cmp(const void *aptr, const void *bptr)
{
	const int16_t *a = (const int16_t*)aptr;
	const int16_t *b = (const int16_t*)bptr;

	const float *m = zsort_cls.xform;

	const struct g3d_vertex *va = zsort_cls.varr + a[0];
	const struct g3d_vertex *vb = zsort_cls.varr + b[0];

	float za = m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	float zb = m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	va = zsort_cls.varr + a[2];
	vb = zsort_cls.varr + b[2];

	za += m[2] * va->x + m[6] * va->y + m[10] * va->z + m[14];
	zb += m[2] * vb->x + m[6] * vb->y + m[10] * vb->z + m[14];

	return za - zb;
}


void zsort_mesh(struct g3d_mesh *m)
{
	zsort_cls.varr = m->varr;
	zsort_cls.xform = g3d_get_matrix(G3D_MODELVIEW, 0);

	if(m->iarr) {
		int nfaces = m->icount / m->prim;
		qsort(m->iarr, nfaces, m->prim * sizeof *m->iarr, zsort_indexed_cmp);
	} else {
		int nfaces = m->vcount / m->prim;
		qsort(m->varr, nfaces, m->prim * sizeof *m->varr, zsort_cmp);
	}
}


void draw_mesh(struct g3d_mesh *mesh)
{
	if(mesh->iarr) {
		g3d_draw_indexed(mesh->prim, mesh->varr, mesh->vcount, mesh->iarr, mesh->icount);
	} else {
		g3d_draw(mesh->prim, mesh->varr, mesh->vcount);
	}
}


#define NORMAL(vp, x, y, z) do { vp->nx = x; vp->ny = y; vp->nz = z; } while(0)
#define COLOR(vp, cr, cg, cb) do { vp->r = cr; vp->g = cg; vp->b = cb; } while(0)
#define TEXCOORD(vp, tu, tv) do { vp->u = tu; vp->v = tv; } while(0)
#define VERTEX(vp, vx, vy, vz) \
	do { \
		vp->x = vx; vp->y = vy; vp->z = vz; vp->w = 1.0f; \
		++vp; \
	} while(0)

int gen_cube(struct g3d_mesh *mesh, float sz, int sub)
{
	struct g3d_vertex *vptr;
	float hsz = sz / 2.0;

	mesh->prim = G3D_QUADS;
	mesh->iarr = 0;
	mesh->icount = 0;

	mesh->vcount = 24;
	if(!(mesh->varr = malloc(mesh->vcount * sizeof *mesh->varr))) {
		return -1;
	}
	vptr = mesh->varr;

	/* -Z */
	NORMAL(vptr, 0, 0, -1);
	COLOR(vptr, 255, 0, 255);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	/* -Y */
	NORMAL(vptr, 0, -1, 0);
	COLOR(vptr, 0, 255, 255);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, -hsz, -hsz, hsz);
	/* -X */
	NORMAL(vptr, -1, 0, 0);
	COLOR(vptr, 255, 255, 0);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, -hsz, hsz);
	VERTEX(vptr, -hsz, hsz, hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	/* +X */
	NORMAL(vptr, 1, 0, 0);
	COLOR(vptr, 255, 0, 0);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	/* +Y */
	NORMAL(vptr, 0, 1, 0);
	COLOR(vptr, 0, 255, 0);
	VERTEX(vptr, -hsz, hsz, hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	/* +Z */
	NORMAL(vptr, 0, 0, 1);
	COLOR(vptr, 0, 0, 255);
	VERTEX(vptr, -hsz, -hsz, hsz);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	VERTEX(vptr, -hsz, hsz, hsz);

	return 0;
}

static void torusvec(float *res, float theta, float phi, float mr, float rr)
{
	float rx, ry, rz;
	theta = -theta;

	rx = -cos(phi) * rr + mr;
	ry = sin(phi) * rr;
	rz = 0.0f;

	res[0] = rx * sin(theta) + rz * cos(theta);
	res[1] = ry;
	res[2] = -rx * cos(theta) + rz * sin(theta);
}

int gen_torus(struct g3d_mesh *mesh, float rad, float ringrad, int usub, int vsub)
{
	int i, j;
	int nfaces, uverts, vverts;
	struct g3d_vertex *vptr;
	int16_t *iptr;

	mesh->prim = G3D_QUADS;

	if(usub < 4) usub = 4;
	if(vsub < 2) vsub = 2;

	uverts = usub + 1;
	vverts = vsub + 1;

	mesh->vcount = uverts * vverts;
	nfaces = usub * vsub;
	mesh->icount = nfaces * 4;

	printf("generating torus with %d faces (%d vertices)\n", nfaces, mesh->vcount);

	if(!(mesh->varr = malloc(mesh->vcount * sizeof *mesh->varr))) {
		return -1;
	}
	if(!(mesh->iarr = malloc(mesh->icount * sizeof *mesh->iarr))) {
		return -1;
	}
	vptr = mesh->varr;
	iptr = mesh->iarr;

	for(i=0; i<uverts; i++) {
		float u = (float)i / (float)(uverts - 1);
		float theta = u * 2.0 * M_PI;
		float rcent[3];

		torusvec(rcent, theta, 0, rad, 0);

		for(j=0; j<vverts; j++) {
			float v = (float)j / (float)(vverts - 1);
			float phi = v * 2.0 * M_PI;
			int chess = (i & 1) == (j & 1);

			torusvec(&vptr->x, theta, phi, rad, ringrad);

			vptr->nx = (vptr->x - rcent[0]) / ringrad;
			vptr->ny = (vptr->y - rcent[1]) / ringrad;
			vptr->nz = (vptr->z - rcent[2]) / ringrad;
			vptr->u = u;
			vptr->v = v;
			vptr->r = chess ? 255 : 64;
			vptr->g = 128;
			vptr->b = chess ? 64 : 255;
			++vptr;

			if(i < usub && j < vsub) {
				int idx = i * vverts + j;
				*iptr++ = idx;
				*iptr++ = idx + 1;
				*iptr++ = idx + vverts + 1;
				*iptr++ = idx + vverts;
			}
		}
	}
	return 0;
}

