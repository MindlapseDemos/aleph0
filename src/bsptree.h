#ifndef BSPMESH_H_
#define BSPMESH_H_

#include "mesh.h"
#include "vmath.h"
#include "polyclip.h"

struct bsppoly {
	struct cplane plane;
	int vcount;
	struct g3d_vertex *verts;
};

struct bspnode {
	struct bsppoly poly;
	struct bspnode *front, *back;
};

struct bsptree {
	struct bspnode *root;
	struct bsppoly *soup;	/* dynarr: see dynarr.h */
};

int init_bsp(struct bsptree *bsp);
void destroy_bsp(struct bsptree *bsp);

int save_bsp(struct bsptree *bsp, const char *fname);
int load_bsp(struct bsptree *bsp, const char *fname);

int bsp_add_poly(struct bsptree *bsp, struct g3d_vertex *v, int vnum);
int bsp_add_mesh(struct bsptree *bsp, struct g3d_mesh *m);

int bsp_build(struct bsptree *bsp);

void draw_bsp(struct bsptree *bsp, float view_x, float view_y, float view_z);

#endif	/* BSPMESH_H_ */
