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
#include <string.h>
#include "g3dscn.h"
#include "dynarr.h"

int g3dimpl_obj_init(struct object *o, int type)
{
	static int last_mesh, last_light, last_camera;
	struct goat3d_mesh *m;
	struct goat3d_light *lt;
	struct goat3d_camera *cam;
	char *name;

	if(!(name = malloc(64))) {
		return -1;
	}

	switch(type) {
	case OBJTYPE_MESH:
		m = (struct goat3d_mesh*)o;
		memset(m, 0, sizeof *m);
		if(!(m->vertices = dynarr_alloc(0, sizeof *m->vertices))) goto err;
		if(!(m->normals = dynarr_alloc(0, sizeof *m->normals))) goto err;
		if(!(m->tangents = dynarr_alloc(0, sizeof *m->tangents))) goto err;
		if(!(m->texcoords = dynarr_alloc(0, sizeof *m->texcoords))) goto err;
		if(!(m->skin_weights = dynarr_alloc(0, sizeof *m->skin_weights))) goto err;
		if(!(m->skin_matrices = dynarr_alloc(0, sizeof *m->skin_matrices))) goto err;
		if(!(m->colors = dynarr_alloc(0, sizeof *m->colors))) goto err;
		if(!(m->faces = dynarr_alloc(0, sizeof *m->faces))) goto err;
		if(!(m->bones = dynarr_alloc(0, sizeof *m->bones))) goto err;
		sprintf(name, "mesh%d", last_mesh++);
		break;

	case OBJTYPE_LIGHT:
		lt = (struct goat3d_light*)o;
		memset(lt, 0, sizeof *lt);
		cgm_vcons(&lt->color, 1, 1, 1);
		cgm_vcons(&lt->attenuation, 1, 0, 0);
		cgm_vcons(&lt->dir, 0, 0, 1);
		lt->inner_cone = cgm_deg_to_rad(30);
		lt->outer_cone = cgm_deg_to_rad(45);
		sprintf(name, "light%d", last_light++);
		break;

	case OBJTYPE_CAMERA:
		cam = (struct goat3d_camera*)o;
		memset(cam, 0, sizeof *cam);
		cam->near_clip = 0.5f;
		cam->far_clip = 500.0f;
		cgm_vcons(&cam->up, 0, 1, 0);
		sprintf(name, "camera%d", last_camera++);
		break;

	default:
		return -1;
	}

	o->name = name;
	o->type = type;
	cgm_qcons(&o->rot, 0, 0, 0, 1);
	cgm_vcons(&o->scale, 1, 1, 1);
	return 0;

err:
	free(name);
	g3dimpl_obj_destroy(o);
	return -1;
}

void g3dimpl_obj_destroy(struct object *o)
{
	struct goat3d_mesh *m;

	switch(o->type) {
	case OBJTYPE_MESH:
		m = (struct goat3d_mesh*)o;
		dynarr_free(m->vertices);
		dynarr_free(m->normals);
		dynarr_free(m->tangents);
		dynarr_free(m->texcoords);
		dynarr_free(m->skin_weights);
		dynarr_free(m->skin_matrices);
		dynarr_free(m->colors);
		dynarr_free(m->faces);
		dynarr_free(m->bones);
		break;

	default:
		break;
	}
}

void g3dimpl_mesh_bounds(struct aabox *bb, struct goat3d_mesh *m, float *xform)
{
	int i, nverts;

	g3dimpl_aabox_init(bb);

	nverts = dynarr_size(m->vertices);
	for(i=0; i<nverts; i++) {
		cgm_vec3 v = m->vertices[i];
		if(xform) cgm_vmul_m4v3(&v, xform);

		if(v.x < bb->bmin.x) bb->bmin.x = v.x;
		if(v.y < bb->bmin.y) bb->bmin.y = v.y;
		if(v.z < bb->bmin.z) bb->bmin.z = v.z;

		if(v.x > bb->bmax.x) bb->bmax.x = v.x;
		if(v.y > bb->bmax.y) bb->bmax.y = v.y;
		if(v.z > bb->bmax.z) bb->bmax.z = v.z;
	}
}

int g3dimpl_mtl_init(struct goat3d_material *mtl)
{
	memset(mtl, 0, sizeof *mtl);
	if(!(mtl->attrib = dynarr_alloc(0, sizeof *mtl->attrib))) {
		return -1;
	}
	return 0;
}

void g3dimpl_mtl_destroy(struct goat3d_material *mtl)
{
	int i, num = dynarr_size(mtl->attrib);
	for(i=0; i<num; i++) {
		free(mtl->attrib[i].name);
		free(mtl->attrib[i].map);
	}
	dynarr_free(mtl->attrib);
}

struct material_attrib *g3dimpl_mtl_findattr(struct goat3d_material *mtl, const char *name)
{
	int i, num = dynarr_size(mtl->attrib);

	for(i=0; i<num; i++) {
		if(strcmp(mtl->attrib[i].name, name) == 0) {
			return mtl->attrib + i;
		}
	}
	return 0;
}

struct material_attrib *g3dimpl_mtl_getattr(struct goat3d_material *mtl, const char *name)
{
	int idx, len;
	char *tmpname;
	struct material_attrib *tmpattr, *ma;

	if((ma = g3dimpl_mtl_findattr(mtl, name))) {
		return ma;
	}

	len = strlen(name);
	if(!(tmpname = malloc(len + 1))) {
		return 0;
	}
	memcpy(tmpname, name, len + 1);

	idx = dynarr_size(mtl->attrib);
	if(!(tmpattr = dynarr_push(mtl->attrib, 0))) {
		free(tmpname);
		return 0;
	}
	mtl->attrib = tmpattr;

	ma = mtl->attrib + idx;
	ma->name = tmpname;
	ma->map = 0;
	cgm_wcons(&ma->value, 1, 1, 1, 1);

	return ma;
}

void g3dimpl_node_bounds(struct aabox *bb, struct goat3d_node *n)
{
	struct object *obj = n->obj;
	struct goat3d_node *cn = n->child;
	float xform[16];

	goat3d_get_matrix(n, xform);

	if(obj && obj->type == OBJTYPE_MESH) {
		g3dimpl_mesh_bounds(bb, (struct goat3d_mesh*)obj, xform);
	} else {
		g3dimpl_aabox_init(bb);
	}

	while(cn) {
		struct aabox cbox;
		g3dimpl_node_bounds(&cbox, cn);
		g3dimpl_aabox_union(bb, bb, &cbox);
		cn = cn->next;
	}
}
