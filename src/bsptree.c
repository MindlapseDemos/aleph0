#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__DJGPP__)
#include <malloc.h>
#else
#ifdef WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif
#endif
#include "bsptree.h"
#include "dynarr.h"
#include "inttypes.h"
#include "polyclip.h"
#include "vmath.h"
#include "util.h"

struct bspfile_header {
	char magic[4];
	uint32_t num_nodes;
};

static int count_nodes(struct bspnode *n);
static void free_tree(struct bspnode *n);

static int init_poly(struct bsppoly *poly, struct g3d_vertex *v, int vnum);
static int init_poly_noplane(struct bsppoly *poly, struct g3d_vertex *v, int vnum);

static struct bspnode *build_tree(struct bsppoly *polyarr, int num_polys);

static void draw_bsp_tree(struct bspnode *n, const vec3_t *vdir);

static void save_bsp_tree(struct bspnode *n, FILE *fp);
static struct bspnode *load_bsp_tree(FILE *fp);

int init_bsp(struct bsptree *bsp)
{
	bsp->root = 0;
	bsp->soup = 0;
	return 0;
}

void destroy_bsp(struct bsptree *bsp)
{
	int i;

	free_tree(bsp->root);

	if(bsp->soup) {
		for(i=0; i<dynarr_size(bsp->soup); i++) {
			free(bsp->soup[i].verts);
		}
		dynarr_free(bsp->soup);
	}
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
	struct bsppoly poly, *tmp;

	if(!bsp->soup && !(bsp->soup = dynarr_alloc(0, sizeof *bsp->soup))) {
		fprintf(stderr, "bsp_add_poly: failed to create polygon soup dynamic array\n");
		return -1;
	}

	if(init_poly(&poly, v, vnum) == -1) {
		return -1;
	}

	if(!(tmp = dynarr_push(bsp->soup, &poly))) {
		fprintf(stderr, "bsp_add_poly: failed to reallocate polygon soup\n");
		free(poly.verts);
		return -1;
	}
	bsp->soup = tmp;

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

int bsp_build(struct bsptree *bsp)
{
	assert(bsp->soup);
	assert(!dynarr_empty(bsp->soup));

	free_tree(bsp->root);

	printf("building BSP tree with %d source polygons ...\n", dynarr_size(bsp->soup));
	if(!(bsp->root = build_tree(bsp->soup, dynarr_size(bsp->soup)))) {
		fprintf(stderr, "bsp_build failed\n");
		return -1;
	}
	printf("done. created a tree with %d nodes\n", count_nodes(bsp->root));

	/* build_tree has called dynarr_free on bsp->soup */
	bsp->soup = 0;

	return 0;
}

void draw_bsp(struct bsptree *bsp, float view_x, float view_y, float view_z)
{
	vec3_t vdir;

	assert(bsp->root);

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
		free(n->poly.verts);
		free(n);
	}
}


static int init_poly(struct bsppoly *poly, struct g3d_vertex *v, int vnum)
{
	vec3_t va, vb, norm;

	if(init_poly_noplane(poly, v, vnum) == -1) {
		return -1;
	}

	va.x = v[1].x - v[0].x;
	va.y = v[1].y - v[0].y;
	va.z = v[1].z - v[0].z;
	vb.x = v[2].x - v[0].x;
	vb.y = v[2].y - v[0].y;
	vb.z = v[2].z - v[0].z;
	norm = v3_cross(va, vb);
	v3_normalize(&norm);

	poly->plane.x = v[0].x;
	poly->plane.y = v[0].y;
	poly->plane.z = v[0].z;
	poly->plane.nx = norm.x;
	poly->plane.ny = norm.y;
	poly->plane.nz = norm.z;
	return 0;
}

static int init_poly_noplane(struct bsppoly *poly, struct g3d_vertex *v, int vnum)
{
	if(!(poly->verts = malloc(vnum * sizeof *poly->verts))) {
		fprintf(stderr, "failed to allocate BSP polygon\n");
		return -1;
	}
	poly->vcount = vnum;
	memcpy(poly->verts, v, vnum * sizeof *poly->verts);
	return 0;
}

static int choose_poly(struct bsppoly *polyarr, int num_polys)
{
	int i, j, best, best_splits;

	if(num_polys <= 1) {
		return 0;
	}

	best = -1;
	best_splits = INT_MAX;

	for(i=0; i<num_polys; i++) {
		struct cplane *plane = &polyarr[i].plane;
		int num_splits = 0;

#ifdef USE_OPENMP
#pragma omp parallel for reduction(+:num_splits)
#endif
		for(j=0; j<num_polys; j++) {
			if(i == j) continue;

			if(check_clip_poly(polyarr[j].verts, polyarr[j].vcount, plane) == 0) {
				num_splits++;
			}
		}

		if(num_splits < best_splits) {
			best_splits = num_splits;
			best = i;
		}
	}

	/*printf("choose_poly(..., %d) -> %d splits\n", num_polys, best_splits);*/

	return best;
}

static struct bspnode *build_tree(struct bsppoly *polyarr, int num_polys)
{
	int i, pidx, vnum;
	struct bsppoly *sp, *tmp;
	struct bsppoly *front_polys, *back_polys;
	struct bspnode *node;

	int clipres, clipres_neg, clipped_vnum, clipped_neg_vnum, max_clipped_vnum = 0;
	struct g3d_vertex *v, *clipped = 0, *clipped_neg = 0;
	struct cplane negplane;

	if((pidx = choose_poly(polyarr, num_polys)) == -1) {
		return 0;
	}
	sp = polyarr + pidx;

	negplane.x = sp->plane.x;
	negplane.y = sp->plane.y;
	negplane.z = sp->plane.z;
	negplane.nx = -sp->plane.nx;
	negplane.ny = -sp->plane.ny;
	negplane.nz = -sp->plane.nz;

	if(!(front_polys = dynarr_alloc(0, sizeof *front_polys)) ||
			!(back_polys = dynarr_alloc(0, sizeof *back_polys))) {
		fprintf(stderr, "build_tree: failed to allocate front/back polygon arrays\n");
		dynarr_free(front_polys);
		return 0;
	}

	for(i=0; i<num_polys; i++) {
		if(i == pidx) continue;

		vnum = polyarr[i].vcount;

		if(vnum * 2 > max_clipped_vnum) {
			/* resize clipped polygon buffers if necessary */
			max_clipped_vnum = vnum * 2;
			if(!(v = realloc(clipped, max_clipped_vnum * sizeof *clipped))) {
				fprintf(stderr, "build_tree: failed to reallocate clipped polygon buffer\n");
				goto fail;
			}
			clipped = v;
			if(!(v = realloc(clipped_neg, max_clipped_vnum * sizeof *clipped))) {
				fprintf(stderr, "build_tree: failed to reallocate clipped polygon buffer\n");
				goto fail;
			}
			clipped_neg = v;
		}

		v = polyarr[i].verts;

		clipres = clip_poly(clipped, &clipped_vnum, v, vnum, &sp->plane);
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
			if(!(tmp = dynarr_push(front_polys, polyarr + i))) {
				fprintf(stderr, "build_tree: failed to reallocate polygon array\n");
				goto fail;
			}
			front_polys = tmp;

		} else if(clipres < 0) {
			/* polygon completely in the negative subspace */
			if(!(tmp = dynarr_push(back_polys, polyarr + i))) {
				fprintf(stderr, "build_tree: failed to reallocate polygon array\n");
				goto fail;
			}
			back_polys = tmp;

		} else {
			/* polygon is straddling the plane */
			struct bsppoly poly;
			poly.plane = polyarr[i].plane;

			if(init_poly_noplane(&poly, clipped, clipped_vnum) == -1) {
				goto fail;
			}
			if(!(tmp = dynarr_push(front_polys, &poly))) {
				fprintf(stderr, "build_tree: failed to reallocate polygon array\n");
				free(poly.verts);
				goto fail;
			}
			front_polys = tmp;

			if(init_poly_noplane(&poly, clipped_neg, clipped_neg_vnum) == -1) {
				goto fail;
			}
			if(!(tmp = dynarr_push(back_polys, &poly))) {
				fprintf(stderr, "build_tree: failed to reallocate polygon array\n");
				free(poly.verts);
				goto fail;
			}
			back_polys = tmp;

			/* we allocated new sub-polygons, so we need to free the original vertex array */
			free(polyarr[i].verts);
		}
	}

	if(!(node = malloc(sizeof *node))) {
		fprintf(stderr, "build_tree: failed to allocate new BSP node\n");
		goto fail;
	}
	node->poly = *sp;
	node->front = node->back = 0;

	if(dynarr_size(front_polys)) {
		if(!(node->front = build_tree(front_polys, dynarr_size(front_polys)))) {
			goto fail;
		}
	}
	if(dynarr_size(back_polys)) {
		if(!(node->back = build_tree(back_polys, dynarr_size(back_polys)))) {
			goto fail;
		}
	}

	free(clipped);
	free(clipped_neg);

	/* we no longer need the original polygon array */
	dynarr_free(polyarr);

	return node;

fail:
	free(clipped);
	free(clipped_neg);

	for(i=0; i<dynarr_size(front_polys); i++) {
		free(front_polys[i].verts);
	}
	dynarr_free(front_polys);

	for(i=0; i<dynarr_size(back_polys); i++) {
		free(back_polys[i].verts);
	}
	dynarr_free(back_polys);

	return 0;
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
	struct bsppoly *p;

	if(!n) return;

	p = &n->poly;

	dot = vdir->x * p->plane.nx + vdir->y * p->plane.ny + vdir->z * p->plane.nz;
	if(dot >= 0.0f) {
		draw_bsp_tree(n->front, vdir);
#ifdef DRAW_NGONS
		g3d_draw_indexed(p->vcount, p->verts, p->vcount, 0, 0);
#else
		debug_draw_poly(p->verts, p->vcount);
#endif
		draw_bsp_tree(n->back, vdir);
	} else {
		draw_bsp_tree(n->back, vdir);
#ifdef DRAW_NGONS
		g3d_draw_indexed(p->vcount, p->verts, p->vcount, 0, 0);
#else
		debug_draw_poly(p->verts, p->vcount);
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
