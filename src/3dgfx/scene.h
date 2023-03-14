#ifndef SCENE_H_
#define SCENE_H_

#include "cgmath/cgmath.h"
#include "mesh.h"
#include "goat3d.h"

struct g3d_node {
	char *name;
	struct g3d_mesh *mesh;
	struct g3d_node *parent;
	struct g3d_node *child, *next;

	cgm_vec3 pos;
	cgm_quat rot;
	cgm_vec3 scale;
	cgm_vec3 pivot;

	float xform[16];
	int xform_valid;
};

struct g3d_track {
	struct g3d_node *node;
	struct goat3d_track *trk;
};

struct g3d_anim {
	char *name;
	struct g3d_track **tracks;	/* darray */
	long start, end, dur;
};

struct g3d_scene {
	/* dynamic arrays (darray) */
	struct g3d_mesh **meshes;
	struct g3d_node **nodes;
	struct g3d_anim **anims;
};

void scn_init(struct g3d_scene *scn);
void scn_destroy(struct g3d_scene *scn);

struct g3d_scene *scn_create(void);
void scn_free(struct g3d_scene *scn);

void scn_clear(struct g3d_scene *scn);

void scn_init_node(struct g3d_node *node);
void scn_destroy_node(struct g3d_node *node);
void scn_free_node(struct g3d_node *node);

void scn_init_anim(struct g3d_anim *anim);
void scn_destroy_anim(struct g3d_anim *anim);
void scn_free_anim(struct g3d_anim *anim);

void scn_init_track(struct g3d_track *track);
void scn_destroy_track(struct g3d_track *track);

void scn_add_mesh(struct g3d_scene *scn, struct g3d_mesh *mesh);
void scn_add_node(struct g3d_scene *scn, struct g3d_node *node);
void scn_add_anim(struct g3d_scene *scn, struct g3d_anim *anim);

int scn_mesh_count(struct g3d_scene *scn);
int scn_node_count(struct g3d_scene *scn);
int scn_anim_count(struct g3d_scene *scn);

struct g3d_mesh *scn_find_mesh(struct g3d_scene *scn, const char *name);
struct g3d_node *scn_find_node(struct g3d_scene *scn, const char *name);
struct g3d_anim *scn_find_anim(struct g3d_scene *scn, const char *name);

int conv_goat3d_scene(struct g3d_scene *scn, struct goat3d *g);
int conv_goat3d_mesh(struct g3d_mesh *dstmesh, struct goat3d_mesh *srcmesh);
int conv_goat3d_node(struct g3d_scene *scn, struct g3d_node *dstnode, struct goat3d_node *srcnode);
int link_goat3d_node(struct g3d_scene *scn, struct g3d_node *dstnode, struct goat3d_node *srcnode);
int conv_goat3d_anim(struct g3d_scene *scn, struct g3d_anim *dstanim, struct goat3d_anim *srcanim);
int conv_goat3d_track(struct g3d_scene *scn, struct g3d_track *dsttrk, struct goat3d_track *srctrk);

void scn_eval_anim(struct g3d_anim *anim, long tm);
void scn_draw(struct g3d_scene *scn);

#endif	/* SCENE_H_ */
