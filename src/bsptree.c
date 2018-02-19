#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "bsptree.h"
#include "dynarr.h"
#include "inttypes.h"
#include "polyclip.h"
#include "vmath.h"

struct bspfile_header {
	char magic[4];
	uint32_t num_nodes;
};

static int count_nodes(struct bspnode *n);
static void free_tree(struct bspnode *n);

static struct bspnode *new_node(struct g3d_vertex *v, int vnum);
static struct bspnode *add_poly_tree(struct bspnode *n, struct g3d_vertex *v, int vnum);
static void draw_bsp_tree(struct bspnode *n, const vec3_t *vdir);

static void save_bsp_tree(struct bspnode *n, FILE *fp);
static struct bspnode *load_bsp_tree(FILE *fp);

int init_bsp(struct bsptree *bsp)
{
	bsp->root = 0;
	return 0;
}

void destroy_bsp(struct bsptree *bsp)
{
	free_tree(bsp->root);
}

int save_bsp(struct bsptree *bsp, const char *fname)
{
	FILE *fp;
	struct bspfile_header hdr;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "save_bsp: failed to open %s for writing\n", fname);
		return -1;
	}

	memcpy(hdr.magic, "MBSP", 4);
	hdr.num_nodes = count_nodes(bsp->root);
	fwrite(&hdr, sizeof hdr, 1, fp);

	save_bsp_tree(bsp->root, fp);

	fclose(fp);
	return 0;
}

int load_bsp(struct bsptree *bsp, const char *fname)
{
	FILE *fp;
	struct bspfile_header hdr;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "load_bsp: failed to open %s\n", fname);
		return -1;
	}

	if(fread(&hdr, sizeof hdr, 1, fp) < 1) {
		fprintf(stderr, "load_bsp: %s: failed to read header\n", fname);
		fclose(fp);
		return -1;
	}
	if(memcmp(hdr.magic, "MBSP", 4) != 0) {
		fprintf(stderr, "load_bsp: %s: invalid magic\n", fname);
		fclose(fp);
		return -1;
	}
	bsp->root = load_bsp_tree(fp);

	fclose(fp);
	return 0;
}

int bsp_add_poly(struct bsptree *bsp, struct g3d_vertex *v, int vnum)
{
	struct bspnode *n;
	assert(vnum >= 3);

	if(!(n = add_poly_tree(bsp->root, v, vnum))) {
		fprintf(stderr, "bsp_add_poly: failed to add polygon\n");
		return -1;
	}
	bsp->root = n;
	return 0;
}

int bsp_add_mesh(struct bsptree *bsp, struct g3d_mesh *m)
{
	int i, j, nfaces;
	struct g3d_vertex v[4];
	struct g3d_vertex *vptr = m->varr;
	uint16_t *iptr = m->iarr;

	nfaces = m->iarr ? m->icount / m->prim : m->vcount / m->prim;

	for(i=0; i<nfaces; i++) {
		for(j=0; j<m->prim; j++) {
			if(m->iarr) {
				v[j] = m->varr[*iptr++];
			} else {
				v[j] = *vptr++;
			}
		}
		if(bsp_add_poly(bsp, v, m->prim) == -1) {
			return -1;
		}
	}
	return 0;
}

void draw_bsp(struct bsptree *bsp, float view_x, float view_y, float view_z)
{
	vec3_t vdir;
	vdir.x = view_x;
	vdir.y = view_y;
	vdir.z = view_z;
	draw_bsp_tree(bsp->root, &vdir);
}

static int count_nodes(struct bspnode *n)
{
	if(!n) return 0;
	return count_nodes(n->front) + count_nodes(n->back) + 1;
}

static void free_tree(struct bspnode *n)
{
	if(n) {
		free_tree(n->front);
		free_tree(n->back);
		free(n);
	}
}


static struct bspnode *new_node(struct g3d_vertex *v, int vnum)
{
	struct bspnode *n;
	vec3_t va, vb, norm;

	if(!(n = malloc(sizeof *n)) || !(n->verts = malloc(vnum * sizeof *n->verts))) {
		fprintf(stderr, "failed to allocate BSP tree node\n");
		free(n);
		return 0;
	}
	va.x = v[1].x - v[0].x;
	va.y = v[1].y - v[0].y;
	va.z = v[1].z - v[0].z;
	vb.x = v[2].x - v[0].x;
	vb.y = v[2].y - v[0].y;
	vb.z = v[2].z - v[0].z;
	norm = v3_cross(va, vb);
	v3_normalize(&norm);

	n->plane.x = v[0].x;
	n->plane.y = v[0].y;
	n->plane.z = v[0].z;
	n->plane.nx = norm.x;
	n->plane.ny = norm.y;
	n->plane.nz = norm.z;

	n->vcount = vnum;
	memcpy(n->verts, v, vnum * sizeof *n->verts);

	n->front = n->back = 0;
	return n;
}

static struct bspnode *add_poly_tree(struct bspnode *n, struct g3d_vertex *v, int vnum)
{
	struct bspnode *nres;
	int clipres, clipres_neg, clipped_vnum, clipped_neg_vnum;
	struct g3d_vertex *clipped, *clipped_neg;
	struct cplane negplane;

	assert(vnum > 0);

	if(!n) {
		return new_node(v, vnum);
	}

	negplane.x = n->plane.x;
	negplane.y = n->plane.y;
	negplane.z = n->plane.z;
	negplane.nx = -n->plane.nx;
	negplane.ny = -n->plane.ny;
	negplane.nz = -n->plane.nz;

	clipped = alloca((vnum * 2) * sizeof *clipped);
	clipped_neg = alloca((vnum * 2) * sizeof *clipped_neg);

	clipres = clip_poly(clipped, &clipped_vnum, v, vnum, &n->plane);
	clipres_neg = clip_poly(clipped_neg, &clipped_neg_vnum, v, vnum, &negplane);

	/* detect edge cases where due to floating point imprecision, clipping
	 * by the positive plane clips the polygon, but clipping by the negative
	 * plane doesn't. If that happens, consider the polygon completely on
	 * the side indicated by -clipres_neg
	 */
	if(clipres == 0 && clipres_neg != 0) {
		clipres = -clipres_neg;
	}

	if(clipres > 0) {
		/* polygon completely in the positive subspace */
		if(!(nres = add_poly_tree(n->front, v, vnum))) {
			return 0;
		}
		n->front = nres;

	} else if(clipres < 0) {
		/* polygon completely in the negative subspace */
		if(!(nres = add_poly_tree(n->back, v, vnum))) {
			return 0;
		}
		n->back = nres;

	} else {
		/* polygon is straddling the plane */
		if(!(nres = add_poly_tree(n->front, clipped, clipped_vnum))) {
			return 0;
		}
		n->front = nres;

		if(!(nres = add_poly_tree(n->back, clipped_neg, clipped_neg_vnum))) {
			return 0;
		}
		n->back = nres;
	}
	return n;
}

#undef DRAW_NGONS

#ifndef DRAW_NGONS
static void debug_draw_poly(struct g3d_vertex *varr, int vcount)
{
	int i, nfaces = vcount - 2;
	int vbuf_size = nfaces * 3;
	struct g3d_vertex *vbuf = alloca(vbuf_size * sizeof *vbuf);
	struct g3d_vertex *vptr = varr + 1;

	for(i=0; i<nfaces; i++) {
		vbuf[i * 3] = varr[0];
		vbuf[i * 3 + 1] = *vptr++;
		vbuf[i * 3 + 2] = *vptr;
	}

	g3d_draw_indexed(G3D_TRIANGLES, vbuf, vbuf_size, 0, 0);
}
#endif

static void draw_bsp_tree(struct bspnode *n, const vec3_t *vdir)
{
	float dot;

	if(!n) return;

	dot = vdir->x * n->plane.nx + vdir->y * n->plane.ny + vdir->z * n->plane.nz;
	if(dot >= 0.0f) {
		draw_bsp_tree(n->front, vdir);
#ifdef DRAW_NGONS
		g3d_draw_indexed(n->vcount, n->verts, n->vcount, 0, 0);
#else
		debug_draw_poly(n->verts, n->vcount);
#endif
		draw_bsp_tree(n->back, vdir);
	} else {
		draw_bsp_tree(n->back, vdir);
#ifdef DRAW_NGONS
		g3d_draw_indexed(n->vcount, n->verts, n->vcount, 0, 0);
#else
		debug_draw_poly(n->verts, n->vcount);
#endif
		draw_bsp_tree(n->front, vdir);
	}
}

static void save_bsp_tree(struct bspnode *n, FILE *fp)
{
	/* TODO */
}

static struct bspnode *load_bsp_tree(FILE *fp)
{
	return 0;	/* TODO */
}
