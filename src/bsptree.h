#ifndef BSPMESH_H_
#define BSPMESH_H_

#include "mesh.h"
#include "vmath.h"
#include "polyclip.h"

struct bspnode {
	struct cplane plane;
	int vcount;
	struct g3d_vertex *verts;
	struct bspnode *front, *back;
};

struct bsptree {
	struct bspnode *root;
};

int init_bsp(struct bsptree *bsp);
void destroy_bsp(struct bsptree *bsp);

int save_bsp(struct bsptree *bsp, const char *fname);
int load_bsp(struct bsptree *bsp, const char *fname);

int bsp_add_poly(struct bsptree *bsp, struct g3d_vertex *v, int vnum);
void bsp_add_mesh(struct bsptree *bsp, struct g3d_mesh *m);

void draw_bsp(struct bsptree *bsp, float view_x, float view_y, float view_z);

#endif	/* BSPMESH_H_ */
