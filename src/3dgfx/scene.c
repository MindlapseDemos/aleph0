#include <string.h>
#include "scene.h"
#include "darray.h"
#include "util.h"
#include "goat3d.h"
#include "cgmath/cgmath.h"

void scn_init(struct g3d_scene *scn)
{
	scn->meshes = darr_alloc(0, sizeof *scn->meshes);
	scn->nodes = darr_alloc(0, sizeof *scn->nodes);
	scn->anims = darr_alloc(0, sizeof *scn->anims);
}

void scn_destroy(struct g3d_scene *scn)
{
	scn_clear(scn);
	darr_free(scn->meshes);
	darr_free(scn->nodes);
	darr_free(scn->anims);
}

struct g3d_scene *scn_create(void)
{
	struct g3d_scene *scn = malloc_nf(sizeof *scn);
	scn_init(scn);
	return scn;
}

void scn_free(struct g3d_scene *scn)
{
	if(!scn) return;
	scn_destroy(scn);
	free(scn);
}

void scn_clear(struct g3d_scene *scn)
{
	int i, num;

	num = darr_size(scn->meshes);
	for(i=0; i<num; i++) {
		free_mesh(scn->meshes[i]);
	}
	darr_clear(scn->meshes);

	num = darr_size(scn->nodes);
	for(i=0; i<num; i++) {
		scn_free_node(scn->nodes[i]);
	}
	darr_clear(scn->nodes);

	num = darr_size(scn->anims);
	for(i=0; i<num; i++) {
		scn_free_anim(scn->anims[i]);
	}
	darr_clear(scn->anims);
}

void scn_init_node(struct g3d_node *node)
{
	memset(node, 0, sizeof *node);
	cgm_midentity(node->xform);
}

void scn_destroy_node(struct g3d_node *node)
{
	free(node->name);
}

void scn_free_node(struct g3d_node *node)
{
	if(!node) return;
	scn_destroy_node(node);
	free(node);
}

void scn_init_anim(struct g3d_anim *anim)
{
	scn_init_track(&anim->pos);
	scn_init_track(&anim->rot);
	scn_init_track(&anim->scale);
}

void scn_destroy_anim(struct g3d_anim *anim)
{
	scn_destroy_track(&anim->pos);
	scn_destroy_track(&anim->rot);
	scn_destroy_track(&anim->scale);
}

void scn_free_anim(struct g3d_anim *anim)
{
	if(!anim) return;
	scn_destroy_anim(anim);
	free(anim);
}

void scn_init_track(struct g3d_track *track)
{
	track->trk = goat3d_create_track();
}

void scn_destroy_track(struct g3d_track *track)
{
	goat3d_destroy_track(track->trk);
}

void scn_add_mesh(struct g3d_scene *scn, struct g3d_mesh *mesh)
{
	darr_push(scn->meshes, &mesh);
}

void scn_add_node(struct g3d_scene *scn, struct g3d_node *node)
{
	darr_push(scn->nodes, &node);
}

void scn_add_anim(struct g3d_scene *scn, struct g3d_anim *anim)
{
	darr_push(scn->anims, &anim);
}

int scn_mesh_count(struct g3d_scene *scn)
{
	return darr_size(scn->meshes);
}

int scn_node_count(struct g3d_scene *scn)
{
	return darr_size(scn->nodes);
}

int scn_anim_count(struct g3d_scene *scn)
{
	return darr_size(scn->anims);
}
