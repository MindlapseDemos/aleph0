/*
goat3d - 3D scene, and animation file format library.
Copyright (C) 2013-2023  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <string.h>
#include "g3dscn.h"
#include "log.h"
#include "dynarr.h"
#include "treestor.h"

/* type passed to namegen */
enum { MTL, MESH, LIGHT, CAMERA, NODE, ANIM, TRACK };

static struct ts_node *create_mtltree(struct goat3d *g, const struct goat3d_material *mtl);
static struct ts_node *create_meshtree(struct goat3d *g, const struct goat3d_mesh *mesh);
static struct ts_node *create_lighttree(struct goat3d *g, const struct goat3d_light *light);
static struct ts_node *create_camtree(struct goat3d *g, const struct goat3d_camera *cam);
static struct ts_node *create_nodetree(struct goat3d *g, const struct goat3d_node *node);
static struct ts_node *create_animtree(struct goat3d *g, const struct goat3d_anim *anim);
static struct ts_node *create_tracktree(struct goat3d *g, const struct goat3d_track *trk);

static void init_namegen(struct goat3d *g);
static const char *namegen(struct goat3d *g, const char *name, int type);

GOAT3DAPI char *goat3d_b64encode(const void *data, int size, char *buf, int *bufsz);
#define b64encode goat3d_b64encode

#define create_tsnode(n, p, nstr) \
	do { \
		int len = strlen(nstr); \
		if(!((n) = ts_alloc_node())) { \
			goat3d_logmsg(LOG_ERROR, "failed to create treestore node\n"); \
			goto err; \
		} \
		if(!((n)->name = malloc(len + 1))) { \
			goat3d_logmsg(LOG_ERROR, "failed to allocate node name string\n"); \
			ts_free_node(n); \
			goto err; \
		} \
		memcpy((n)->name, (nstr), len + 1); \
		if(p) { \
			ts_add_child((p), (n)); \
		} \
	} while(0)

#define create_tsattr(a, n, nstr, atype) \
	do { \
		if(!((a) = ts_alloc_attr())) { \
			goat3d_logmsg(LOG_ERROR, "failed to create treestore attribute\n"); \
			goto err; \
		} \
		if(ts_set_attr_name(a, nstr) == -1) { \
			goat3d_logmsg(LOG_ERROR, "failed to allocate attrib name string\n"); \
			ts_free_attr(a); \
			goto err; \
		} \
		(a)->val.type = (atype); \
		if(n) { \
			ts_add_attr((n), (a)); \
		} \
	} while(0)



int g3dimpl_scnsave(const struct goat3d *g, struct goat3d_io *io)
{
	int i, num;
	struct ts_io tsio;
	struct ts_node *tsroot = 0, *tsn, *tsenv;
	struct ts_attr *tsa;

	tsio.data = io->cls;
	tsio.read = io->read;
	tsio.write = io->write;

	init_namegen((struct goat3d*)g);

	create_tsnode(tsroot, 0, "scene");

	/* environment */
	create_tsnode(tsenv, tsroot, "env");
	create_tsattr(tsa, tsenv, "ambient", TS_VECTOR);
	ts_set_valuefv(&tsa->val, 3, g->ambient.x, g->ambient.y, g->ambient.z);
	/* TODO: fog */

	num = dynarr_size(g->materials);
	for(i=0; i<num; i++) {
		if(!(tsn = create_mtltree((struct goat3d*)g, g->materials[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->meshes);
	for(i=0; i<num; i++) {
		if(!(tsn = create_meshtree((struct goat3d*)g, g->meshes[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->lights);
	for(i=0; i<num; i++) {
		if(!(tsn = create_lighttree((struct goat3d*)g, g->lights[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->cameras);
	for(i=0; i<num; i++) {
		if(!(tsn = create_camtree((struct goat3d*)g, g->cameras[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->nodes);
	for(i=0; i<num; i++) {
		if(!(tsn = create_nodetree((struct goat3d*)g, g->nodes[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->anims);
	for(i=0; i<num; i++) {
		if(!(tsn = create_animtree((struct goat3d*)g, g->anims[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	if(ts_save_io(tsroot, &tsio) == -1) {
		goat3d_logmsg(LOG_ERROR, "g3dimpl_scnsave: failed\n");
		goto err;
	}
	return 0;

err:
	ts_free_tree(tsroot);
	return -1;
}

int g3dimpl_anmsave(const struct goat3d *g, struct goat3d_io *io)
{
	return -1;
}

static struct ts_node *create_mtltree(struct goat3d *g, const struct goat3d_material *mtl)
{
	int i, num_attr;
	struct ts_node *tsn, *tsmtl = 0;
	struct ts_attr *tsa;

	create_tsnode(tsmtl, 0, "mtl");
	create_tsattr(tsa, tsmtl, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, namegen(g, mtl->name, MTL)) == -1) {
		goto err;
	}

	num_attr = dynarr_size(mtl->attrib);
	for(i=0; i<num_attr; i++) {
		struct material_attrib *attr = mtl->attrib + i;

		create_tsnode(tsn, tsmtl, "attr");
		create_tsattr(tsa, tsn, "name", TS_STRING);
		if(ts_set_value_str(&tsa->val, attr->name) == -1) {
			goto err;
		}
		create_tsattr(tsa, tsn, "val", TS_VECTOR);
		ts_set_valuefv(&tsa->val, 4, attr->value.x, attr->value.y, attr->value.z, attr->value.w);
		if(attr->map) {
			create_tsattr(tsa, tsn, "map", TS_STRING);
			if(ts_set_value_str(&tsa->val, attr->map) == -1) {
				goto err;
			}
		}
	}
	return tsmtl;

err:
	ts_free_tree(tsmtl);
	return 0;
}

static struct ts_node *create_meshtree(struct goat3d *g, const struct goat3d_mesh *mesh)
{
	int i, num;
	struct ts_node *tsmesh = 0, *tslist, *tsitem;
	struct ts_attr *tsa;

	create_tsnode(tsmesh, 0, "mesh");
	create_tsattr(tsa, tsmesh, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, namegen(g, mesh->name, MESH)) == -1) {
		goto err;
	}

	if(mesh->mtl) {
		if(mesh->mtl->name) {
			create_tsattr(tsa, tsmesh, "material", TS_STRING);
			if(ts_set_value_str(&tsa->val, mesh->mtl->name) == -1) {
				goto err;
			}
		} else {
			create_tsattr(tsa, tsmesh, "material", TS_NUMBER);
			ts_set_valuei(&tsa->val, mesh->mtl->idx);
		}
	}

	/* TODO option of saving separate mesh files */

	if((num = dynarr_size(mesh->vertices))) {
		create_tsnode(tslist, tsmesh, "vertex_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->vertices, num * 3 * sizeof(float), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				cgm_vec3 *vptr = mesh->vertices + i;
				create_tsnode(tsitem, tslist, "vertex");
				create_tsattr(tsa, tsitem, "pos", TS_VECTOR);
				ts_set_valuefv(&tsa->val, 3, vptr->x, vptr->y, vptr->z);
			}
		}
	}

	if((num = dynarr_size(mesh->normals))) {
		create_tsnode(tslist, tsmesh, "normal_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->normals, num * 3 * sizeof(float), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				cgm_vec3 *nptr = mesh->normals + i;
				create_tsnode(tsitem, tslist, "normal");
				create_tsattr(tsa, tsitem, "dir", TS_VECTOR);
				ts_set_valuefv(&tsa->val, 3, nptr->x, nptr->y, nptr->z);
			}
		}
	}

	if((num = dynarr_size(mesh->tangents))) {
		create_tsnode(tslist, tsmesh, "tangent_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->tangents, num * 3 * sizeof(float), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				cgm_vec3 *tptr = mesh->tangents + i;
				create_tsnode(tsitem, tslist, "tangent");
				create_tsattr(tsa, tsitem, "dir", TS_VECTOR);
				ts_set_valuefv(&tsa->val, 3, tptr->x, tptr->y, tptr->z);
			}
		}
	}

	if((num = dynarr_size(mesh->texcoords))) {
		create_tsnode(tslist, tsmesh, "texcoord_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->texcoords, num * 3 * sizeof(float), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				cgm_vec2 *uvptr = mesh->texcoords + i;
				create_tsnode(tsitem, tslist, "texcoord");
				create_tsattr(tsa, tsitem, "uv", TS_VECTOR);
				ts_set_valuefv(&tsa->val, 3, uvptr->x, uvptr->y, 0.0f);
			}
		}
	}

	if((num = dynarr_size(mesh->skin_weights))) {
		create_tsnode(tslist, tsmesh, "skinweight_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->skin_weights, num * 4 * sizeof(float), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				cgm_vec4 *wptr = mesh->skin_weights + i;
				create_tsnode(tsitem, tslist, "skinweight");
				create_tsattr(tsa, tsitem, "weights", TS_VECTOR);
				ts_set_valuefv(&tsa->val, 4, wptr->x, wptr->y, wptr->z, wptr->w);
			}
		}
	}

	if((num = dynarr_size(mesh->skin_matrices))) {
		create_tsnode(tslist, tsmesh, "skinmatrix_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->skin_matrices, num * 4 * sizeof(int), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				int4 *iptr = mesh->skin_matrices + i;
				create_tsnode(tsitem, tslist, "skinmatrix");
				create_tsattr(tsa, tsitem, "idx", TS_VECTOR);
				ts_set_valueiv(&tsa->val, 4, iptr->x, iptr->y, iptr->z, iptr->w);
			}
		}
	}

	if((num = dynarr_size(mesh->colors))) {
		create_tsnode(tslist, tsmesh, "color_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->colors, num * 4 * sizeof(float), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				cgm_vec4 *cptr = mesh->colors + i;
				create_tsnode(tsitem, tslist, "color");
				create_tsattr(tsa, tsitem, "color", TS_VECTOR);
				ts_set_valuefv(&tsa->val, 4, cptr->x, cptr->y, cptr->z, cptr->w);
			}
		}
	}

	if((num = dynarr_size(mesh->bones))) {
		create_tsnode(tslist, tsmesh, "bone_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		/* TODO: base64 option */
		for(i=0; i<num; i++) {
			create_tsnode(tsitem, tslist, "bone");
			create_tsattr(tsa, tsitem, "name", TS_STRING);
			if(ts_set_value_str(&tsa->val, mesh->bones[i]->name) == -1) {
				goto err;
			}
		}
	}

	if((num = dynarr_size(mesh->faces))) {
		create_tsnode(tslist, tsmesh, "face_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		if(goat3d_getopt(g, GOAT3D_OPT_SAVEBINDATA)) {
			create_tsattr(tsa, tslist, "base64", TS_STRING);
			if(!(tsa->val.str = b64encode(mesh->faces, num * 3 * sizeof(int), 0, 0))) {
				goto err;
			}
		} else {
			for(i=0; i<num; i++) {
				struct face *fptr = mesh->faces + i;
				create_tsnode(tsitem, tslist, "face");
				create_tsattr(tsa, tsitem, "idx", TS_VECTOR);
				ts_set_valueiv(&tsa->val, 3, fptr->v[0], fptr->v[1], fptr->v[2]);
			}
		}
	}

	return tsmesh;

err:
	ts_free_tree(tsmesh);
	return 0;
}

static struct ts_node *create_lighttree(struct goat3d *g, const struct goat3d_light *light)
{
	struct ts_node *tslight = 0;
	struct ts_attr *tsa;

	create_tsnode(tslight, 0, "light");
	create_tsattr(tsa, tslight, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, namegen(g, light->name, LIGHT)) == -1) {
		goto err;
	}

	if(light->ltype != LTYPE_DIR) {
		create_tsattr(tsa, tslight, "pos", TS_VECTOR);
		ts_set_valuefv(&tsa->val, 3, light->pos.x, light->pos.y, light->pos.z);
	}

	if(light->ltype != LTYPE_POINT) {
		create_tsattr(tsa, tslight, "dir", TS_VECTOR);
		ts_set_valuefv(&tsa->val, 3, light->dir.x, light->dir.y, light->dir.z);
	}

	if(light->ltype == LTYPE_SPOT) {
		create_tsattr(tsa, tslight, "cone_inner", TS_NUMBER);
		ts_set_valuef(&tsa->val, light->inner_cone);
		create_tsattr(tsa, tslight, "cone_outer", TS_NUMBER);
		ts_set_valuef(&tsa->val, light->outer_cone);
	}

	create_tsattr(tsa, tslight, "color", TS_VECTOR);
	ts_set_valuefv(&tsa->val, 3, light->color.x, light->color.y, light->color.z);

	create_tsattr(tsa, tslight, "atten", TS_VECTOR);
	ts_set_valuefv(&tsa->val, 3, light->attenuation.x, light->attenuation.y, light->attenuation.z);

	create_tsattr(tsa, tslight, "distance", TS_NUMBER);
	ts_set_valuef(&tsa->val, light->max_dist);

	return tslight;

err:
	ts_free_tree(tslight);
	return 0;
}

static struct ts_node *create_camtree(struct goat3d *g, const struct goat3d_camera *cam)
{
	struct ts_node *tscam = 0;
	struct ts_attr *tsa;

	create_tsnode(tscam, 0, "camera");
	create_tsattr(tsa, tscam, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, namegen(g, cam->name, CAMERA)) == -1) {
		goto err;
	}

	create_tsattr(tsa, tscam, "pos", TS_VECTOR);
	ts_set_valuefv(&tsa->val, 3, cam->pos.x, cam->pos.y, cam->pos.z);

	if(cam->camtype == CAMTYPE_TARGET) {
		create_tsattr(tsa, tscam, "target", TS_VECTOR);
		ts_set_valuefv(&tsa->val, 3, cam->target.x, cam->target.y, cam->target.z);
	}

	create_tsattr(tsa, tscam, "fov", TS_NUMBER);
	ts_set_valuef(&tsa->val, cam->fov);

	create_tsattr(tsa, tscam, "nearclip", TS_NUMBER);
	ts_set_valuef(&tsa->val, cam->near_clip);

	create_tsattr(tsa, tscam, "farclip", TS_NUMBER);
	ts_set_valuef(&tsa->val, cam->far_clip);

	return tscam;

err:
	ts_free_tree(tscam);
	return 0;
}

static struct ts_node *create_nodetree(struct goat3d *g, const struct goat3d_node *node)
{
	struct ts_node *tsnode = 0;
	struct ts_attr *tsa;
	struct goat3d_node *par;
	static const char *objtypestr[] = {"null", "mesh", "light", "camera"};
	float vec[4];
	float xform[16];

	create_tsnode(tsnode, 0, "node");
	create_tsattr(tsa, tsnode, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, namegen(g, node->name, NODE)) == -1) {
		goto err;
	}

	if((par = goat3d_get_node_parent(node))) {
		create_tsattr(tsa, tsnode, "parent", TS_STRING);
		if(ts_set_value_str(&tsa->val, goat3d_get_node_name(par)) == -1) {
			goto err;
		}
	}

	if(node->obj && node->type != GOAT3D_NODE_NULL) {
		create_tsattr(tsa, tsnode, objtypestr[node->type], TS_STRING);
		if(ts_set_value_str(&tsa->val, ((struct object*)node->obj)->name) == -1) {
			goto err;
		}
	}

	goat3d_get_node_position(node, vec, vec + 1, vec + 2);
	create_tsattr(tsa, tsnode, "pos", TS_VECTOR);
	ts_set_valuef_arr(&tsa->val, 3, vec);

	goat3d_get_node_rotation(node, vec, vec + 1, vec + 2, vec + 3);
	create_tsattr(tsa, tsnode, "rot", TS_VECTOR);
	ts_set_valuef_arr(&tsa->val, 4, vec);

	goat3d_get_node_scaling(node, vec, vec + 1, vec + 2);
	create_tsattr(tsa, tsnode, "scale", TS_VECTOR);
	ts_set_valuef_arr(&tsa->val, 3, vec);

	goat3d_get_node_pivot(node, vec, vec + 1, vec + 2);
	create_tsattr(tsa, tsnode, "pivot", TS_VECTOR);
	ts_set_valuef_arr(&tsa->val, 3, vec);

	goat3d_get_node_matrix(node, xform);
	cgm_mtranspose(xform);
	create_tsattr(tsa, tsnode, "matrix0", TS_VECTOR);
	ts_set_valuef_arr(&tsa->val, 4, xform);
	create_tsattr(tsa, tsnode, "matrix1", TS_VECTOR);
	ts_set_valuef_arr(&tsa->val, 4, xform + 4);
	create_tsattr(tsa, tsnode, "matrix2", TS_VECTOR);
	ts_set_valuef_arr(&tsa->val, 4, xform + 8);

	return tsnode;

err:
	ts_free_tree(tsnode);
	return 0;
}

static struct ts_node *create_animtree(struct goat3d *g, const struct goat3d_anim *anim)
{
	int i, num_trk;
	struct ts_node *tsanim, *tstrk;
	struct ts_attr *tsa;

	create_tsnode(tsanim, 0, "anim");
	create_tsattr(tsa, tsanim, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, namegen(g, anim->name, ANIM)) == -1) {
		goto err;
	}


	num_trk = goat3d_get_anim_track_count(anim);
	for(i=0; i<num_trk; i++) {
		if((tstrk = create_tracktree(g, goat3d_get_anim_track(anim, i)))) {
			ts_add_child(tsanim, tstrk);
		}
	}

	return tsanim;

err:
	ts_free_tree(tsanim);
	return 0;
}

static const char *instr[] = {"step", "linear", "cubic"};
static const char *exstr[] = {"extend", "clamp", "repeat", "pingpong"};

static struct ts_node *create_tracktree(struct goat3d *g, const struct goat3d_track *trk)
{
	int i, num_keys;
	struct ts_node *tstrk, *tskey;
	struct ts_attr *tsa;
	struct goat3d_key key;
	enum goat3d_track_type basetype;

	create_tsnode(tstrk, 0, "track");
	create_tsattr(tsa, tstrk, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, namegen(g, trk->name, TRACK)) == -1) {
		goto err;
	}

	create_tsattr(tsa, tstrk, "type", TS_STRING);
	if(ts_set_value_str(&tsa->val, g3dimpl_trktypestr(trk->type)) == -1) {
		goto err;
	}
	basetype = trk->type & 0xff;

	create_tsattr(tsa, tstrk, "interp", TS_STRING);
	if(ts_set_value_str(&tsa->val, instr[trk->trk[0].interp]) == -1) {
		goto err;
	}
	create_tsattr(tsa, tstrk, "extrap", TS_STRING);
	if(ts_set_value_str(&tsa->val, exstr[trk->trk[0].extrap]) == -1) {
		goto err;
	}

	if(trk->node) {
		create_tsattr(tsa, tstrk, "node", TS_STRING);
		if(ts_set_value_str(&tsa->val, trk->node->name) == -1) {
			goto err;
		}
	}

	num_keys = goat3d_get_track_key_count(trk);
	for(i=0; i<num_keys; i++) {
		goat3d_get_track_key(trk, i, &key);

		create_tsnode(tskey, tstrk, "key");
		create_tsattr(tsa, tskey, "time", TS_NUMBER);
		ts_set_valuei(&tsa->val, key.tm);

		if(basetype == GOAT3D_TRACK_VAL) {
			create_tsattr(tsa, tskey, "value", TS_NUMBER);
			ts_set_valuef(&tsa->val, key.val[0]);
		} else {
			static const int typecount[] = {1, 3, 4, 4};
			create_tsattr(tsa, tskey, "value", TS_VECTOR);
			ts_set_valuef_arr(&tsa->val, typecount[basetype], key.val);
		}
	}

	return tstrk;

err:
	ts_free_tree(tstrk);
	return 0;
}



static void init_namegen(struct goat3d *g)
{
	memset(g->namecnt, 0, sizeof g->namecnt);
}

static const char *namegen(struct goat3d *g, const char *name, int type)
{
	static const char *fmt[] = {"material%03u", "mesh%03u", "light%03u",
		"camera%03u", "node%03u", "animation%03u", "track%03u"};

	if(name) {
		/* if an actual name happens to match our pattern, make sure to skip it
		 * for the auto-generated names
		 */
		int n;
		if(sscanf(name, fmt[type], &n)) {
			g->namecnt[type] = n;
		}
		return name;
	}

	sprintf(g->namebuf, fmt[type], g->namecnt[type]++);
	return g->namebuf;
}

static const char *enctab =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

GOAT3DAPI char *goat3d_b64encode(const void *data, int size, char *buf, int *bufsz)
{
	const unsigned char *src = data;
	char *dest;
	int i;
	int outsz = size * 4 / 3 + 1;
	int bitgrp[4];
	int leftover;

	if(buf) {
		if(*bufsz < outsz) {
			*bufsz = outsz;
			return 0;
		}
	} else {
		if(bufsz) *bufsz = outsz;

		/* reserve more space for up to two padding bytes */
		if(!(buf = malloc(outsz + 2))) {
			return 0;
		}
	}
	dest = buf;

	leftover = 0;
	while(size > 0) {
		bitgrp[0] = (src[0] & 0xfc) >> 2;
		bitgrp[1] = (src[0] & 3) << 4;
		if(--size <= 0) {
			leftover = 2;
			break;
		}
		bitgrp[1] |= src[1] >> 4;
		bitgrp[2] = (src[1] & 0xf) << 2;
		if(--size <= 0) {
			leftover = 3;
			break;
		}
		bitgrp[2] |= src[2] >> 6;
		bitgrp[3] = src[2] & 0x3f;

		dest[0] = enctab[bitgrp[0]];
		dest[1] = enctab[bitgrp[1]];
		dest[2] = enctab[bitgrp[2]];
		dest[3] = enctab[bitgrp[3]];

		dest += 4;
		src += 3;
		leftover = 0;
		size--;
	}

	if(leftover) {
		for(i=0; i<4; i++) {
			if(i < leftover) {
				*dest++ = enctab[bitgrp[i]];
			} else {
				*dest++ = '=';
			}
		}
	}

	*dest = 0;
	return buf;
}
