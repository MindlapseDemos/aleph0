#ifndef MESH_H_
#define MESH_H_

struct g3d_mesh {
	int prim;
	struct g3d_vertex *varr;
	int16_t *iarr;
	int vcount, icount;
};

int load_mesh(struct g3d_mesh *mesh, const char *fname);

void zsort_mesh(struct g3d_mesh *mesh);
void draw_mesh(struct g3d_mesh *mesh);

int gen_cube(struct g3d_mesh *mesh, float sz, int sub);
int gen_torus(struct g3d_mesh *mesh, float rad, float ringrad, int usub, int vsub);

#endif	/* MESH_H_ */
