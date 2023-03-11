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

static struct ts_node *create_mtltree(const struct goat3d_material *mtl);
static struct ts_node *create_meshtree(const struct goat3d_mesh *mesh);
static struct ts_node *create_lighttree(const struct goat3d_light *light);
static struct ts_node *create_camtree(const struct goat3d_camera *cam);
static struct ts_node *create_nodetree(const struct goat3d_node *node);
static struct ts_node *create_animtree(const struct goat3d_anim *anim);
static struct ts_node *create_tracktree(const struct goat3d_track *trk);

#define create_tsnode(n, p, nstr) \
	do { \
		int len = strlen(nstr); \
		if(!((n) = ts_alloc_node())) { \
			goat3d_logmsg(LOG_ERROR, "%s: failed to create treestore node\n", __func__); \
			goto err; \
		} \
		if(!((n)->name = malloc(len + 1))) { \
			goat3d_logmsg(LOG_ERROR, "%s: failed to allocate node name string\n", __func__); \
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
			goat3d_logmsg(LOG_ERROR, "%s: failed to create treestore attribute\n", __func__); \
			goto err; \
		} \
		if(ts_set_attr_name(a, nstr) == -1) { \
			goat3d_logmsg(LOG_ERROR, "%s: failed to allocate attrib name string\n", __func__); \
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

	create_tsnode(tsroot, 0, "scene");

	/* environment */
	create_tsnode(tsenv, tsroot, "env");
	create_tsattr(tsa, tsenv, "ambient", TS_VECTOR);
	ts_set_valuefv(&tsa->val, 3, g->ambient.x, g->ambient.y, g->ambient.z);
	/* TODO: fog */

	num = dynarr_size(g->materials);
	for(i=0; i<num; i++) {
		if(!(tsn = create_mtltree(g->materials[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->meshes);
	for(i=0; i<num; i++) {
		if(!(tsn = create_meshtree(g->meshes[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->lights);
	for(i=0; i<num; i++) {
		if(!(tsn = create_lighttree(g->lights[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->cameras);
	for(i=0; i<num; i++) {
		if(!(tsn = create_camtree(g->cameras[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->nodes);
	for(i=0; i<num; i++) {
		if(!(tsn = create_nodetree(g->nodes[i]))) {
			goto err;
		}
		ts_add_child(tsroot, tsn);
	}

	num = dynarr_size(g->anims);
	for(i=0; i<num; i++) {
		if(!(tsn = create_animtree(g->anims[i]))) {
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

static struct ts_node *create_mtltree(const struct goat3d_material *mtl)
{
	int i, num_attr;
	struct ts_node *tsn, *tsmtl = 0;
	struct ts_attr *tsa;

	create_tsnode(tsmtl, 0, "mtl");
	create_tsattr(tsa, tsmtl, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, mtl->name) == -1) {
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

static struct ts_node *create_meshtree(const struct goat3d_mesh *mesh)
{
	int i, num;
	struct ts_node *tsmesh = 0, *tslist, *tsitem;
	struct ts_attr *tsa;

	create_tsnode(tsmesh, 0, "mesh");
	create_tsattr(tsa, tsmesh, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, mesh->name) == -1) {
		goto err;
	}

	if(mesh->mtl) {
		create_tsattr(tsa, tsmesh, "material", TS_STRING);
		if(ts_set_value_str(&tsa->val, mesh->mtl->name) == -1) {
			goto err;
		}
	}

	/* TODO option of saving separate mesh files */

	if((num = dynarr_size(mesh->vertices))) {
		create_tsnode(tslist, tsmesh, "vertex_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		for(i=0; i<num; i++) {
			cgm_vec3 *vptr = mesh->vertices + i;
			create_tsnode(tsitem, tslist, "vertex");
			create_tsattr(tsa, tsitem, "pos", TS_VECTOR);
			ts_set_valuefv(&tsa->val, 3, vptr->x, vptr->y, vptr->z);
		}
	}

	if((num = dynarr_size(mesh->normals))) {
		create_tsnode(tslist, tsmesh, "normal_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		for(i=0; i<num; i++) {
			cgm_vec3 *nptr = mesh->normals + i;
			create_tsnode(tsitem, tslist, "normal");
			create_tsattr(tsa, tsitem, "dir", TS_VECTOR);
			ts_set_valuefv(&tsa->val, 3, nptr->x, nptr->y, nptr->z);
		}
	}

	if((num = dynarr_size(mesh->tangents))) {
		create_tsnode(tslist, tsmesh, "tangent_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		for(i=0; i<num; i++) {
			cgm_vec3 *tptr = mesh->tangents + i;
			create_tsnode(tsitem, tslist, "tangent");
			create_tsattr(tsa, tsitem, "dir", TS_VECTOR);
			ts_set_valuefv(&tsa->val, 3, tptr->x, tptr->y, tptr->z);
		}
	}

	if((num = dynarr_size(mesh->texcoords))) {
		create_tsnode(tslist, tsmesh, "texcoord_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		for(i=0; i<num; i++) {
			cgm_vec2 *uvptr = mesh->texcoords + i;
			create_tsnode(tsitem, tslist, "texcoord");
			create_tsattr(tsa, tsitem, "uv", TS_VECTOR);
			ts_set_valuefv(&tsa->val, 3, uvptr->x, uvptr->y, 0.0f);
		}
	}

	if((num = dynarr_size(mesh->skin_weights))) {
		create_tsnode(tslist, tsmesh, "skinweight_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		for(i=0; i<num; i++) {
			cgm_vec4 *wptr = mesh->skin_weights + i;
			create_tsnode(tsitem, tslist, "skinweight");
			create_tsattr(tsa, tsitem, "weights", TS_VECTOR);
			ts_set_valuefv(&tsa->val, 4, wptr->x, wptr->y, wptr->z, wptr->w);
		}
	}

	if((num = dynarr_size(mesh->skin_matrices))) {
		create_tsnode(tslist, tsmesh, "skinmatrix_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		for(i=0; i<num; i++) {
			int4 *iptr = mesh->skin_matrices + i;
			create_tsnode(tsitem, tslist, "skinmatrix");
			create_tsattr(tsa, tsitem, "idx", TS_VECTOR);
			ts_set_valueiv(&tsa->val, 4, iptr->x, iptr->y, iptr->z, iptr->w);
		}
	}

	if((num = dynarr_size(mesh->colors))) {
		create_tsnode(tslist, tsmesh, "color_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

		for(i=0; i<num; i++) {
			cgm_vec4 *cptr = mesh->colors + i;
			create_tsnode(tsitem, tslist, "color");
			create_tsattr(tsa, tsitem, "color", TS_VECTOR);
			ts_set_valuefv(&tsa->val, 4, cptr->x, cptr->y, cptr->z, cptr->w);
		}
	}

	if((num = dynarr_size(mesh->bones))) {
		create_tsnode(tslist, tsmesh, "bone_list");
		create_tsattr(tsa, tslist, "list_size", TS_NUMBER);
		ts_set_valuei(&tsa->val, num);

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

		for(i=0; i<num; i++) {
			struct face *fptr = mesh->faces + i;
			create_tsnode(tsitem, tslist, "face");
			create_tsattr(tsa, tsitem, "idx", TS_VECTOR);
			ts_set_valueiv(&tsa->val, 3, fptr->v[0], fptr->v[1], fptr->v[2]);
		}
	}

	return tsmesh;

err:
	ts_free_tree(tsmesh);
	return 0;
}

static struct ts_node *create_lighttree(const struct goat3d_light *light)
{
	struct ts_node *tslight = 0;
	struct ts_attr *tsa;

	create_tsnode(tslight, 0, "light");
	create_tsattr(tsa, tslight, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, light->name) == -1) {
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

static struct ts_node *create_camtree(const struct goat3d_camera *cam)
{
	struct ts_node *tscam = 0;
	struct ts_attr *tsa;

	create_tsnode(tscam, 0, "camera");
	create_tsattr(tsa, tscam, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, cam->name) == -1) {
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

static struct ts_node *create_nodetree(const struct goat3d_node *node)
{
	struct ts_node *tsnode = 0;
	struct ts_attr *tsa;
	struct goat3d_node *par;
	static const char *objtypestr[] = {"null", "mesh", "light", "camera"};
	float vec[4];
	float xform[16];

	create_tsnode(tsnode, 0, "node");
	create_tsattr(tsa, tsnode, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, goat3d_get_node_name(node)) == -1) {
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

static struct ts_node *create_animtree(const struct goat3d_anim *anim)
{
	int i, num_trk;
	struct ts_node *tsanim, *tstrk;
	struct ts_attr *tsa;

	create_tsnode(tsanim, 0, "anim");
	create_tsattr(tsa, tsanim, "name", TS_STRING);
	if(ts_set_value_str(&tsa->val, goat3d_get_anim_name(anim)) == -1) {
		goto err;
	}


	num_trk = goat3d_get_anim_track_count(anim);
	for(i=0; i<num_trk; i++) {
		if((tstrk = create_tracktree(goat3d_get_anim_track(anim, i)))) {
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

static struct ts_node *create_tracktree(const struct goat3d_track *trk)
{
	int i, num_keys;
	struct ts_node *tstrk, *tskey;
	struct ts_attr *tsa;
	struct goat3d_key key;
	enum goat3d_track_type basetype;
	const char *str;

	create_tsnode(tstrk, 0, "track");
	if((str = goat3d_get_track_name(trk))) {
		create_tsattr(tsa, tstrk, "name", TS_STRING);
		if(ts_set_value_str(&tsa->val, str) == -1) {
			goto err;
		}
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
