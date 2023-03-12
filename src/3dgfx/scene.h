#ifndef SCENE_H_
#define SCENE_H_

#include "mesh.h"
#include "goat3d.h"

struct g3d_node {
	char *name;
	struct g3d_mesh *mesh;
	struct g3d_node *parent;
	struct g3d_node *child, *next;

	float xform[16];
};

struct g3d_track {
	struct g3d_node *node;
	struct goat3d_track *trk;
};

struct g3d_anim {
	struct g3d_track pos, rot, scale;
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

void conv_goat3d_mesh(struct g3d_mesh *dstmesh, struct goat3d_mesh *srcmesh);
void conv_goat3d_anim(struct g3d_anim *dstanim, struct goat3d_anim *srcanim);

#endif	/* SCENE_H_ */
