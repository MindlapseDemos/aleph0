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
#include <errno.h>
#include <ctype.h>
#include "goat3d.h"
#include "g3dscn.h"
#include "log.h"
#include "dynarr.h"

static long read_file(void *buf, size_t bytes, void *uptr);
static long write_file(const void *buf, size_t bytes, void *uptr);
static long seek_file(long offs, int whence, void *uptr);
static char *clean_filename(char *str);

static const char *def_scn_name = "unnamed";

#define SETNAME(dest, str)	\
	do { \
		char *tmpname; \
		int len = strlen(str); \
		if(!(tmpname = malloc(len + 1))) { \
			return -1; \
		} \
		memcpy(tmpname, str, len + 1); \
		free(dest); \
		dest = tmpname; \
		return 0; \
	} while(0)


GOAT3DAPI struct goat3d *goat3d_create(void)
{
	struct goat3d *g;

	if(!(g = malloc(sizeof *g))) {
		return 0;
	}
	if(goat3d_init(g) == -1) {
		free(g);
		return 0;
	}
	return g;
}

GOAT3DAPI void goat3d_free(struct goat3d *g)
{
	goat3d_destroy(g);
	free(g);
}

int goat3d_init(struct goat3d *g)
{
	memset(g, 0, sizeof *g);

	cgm_vcons(&g->ambient, 0.05, 0.05, 0.05);

	if(!(g->materials = dynarr_alloc(0, sizeof *g->materials))) goto err;
	if(!(g->meshes = dynarr_alloc(0, sizeof *g->meshes))) goto err;
	if(!(g->lights = dynarr_alloc(0, sizeof *g->lights))) goto err;
	if(!(g->cameras = dynarr_alloc(0, sizeof *g->cameras))) goto err;
	if(!(g->nodes = dynarr_alloc(0, sizeof *g->nodes))) goto err;
	if(!(g->anims = dynarr_alloc(0, sizeof *g->anims))) goto err;

	return 0;

err:
	goat3d_destroy(g);
	return -1;
}

void goat3d_destroy(struct goat3d *g)
{
	goat3d_clear(g);

	dynarr_free(g->materials);
	dynarr_free(g->meshes);
	dynarr_free(g->lights);
	dynarr_free(g->cameras);
	dynarr_free(g->nodes);
	dynarr_free(g->anims);
}

void goat3d_clear(struct goat3d *g)
{
	int i, num;

	num = dynarr_size(g->materials);
	for(i=0; i<num; i++) {
		g3dimpl_mtl_destroy(g->materials[i]);
		free(g->materials[i]);
	}
	DYNARR_CLEAR(g->materials);

	num = dynarr_size(g->meshes);
	for(i=0; i<num; i++) {
		g3dimpl_obj_destroy((struct object*)g->meshes[i]);
		free(g->meshes[i]);
	}
	DYNARR_CLEAR(g->meshes);

	num = dynarr_size(g->lights);
	for(i=0; i<num; i++) {
		g3dimpl_obj_destroy((struct object*)g->lights[i]);
		free(g->lights[i]);
	}
	DYNARR_CLEAR(g->lights);

	num = dynarr_size(g->cameras);
	for(i=0; i<num; i++) {
		g3dimpl_obj_destroy((struct object*)g->cameras[i]);
		free(g->cameras[i]);
	}
	DYNARR_CLEAR(g->cameras);

	num = dynarr_size(g->nodes);
	for(i=0; i<num; i++) {
		goat3d_destroy_node(g->nodes[i]);
	}
	DYNARR_CLEAR(g->nodes);

	num = dynarr_size(g->anims);
	for(i=0; i<num; i++) {
		g3dimpl_anim_destroy(g->anims[i]);
	}

	g->name = 0;
	g->bbox_valid = 0;
}

GOAT3DAPI void goat3d_setopt(struct goat3d *g, enum goat3d_option opt, int val)
{
	if(val) {
		g->flags |= (1 << (int)opt);
	} else {
		g->flags &= ~(1 << (int)opt);
	}
}

GOAT3DAPI int goat3d_getopt(const struct goat3d *g, enum goat3d_option opt)
{
	return (g->flags >> (int)opt) & 1;
}

GOAT3DAPI int goat3d_load(struct goat3d *g, const char *fname)
{
	int len, res;
	char *slash;
	FILE *fp = fopen(fname, "rb");
	if(!fp) {
		goat3d_logmsg(LOG_ERROR, "failed to open file \"%s\" for reading: %s\n", fname, strerror(errno));
		return -1;
	}

	/* if the filename contained any directory components, keep the prefix
	 * to use it as a search path for external mesh file loading
	 */
	len = strlen(fname);
	if(!(g->search_path = malloc(len + 1))) {
		fclose(fp);
		return -1;
	}
	memcpy(g->search_path, fname, len + 1);

	if((slash = strrchr(g->search_path, '/'))) {
		*slash = 0;
	} else {
		if((slash = strrchr(g->search_path, '\\'))) {
			*slash = 0;
		} else {
			free(g->search_path);
			g->search_path = 0;
		}
	}

	if((res = goat3d_load_file(g, fp)) == 0) {
		const char *name;
		if((name = goat3d_get_name(g)) == def_scn_name) {
			goat3d_set_name(g, slash ? slash + 1 : fname);
		}
	}
	fclose(fp);
	return res;
}

GOAT3DAPI int goat3d_save(const struct goat3d *g, const char *fname)
{
	int res;
	FILE *fp = fopen(fname, "wb");
	if(!fp) {
		goat3d_logmsg(LOG_ERROR, "failed to open file \"%s\" for writing: %s\n", fname, strerror(errno));
		return -1;
	}

	res = goat3d_save_file(g, fp);
	fclose(fp);
	return res;
}

GOAT3DAPI int goat3d_load_file(struct goat3d *g, FILE *fp)
{
	struct goat3d_io io;
	io.cls = fp;
	io.read = read_file;
	io.write = write_file;
	io.seek = seek_file;

	return goat3d_load_io(g, &io);
}

GOAT3DAPI int goat3d_save_file(const struct goat3d *g, FILE *fp)
{
	struct goat3d_io io;
	io.cls = fp;
	io.read = read_file;
	io.write = write_file;
	io.seek = seek_file;

	return goat3d_save_io(g, &io);
}

GOAT3DAPI int goat3d_load_io(struct goat3d *g, struct goat3d_io *io)
{
	return g3dimpl_scnload(g, io);
}

GOAT3DAPI int goat3d_save_io(const struct goat3d *g, struct goat3d_io *io)
{
	if(goat3d_getopt(g, GOAT3D_OPT_SAVEXML)) {
		goat3d_logmsg(LOG_ERROR, "saving in the original xml format is no longer supported\n");
		return -1;
	} else if(goat3d_getopt(g, GOAT3D_OPT_SAVETEXT)) {
		/* TODO set treestore output format as text */
	}
	return g3dimpl_scnsave(g, io);
}

/* save/load animations */
GOAT3DAPI int goat3d_load_anim(struct goat3d *g, const char *fname)
{
	int res;
	FILE *fp;

	if(!(fp = fopen(fname, "rb"))) {
		return -1;
	}

	res = goat3d_load_anim_file(g, fp);
	fclose(fp);
	return res;
}

GOAT3DAPI int goat3d_save_anim(const struct goat3d *g, const char *fname)
{
	int res;
	FILE *fp;

	if(!(fp = fopen(fname, "wb"))) {
		return -1;
	}

	res = goat3d_save_anim_file(g, fp);
	fclose(fp);
	return res;
}

GOAT3DAPI int goat3d_load_anim_file(struct goat3d *g, FILE *fp)
{
	struct goat3d_io io;
	io.cls = fp;
	io.read = read_file;
	io.write = write_file;
	io.seek = seek_file;

	return goat3d_load_anim_io(g, &io);
}

GOAT3DAPI int goat3d_save_anim_file(const struct goat3d *g, FILE *fp)
{
	struct goat3d_io io;
	io.cls = fp;
	io.read = read_file;
	io.write = write_file;
	io.seek = seek_file;

	return goat3d_save_anim_io(g, &io);
}

GOAT3DAPI int goat3d_load_anim_io(struct goat3d *g, struct goat3d_io *io)
{
	return g3dimpl_anmload(g, io);
}

GOAT3DAPI int goat3d_save_anim_io(const struct goat3d *g, struct goat3d_io *io)
{
	if(goat3d_getopt(g, GOAT3D_OPT_SAVEXML)) {
		goat3d_logmsg(LOG_ERROR, "saving in the original xml format is no longer supported\n");
		return -1;
	} else if(goat3d_getopt(g, GOAT3D_OPT_SAVETEXT)) {
		/* TODO set treestore save format as text */
	}
	return g3dimpl_anmsave(g, io);
}


GOAT3DAPI int goat3d_set_name(struct goat3d *g, const char *name)
{
	int len = strlen(name);

	free(g->name);
	if(!(g->name = malloc(len + 1))) {
		return -1;
	}
	memcpy(g->name, name, len + 1);
	return 0;
}

GOAT3DAPI const char *goat3d_get_name(const struct goat3d *g)
{
	return g->name ? g->name : def_scn_name;
}

GOAT3DAPI void goat3d_set_ambient(struct goat3d *g, const float *amb)
{
	cgm_vcons(&g->ambient, amb[0], amb[1], amb[2]);
}

GOAT3DAPI void goat3d_set_ambient3f(struct goat3d *g, float ar, float ag, float ab)
{
	cgm_vcons(&g->ambient, ar, ag, ab);
}

GOAT3DAPI const float *goat3d_get_ambient(const struct goat3d *g)
{
	return &g->ambient.x;
}

GOAT3DAPI int goat3d_get_bounds(const struct goat3d *g, float *bmin, float *bmax)
{
	int i, num_nodes, num_meshes;
	struct aabox bbox;

	if(!g->bbox_valid) {
		g3dimpl_aabox_init((struct aabox*)&g->bbox);

		if(dynarr_empty(g->nodes)) {
use_mesh_bounds:
			num_meshes = dynarr_size(g->meshes);
			for(i=0; i<num_meshes; i++) {
				g3dimpl_mesh_bounds(&bbox, g->meshes[i], 0);
				g3dimpl_aabox_union((struct aabox*)&g->bbox, &g->bbox, &bbox);
			}
		} else {
			num_nodes = dynarr_size(g->nodes);
			for(i=0; i<num_nodes; i++) {
				if(g->nodes[i]->parent) {
					continue;
				}
				g3dimpl_node_bounds(&bbox, g->nodes[i]);
				g3dimpl_aabox_union((struct aabox*)&g->bbox, &g->bbox, &bbox);
			}

			/* in case the nodes are junk */
			if(g->bbox.bmin.x > g->bbox.bmax.x) {
				goto use_mesh_bounds;
			}
		}
		((struct goat3d*)g)->bbox_valid = 1;
	}

	if(g->bbox.bmin.x > g->bbox.bmax.x) {
		return -1;
	}

	bmin[0] = g->bbox.bmin.x;
	bmin[1] = g->bbox.bmin.y;
	bmin[2] = g->bbox.bmin.z;
	bmax[0] = g->bbox.bmax.x;
	bmax[1] = g->bbox.bmax.y;
	bmax[2] = g->bbox.bmax.z;
	return 0;
}

// ---- materials ----
GOAT3DAPI int goat3d_add_mtl(struct goat3d *g, struct goat3d_material *mtl)
{
	struct goat3d_material **newarr;
	mtl->idx = dynarr_size(g->materials);
	if(!(newarr = dynarr_push(g->materials, &mtl))) {
		return -1;
	}
	g->materials = newarr;
	return 0;
}

GOAT3DAPI int goat3d_get_mtl_count(struct goat3d *g)
{
	return dynarr_size(g->materials);
}

GOAT3DAPI struct goat3d_material *goat3d_get_mtl(struct goat3d *g, int idx)
{
	return g->materials[idx];
}

GOAT3DAPI struct goat3d_material *goat3d_get_mtl_by_name(struct goat3d *g, const char *name)
{
	int i, num = dynarr_size(g->materials);
	for(i=0; i<num; i++) {
		if(strcmp(g->materials[i]->name, name) == 0) {
			return g->materials[i];
		}
	}
	return 0;
}

GOAT3DAPI struct goat3d_material *goat3d_create_mtl(void)
{
	struct goat3d_material *mtl;
	if(!(mtl = malloc(sizeof *mtl))) {
		return 0;
	}
	g3dimpl_mtl_init(mtl);
	return mtl;
}

GOAT3DAPI void goat3d_destroy_mtl(struct goat3d_material *mtl)
{
	g3dimpl_mtl_destroy(mtl);
	free(mtl);
}

GOAT3DAPI int goat3d_set_mtl_name(struct goat3d_material *mtl, const char *name)
{
	SETNAME(mtl->name, name);
}

GOAT3DAPI const char *goat3d_get_mtl_name(const struct goat3d_material *mtl)
{
	return mtl->name;
}

GOAT3DAPI int goat3d_set_mtl_attrib(struct goat3d_material *mtl, const char *attrib, const float *val)
{
	struct material_attrib *ma = g3dimpl_mtl_getattr(mtl, attrib);
	if(!ma) return -1;
	cgm_wcons(&ma->value, val[0], val[1], val[2], val[3]);
	return 0;
}

GOAT3DAPI int goat3d_set_mtl_attrib1f(struct goat3d_material *mtl, const char *attrib, float val)
{
	return goat3d_set_mtl_attrib4f(mtl, attrib, val, 0, 0, 1);
}

GOAT3DAPI int goat3d_set_mtl_attrib3f(struct goat3d_material *mtl, const char *attrib, float r, float g, float b)
{
	return goat3d_set_mtl_attrib4f(mtl, attrib, r, g, b, 1);
}

GOAT3DAPI int goat3d_set_mtl_attrib4f(struct goat3d_material *mtl, const char *attrib, float r, float g, float b, float a)
{
	struct material_attrib *ma = g3dimpl_mtl_getattr(mtl, attrib);
	if(!ma) return -1;
	cgm_wcons(&ma->value, r, g, b, a);
	return 0;
}

GOAT3DAPI const float *goat3d_get_mtl_attrib(struct goat3d_material *mtl, const char *attrib)
{
	struct material_attrib *ma = g3dimpl_mtl_findattr(mtl, attrib);
	return ma ? &ma->value.x : 0;
}

GOAT3DAPI int goat3d_set_mtl_attrib_map(struct goat3d_material *mtl, const char *attrib, const char *mapname)
{
	int len;
	char *tmp;
	struct material_attrib *ma;

	len = strlen(mapname);
	if(!(tmp = malloc(len + 1))) {
		return -1;
	}
	memcpy(tmp, mapname, len + 1);

	if(!(ma = g3dimpl_mtl_getattr(mtl, attrib))) {
		free(tmp);
		return -1;
	}
	free(ma->map);
	ma->map = tmp;
	tmp = clean_filename(ma->map);
	if(tmp != ma->map) {
		memmove(ma->map, tmp, len - (tmp - ma->map) + 1);
	}
	return 0;
}

GOAT3DAPI const char *goat3d_get_mtl_attrib_map(struct goat3d_material *mtl, const char *attrib)
{
	struct material_attrib *ma = g3dimpl_mtl_findattr(mtl, attrib);
	return ma->map;
}

GOAT3DAPI const char *goat3d_get_mtl_attrib_name(struct goat3d_material *mtl, int idx)
{
	return mtl->attrib[idx].name;
}

GOAT3DAPI int goat3d_get_mtl_attrib_count(struct goat3d_material *mtl)
{
	return dynarr_size(mtl->attrib);
}

// ---- meshes ----
GOAT3DAPI int goat3d_add_mesh(struct goat3d *g, struct goat3d_mesh *mesh)
{
	struct goat3d_mesh **arr;
	if(!(arr = dynarr_push(g->meshes, &mesh))) {
		return -1;
	}
	g->meshes = arr;
	return 0;
}

GOAT3DAPI int goat3d_get_mesh_count(struct goat3d *g)
{
	return dynarr_size(g->meshes);
}

GOAT3DAPI struct goat3d_mesh *goat3d_get_mesh(struct goat3d *g, int idx)
{
	return g->meshes[idx];
}

GOAT3DAPI struct goat3d_mesh *goat3d_get_mesh_by_name(struct goat3d *g, const char *name)
{
	int i, num = dynarr_size(g->meshes);
	for(i=0; i<num; i++) {
		if(strcmp(g->meshes[i]->name, name) == 0) {
			return g->meshes[i];
		}
	}
	return 0;
}

GOAT3DAPI struct goat3d_mesh *goat3d_create_mesh(void)
{
	struct goat3d_mesh *m;

	if(!(m = malloc(sizeof *m))) {
		return 0;
	}
	if(g3dimpl_obj_init((struct object*)m, OBJTYPE_MESH) == -1) {
		free(m);
		return 0;
	}
	return m;
}

GOAT3DAPI void goat3d_destroy_mesh(struct goat3d_mesh *mesh)
{
	g3dimpl_obj_destroy((struct object*)mesh);
	free(mesh);
}

GOAT3DAPI int goat3d_set_mesh_name(struct goat3d_mesh *mesh, const char *name)
{
	SETNAME(mesh->name, name);
}

GOAT3DAPI const char *goat3d_get_mesh_name(const struct goat3d_mesh *mesh)
{
	return mesh->name;
}

GOAT3DAPI void goat3d_set_mesh_mtl(struct goat3d_mesh *mesh, struct goat3d_material *mtl)
{
	mesh->mtl = mtl;
}

GOAT3DAPI struct goat3d_material *goat3d_get_mesh_mtl(struct goat3d_mesh *mesh)
{
	return mesh->mtl;
}

GOAT3DAPI int goat3d_get_mesh_vertex_count(struct goat3d_mesh *mesh)
{
	return dynarr_size(mesh->vertices);
}

GOAT3DAPI int goat3d_get_mesh_attrib_count(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib)
{
	switch(attrib) {
	case GOAT3D_MESH_ATTR_VERTEX:
		return dynarr_size(mesh->vertices);
	case GOAT3D_MESH_ATTR_NORMAL:
		return dynarr_size(mesh->normals);
	case GOAT3D_MESH_ATTR_TANGENT:
		return dynarr_size(mesh->tangents);
	case GOAT3D_MESH_ATTR_TEXCOORD:
		return dynarr_size(mesh->texcoords);
	case GOAT3D_MESH_ATTR_SKIN_WEIGHT:
		return dynarr_size(mesh->skin_weights);
	case GOAT3D_MESH_ATTR_SKIN_MATRIX:
		return dynarr_size(mesh->skin_matrices);
	case GOAT3D_MESH_ATTR_COLOR:
		return dynarr_size(mesh->colors);
	default:
		break;
	}
	return 0;
}

GOAT3DAPI int goat3d_get_mesh_face_count(struct goat3d_mesh *mesh)
{
	return dynarr_size(mesh->faces);
}

#define SET_VERTEX_DATA(arr, p, n) \
	do { \
		void *tmp = dynarr_resize(arr, n); \
		if(!tmp) { \
			goat3d_logmsg(LOG_ERROR, "failed to resize vertex array (%d)\n", n); \
			return -1; \
		} \
		arr = tmp; \
		memcpy(arr, p, n * sizeof *arr); \
	} while(0)

GOAT3DAPI int goat3d_set_mesh_attribs(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib, const void *data, int vnum)
{
	if(attrib == GOAT3D_MESH_ATTR_VERTEX) {
		SET_VERTEX_DATA(mesh->vertices, data, vnum);
		return 0;
	}

	if(vnum != dynarr_size(mesh->vertices)) {
		goat3d_logmsg(LOG_ERROR, "trying to set mesh attrib data with number of elements different than the vertex array\n");
		return -1;
	}

	switch(attrib) {
	case GOAT3D_MESH_ATTR_NORMAL:
		SET_VERTEX_DATA(mesh->normals, data, vnum);
		break;
	case GOAT3D_MESH_ATTR_TANGENT:
		SET_VERTEX_DATA(mesh->tangents, data, vnum);
		break;
	case GOAT3D_MESH_ATTR_TEXCOORD:
		SET_VERTEX_DATA(mesh->texcoords, data, vnum);
		break;
	case GOAT3D_MESH_ATTR_SKIN_WEIGHT:
		SET_VERTEX_DATA(mesh->skin_weights, data, vnum);
		break;
	case GOAT3D_MESH_ATTR_SKIN_MATRIX:
		SET_VERTEX_DATA(mesh->skin_matrices, data, vnum);
		break;
	case GOAT3D_MESH_ATTR_COLOR:
		SET_VERTEX_DATA(mesh->colors, data, vnum);
	default:
		goat3d_logmsg(LOG_ERROR, "trying to set unknown vertex attrib: %d\n", attrib);
		return -1;
	}
	return 0;
}

GOAT3DAPI int goat3d_add_mesh_attrib1f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		float val)
{
	return goat3d_add_mesh_attrib4f(mesh, attrib, val, 0, 0, 1);
}

GOAT3DAPI int goat3d_add_mesh_attrib2f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		float x, float y)
{
	return goat3d_add_mesh_attrib4f(mesh, attrib, x, y, 0, 1);
}

GOAT3DAPI int goat3d_add_mesh_attrib3f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		float x, float y, float z)
{
	return goat3d_add_mesh_attrib4f(mesh, attrib, x, y, z, 1);
}

GOAT3DAPI int goat3d_add_mesh_attrib4f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		float x, float y, float z, float w)
{
	float vec[4];
	int4 intvec;
	void *tmp;

	switch(attrib) {
	case GOAT3D_MESH_ATTR_VERTEX:
		cgm_vcons((cgm_vec3*)vec, x, y, z);
		if(!(tmp = dynarr_push(mesh->vertices, vec))) {
			goto err;
		}
		mesh->vertices = tmp;
		break;

	case GOAT3D_MESH_ATTR_NORMAL:
		cgm_vcons((cgm_vec3*)vec, x, y, z);
		if(!(tmp = dynarr_push(mesh->normals, vec))) {
			goto err;
		}
		mesh->normals = tmp;
		break;

	case GOAT3D_MESH_ATTR_TANGENT:
		cgm_vcons((cgm_vec3*)vec, x, y, z);
		if(!(tmp = dynarr_push(mesh->tangents, vec))) {
			goto err;
		}
		mesh->tangents = tmp;
		break;

	case GOAT3D_MESH_ATTR_TEXCOORD:
		cgm_vcons((cgm_vec3*)vec, x, y, 0);
		if(!(tmp = dynarr_push(mesh->texcoords, vec))) {
			goto err;
		}
		mesh->texcoords = tmp;
		break;

	case GOAT3D_MESH_ATTR_SKIN_WEIGHT:
		cgm_wcons((cgm_vec4*)vec, x, y, z, w);
		if(!(tmp = dynarr_push(mesh->skin_weights, vec))) {
			goto err;
		}
		mesh->skin_weights = tmp;
		break;

	case GOAT3D_MESH_ATTR_SKIN_MATRIX:
		intvec.x = x;
		intvec.y = y;
		intvec.z = z;
		intvec.w = w;
		if(!(tmp = dynarr_push(mesh->skin_matrices, &intvec))) {
			goto err;
		}
		mesh->skin_matrices = tmp;
		break;

	case GOAT3D_MESH_ATTR_COLOR:
		cgm_wcons((cgm_vec4*)vec, x, y, z, w);
		if(!(tmp = dynarr_push(mesh->colors, vec))) {
			goto err;
		}
		mesh->colors = tmp;

	default:
		goat3d_logmsg(LOG_ERROR, "trying to add unknown vertex attrib: %d\n", attrib);
		return -1;
	}
	return 0;

err:
	goat3d_logmsg(LOG_ERROR, "failed to push vertex attrib\n");
	return -1;
}

GOAT3DAPI void *goat3d_get_mesh_attribs(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib)
{
	return goat3d_get_mesh_attrib(mesh, attrib, 0);
}

GOAT3DAPI void *goat3d_get_mesh_attrib(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib, int idx)
{
	switch(attrib) {
	case GOAT3D_MESH_ATTR_VERTEX:
		return dynarr_empty(mesh->vertices) ? 0 : mesh->vertices + idx;
	case GOAT3D_MESH_ATTR_NORMAL:
		return dynarr_empty(mesh->normals) ? 0 : mesh->normals + idx;
	case GOAT3D_MESH_ATTR_TANGENT:
		return dynarr_empty(mesh->tangents) ? 0 : mesh->tangents + idx;
	case GOAT3D_MESH_ATTR_TEXCOORD:
		return dynarr_empty(mesh->texcoords) ? 0 : mesh->texcoords + idx;
	case GOAT3D_MESH_ATTR_SKIN_WEIGHT:
		return dynarr_empty(mesh->skin_weights) ? 0 : mesh->skin_weights + idx;
	case GOAT3D_MESH_ATTR_SKIN_MATRIX:
		return dynarr_empty(mesh->skin_matrices) ? 0 : mesh->skin_matrices + idx;
	case GOAT3D_MESH_ATTR_COLOR:
		return dynarr_empty(mesh->colors) ? 0 : mesh->colors + idx;
	default:
		break;
	}
	return 0;
}


GOAT3DAPI int goat3d_set_mesh_faces(struct goat3d_mesh *mesh, const int *data, int num)
{
	void *tmp;
	if(!(tmp = dynarr_resize(mesh->faces, num))) {
		goat3d_logmsg(LOG_ERROR, "failed to resize face array (%d)\n", num);
		return -1;
	}
	mesh->faces = tmp;
	memcpy(mesh->faces, data, num * sizeof *mesh->faces);
	return 0;
}

GOAT3DAPI int goat3d_add_mesh_face(struct goat3d_mesh *mesh, int a, int b, int c)
{
	void *tmp;
	struct face face;

	face.v[0] = a;
	face.v[1] = b;
	face.v[2] = c;

	if(!(tmp = dynarr_push(mesh->faces, &face))) {
		goat3d_logmsg(LOG_ERROR, "failed to add face\n");
		return -1;
	}
	mesh->faces = tmp;
	return 0;
}

GOAT3DAPI int *goat3d_get_mesh_faces(struct goat3d_mesh *mesh)
{
	return goat3d_get_mesh_face(mesh, 0);
}

GOAT3DAPI int *goat3d_get_mesh_face(struct goat3d_mesh *mesh, int idx)
{
	return dynarr_empty(mesh->faces) ? 0 : mesh->faces[idx].v;
}

// immedate mode state
static enum goat3d_im_primitive im_prim;
static struct goat3d_mesh *im_mesh;
static cgm_vec3 im_norm, im_tang;
static cgm_vec2 im_texcoord;
static cgm_vec4 im_skinw, im_color = {1, 1, 1, 1};
static int4 im_skinmat;
static int im_use[NUM_GOAT3D_MESH_ATTRIBS];


GOAT3DAPI void goat3d_begin(struct goat3d_mesh *mesh, enum goat3d_im_primitive prim)
{
	DYNARR_CLEAR(mesh->vertices);
	DYNARR_CLEAR(mesh->normals);
	DYNARR_CLEAR(mesh->tangents);
	DYNARR_CLEAR(mesh->texcoords);
	DYNARR_CLEAR(mesh->skin_weights);
	DYNARR_CLEAR(mesh->skin_matrices);
	DYNARR_CLEAR(mesh->colors);
	DYNARR_CLEAR(mesh->faces);

	im_mesh = mesh;
	memset(im_use, 0, sizeof im_use);

	im_prim = prim;
}

GOAT3DAPI void goat3d_end(void)
{
	int i, vidx, num_faces, num_quads;
	void *tmp;

	switch(im_prim) {
	case GOAT3D_TRIANGLES:
		{
			num_faces = dynarr_size(im_mesh->vertices) / 3;
			if(!(tmp = dynarr_resize(im_mesh->faces, num_faces))) {
				return;
			}
			im_mesh->faces = tmp;

			vidx = 0;
			for(i=0; i<num_faces; i++) {
				im_mesh->faces[i].v[0] = vidx++;
				im_mesh->faces[i].v[1] = vidx++;
				im_mesh->faces[i].v[2] = vidx++;
			}
		}
		break;

	case GOAT3D_QUADS:
		{
			num_quads = dynarr_size(im_mesh->vertices) / 4;
			if(!(tmp = dynarr_resize(im_mesh->faces, num_quads * 2))) {
				return;
			}
			im_mesh->faces = tmp;

			vidx = 0;
			for(i=0; i<num_quads; i++) {
				im_mesh->faces[i * 2].v[0] = vidx;
				im_mesh->faces[i * 2].v[1] = vidx + 1;
				im_mesh->faces[i * 2].v[2] = vidx + 2;

				im_mesh->faces[i * 2 + 1].v[0] = vidx;
				im_mesh->faces[i * 2 + 1].v[1] = vidx + 2;
				im_mesh->faces[i * 2 + 1].v[2] = vidx + 3;

				vidx += 4;
			}
		}
		break;

	default:
		break;
	}
}

GOAT3DAPI void goat3d_vertex3f(float x, float y, float z)
{
	void *tmp;
	cgm_vec3 v;

	cgm_vcons(&v, x, y, z);
	if(!(tmp = dynarr_push(im_mesh->vertices, &v))) {
		return;
	}
	im_mesh->vertices = tmp;

	if(im_use[GOAT3D_MESH_ATTR_NORMAL]) {
		if((tmp = dynarr_push(im_mesh->normals, &im_norm))) {
			im_mesh->normals = tmp;
		}
	}
	if(im_use[GOAT3D_MESH_ATTR_TANGENT]) {
		if((tmp = dynarr_push(im_mesh->tangents, &im_tang))) {
			im_mesh->tangents = tmp;
		}
	}
	if(im_use[GOAT3D_MESH_ATTR_TEXCOORD]) {
		if((tmp = dynarr_push(im_mesh->texcoords, &im_texcoord))) {
			im_mesh->texcoords = tmp;
		}
	}
	if(im_use[GOAT3D_MESH_ATTR_SKIN_WEIGHT]) {
		if((tmp = dynarr_push(im_mesh->skin_weights, &im_skinw))) {
			im_mesh->skin_weights = tmp;
		}
	}
	if(im_use[GOAT3D_MESH_ATTR_SKIN_MATRIX]) {
		if((tmp = dynarr_push(im_mesh->skin_matrices, &im_skinmat))) {
			im_mesh->skin_matrices = tmp;
		}
	}
	if(im_use[GOAT3D_MESH_ATTR_COLOR]) {
		if((tmp = dynarr_push(im_mesh->colors, &im_color))) {
			im_mesh->colors = tmp;
		}
	}
}

GOAT3DAPI void goat3d_normal3f(float x, float y, float z)
{
	cgm_vcons(&im_norm, x, y, z);
	im_use[GOAT3D_MESH_ATTR_NORMAL] = 1;
}

GOAT3DAPI void goat3d_tangent3f(float x, float y, float z)
{
	cgm_vcons(&im_tang, x, y, z);
	im_use[GOAT3D_MESH_ATTR_TANGENT] = 1;
}

GOAT3DAPI void goat3d_texcoord2f(float x, float y)
{
	im_texcoord.x = x;
	im_texcoord.y = y;
	im_use[GOAT3D_MESH_ATTR_TEXCOORD] = 1;
}

GOAT3DAPI void goat3d_skin_weight4f(float x, float y, float z, float w)
{
	cgm_wcons(&im_skinw, x, y, z, w);
	im_use[GOAT3D_MESH_ATTR_SKIN_WEIGHT] = 1;
}

GOAT3DAPI void goat3d_skin_matrix4i(int x, int y, int z, int w)
{
	im_skinmat.x = x;
	im_skinmat.y = y;
	im_skinmat.z = z;
	im_skinmat.w = w;
	im_use[GOAT3D_MESH_ATTR_SKIN_MATRIX] = 1;
}

GOAT3DAPI void goat3d_color3f(float x, float y, float z)
{
	goat3d_color4f(x, y, z, 1.0f);
}

GOAT3DAPI void goat3d_color4f(float x, float y, float z, float w)
{
	cgm_wcons(&im_color, x, y, z, w);
	im_use[GOAT3D_MESH_ATTR_COLOR] = 1;
}

GOAT3DAPI void goat3d_get_mesh_bounds(const struct goat3d_mesh *mesh, float *bmin, float *bmax)
{
	struct aabox box;

	g3dimpl_mesh_bounds(&box, (struct goat3d_mesh*)mesh, 0);

	bmin[0] = box.bmin.x;
	bmin[1] = box.bmin.y;
	bmin[2] = box.bmin.z;
	bmax[0] = box.bmax.x;
	bmax[1] = box.bmax.y;
	bmax[2] = box.bmax.z;
}

/* lights */
GOAT3DAPI int goat3d_add_light(struct goat3d *g, struct goat3d_light *lt)
{
	struct goat3d_light **arr;
	if(!(arr = dynarr_push(g->lights, &lt))) {
		return -1;
	}
	g->lights = arr;
	return 0;
}

GOAT3DAPI int goat3d_get_light_count(struct goat3d *g)
{
	return dynarr_size(g->lights);
}

GOAT3DAPI struct goat3d_light *goat3d_get_light(struct goat3d *g, int idx)
{
	return g->lights[idx];
}

GOAT3DAPI struct goat3d_light *goat3d_get_light_by_name(struct goat3d *g, const char *name)
{
	int i, num = dynarr_size(g->lights);
	for(i=0; i<num; i++) {
		if(strcmp(g->lights[i]->name, name) == 0) {
			return g->lights[i];
		}
	}
	return 0;
}


GOAT3DAPI struct goat3d_light *goat3d_create_light(void)
{
	struct goat3d_light *lt;

	if(!(lt = malloc(sizeof *lt))) {
		return 0;
	}
	if(g3dimpl_obj_init((struct object*)lt, OBJTYPE_LIGHT) == -1) {
		free(lt);
		return 0;
	}
	return lt;
}

GOAT3DAPI void goat3d_destroy_light(struct goat3d_light *lt)
{
	g3dimpl_obj_destroy((struct object*)lt);
	free(lt);
}

GOAT3DAPI int goat3d_set_light_name(struct goat3d_light *lt, const char *name)
{
	SETNAME(lt->name, name);
}

GOAT3DAPI const char *goat3d_get_light_name(const struct goat3d_light *lt)
{
	return lt->name;
}

/* cameras */
GOAT3DAPI int goat3d_add_camera(struct goat3d *g, struct goat3d_camera *cam)
{
	struct goat3d_camera **arr;
	if(!(arr = dynarr_push(g->cameras, &cam))) {
		return -1;
	}
	g->cameras = arr;
	return 0;
}

GOAT3DAPI int goat3d_get_camera_count(struct goat3d *g)
{
	return dynarr_size(g->cameras);
}

GOAT3DAPI struct goat3d_camera *goat3d_get_camera(struct goat3d *g, int idx)
{
	return g->cameras[idx];
}

GOAT3DAPI struct goat3d_camera *goat3d_get_camera_by_name(struct goat3d *g, const char *name)
{
	int i, num = dynarr_size(g->cameras);
	for(i=0; i<num; i++) {
		if(strcmp(g->cameras[i]->name, name) == 0) {
			return g->cameras[i];
		}
	}
	return 0;
}

GOAT3DAPI struct goat3d_camera *goat3d_create_camera(void)
{
	struct goat3d_camera *cam;

	if(!(cam = malloc(sizeof *cam))) {
		return 0;
	}
	if(g3dimpl_obj_init((struct object*)cam, OBJTYPE_CAMERA) == -1) {
		free(cam);
		return 0;
	}
	return cam;
}

GOAT3DAPI void goat3d_destroy_camera(struct goat3d_camera *cam)
{
	g3dimpl_obj_destroy((struct object*)cam);
	free(cam);
}

GOAT3DAPI int goat3d_set_camera_name(struct goat3d_camera *cam, const char *name)
{
	SETNAME(cam->name, name);
}

GOAT3DAPI const char *goat3d_get_camera_name(const struct goat3d_camera *cam)
{
	return cam->name;
}

/* node */
GOAT3DAPI int goat3d_add_node(struct goat3d *g, struct goat3d_node *node)
{
	struct goat3d_node **arr;
	if(!(arr = dynarr_push(g->nodes, &node))) {
		return -1;
	}
	g->nodes = arr;
	return 0;
}

GOAT3DAPI int goat3d_get_node_count(struct goat3d *g)
{
	return dynarr_size(g->nodes);
}

GOAT3DAPI struct goat3d_node *goat3d_get_node(struct goat3d *g, int idx)
{
	return g->nodes[idx];
}

GOAT3DAPI struct goat3d_node *goat3d_get_node_by_name(struct goat3d *g, const char *name)
{
	int i, num = dynarr_size(g->nodes);
	for(i=0; i<num; i++) {
		if(strcmp(g->nodes[i]->name, name) == 0) {
			return g->nodes[i];
		}
	}
	return 0;
}

GOAT3DAPI struct goat3d_node *goat3d_create_node(void)
{
	struct goat3d_node *node;

	if(!(node = calloc(1, sizeof *node))) {
		return 0;
	}
	node->type = GOAT3D_NODE_NULL;
	node->obj = 0;
	node->child_count = 0;

	node->rot.w = node->arot.w = 1;
	cgm_vcons(&node->scale, 1, 1, 1);
	cgm_midentity(node->matrix);

	return node;
}

GOAT3DAPI void goat3d_destroy_node(struct goat3d_node *node)
{
	if(!node) return;
	free(node->name);
	free(node);
}

GOAT3DAPI int goat3d_set_node_name(struct goat3d_node *node, const char *name)
{
	SETNAME(node->name, name);
}

GOAT3DAPI const char *goat3d_get_node_name(const struct goat3d_node *node)
{
	return node->name;
}

GOAT3DAPI void goat3d_set_node_object(struct goat3d_node *node, enum goat3d_node_type type, void *obj)
{
	node->obj = obj;
	node->type = type;
}

GOAT3DAPI void *goat3d_get_node_object(const struct goat3d_node *node)
{
	return node->obj;
}

GOAT3DAPI enum goat3d_node_type goat3d_get_node_type(const struct goat3d_node *node)
{
	return node->type;
}

GOAT3DAPI void goat3d_add_node_child(struct goat3d_node *node, struct goat3d_node *child)
{
	child->next = node->child;
	node->child = child;
	child->parent = node;
	node->child_count++;

	child->matrix_valid = 0;
}

GOAT3DAPI int goat3d_get_node_child_count(const struct goat3d_node *node)
{
	return node->child_count;
}

GOAT3DAPI struct goat3d_node *goat3d_get_node_child(const struct goat3d_node *node, int idx)
{
	struct goat3d_node *c = node->child;
	while(c && idx-- > 0) {
		c = c->next;
	}
	return c;
}

GOAT3DAPI struct goat3d_node *goat3d_get_node_parent(const struct goat3d_node *node)
{
	return node->parent;
}

static void invalidate_subtree(struct goat3d_node *node)
{
	struct goat3d_node *c = node->child;

	while(c) {
		invalidate_subtree(c);
		c = c->next;
	}
	node->matrix_valid = 0;
}


GOAT3DAPI void goat3d_set_node_position(struct goat3d_node *node, float x, float y, float z)
{
	cgm_vcons(&node->pos, x, y, z);
	invalidate_subtree(node);
}

GOAT3DAPI void goat3d_set_node_rotation(struct goat3d_node *node, float qx, float qy, float qz, float qw)
{
	cgm_qcons(&node->rot, qx, qy, qz, qw);
	invalidate_subtree(node);
}

GOAT3DAPI void goat3d_set_node_scaling(struct goat3d_node *node, float sx, float sy, float sz)
{
	cgm_vcons(&node->scale, sx, sy, sz);
	invalidate_subtree(node);
}

GOAT3DAPI void goat3d_get_node_position(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr)
{
	if(node->has_anim) {
		*xptr = node->apos.x;
		*yptr = node->apos.y;
		*zptr = node->apos.z;
	} else {
		*xptr = node->pos.x;
		*yptr = node->pos.y;
		*zptr = node->pos.z;
	}
}

GOAT3DAPI void goat3d_get_node_rotation(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr, float *wptr)
{
	if(node->has_anim) {
		*xptr = node->arot.x;
		*yptr = node->arot.y;
		*zptr = node->arot.z;
		*wptr = node->arot.w;
	} else {
		*xptr = node->rot.x;
		*yptr = node->rot.y;
		*zptr = node->rot.z;
		*wptr = node->rot.w;
	}
}

GOAT3DAPI void goat3d_get_node_scaling(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr)
{
	if(node->has_anim) {
		*xptr = node->ascale.x;
		*yptr = node->ascale.y;
		*zptr = node->ascale.z;
	} else {
		*xptr = node->scale.x;
		*yptr = node->scale.y;
		*zptr = node->scale.z;
	}
}


GOAT3DAPI void goat3d_set_node_pivot(struct goat3d_node *node, float px, float py, float pz)
{
	cgm_vcons(&node->pivot, px, py, pz);
	invalidate_subtree(node);
}

GOAT3DAPI void goat3d_get_node_pivot(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr)
{
	*xptr = node->pivot.x;
	*yptr = node->pivot.y;
	*zptr = node->pivot.z;
}

static void calc_node_matrix(const struct goat3d_node *node, float *mat)
{
	int i;
	float rmat[16];
	cgm_vec3 pos, scale;
	cgm_quat rot;

	if(node->has_anim) {
		pos = node->apos;
		rot = node->arot;
		scale = node->ascale;
	} else {
		pos = node->pos;
		rot = node->rot;
		scale = node->scale;
	}

	cgm_mtranslation(mat, node->pivot.x, node->pivot.y, node->pivot.z);
	cgm_mrotation_quat(rmat, &rot);

	for(i=0; i<3; i++) {
		mat[i] = rmat[i];
		mat[4 + i] = rmat[4 + i];
		mat[8 + i] = rmat[8 + i];
	}

	mat[0] *= scale.x; mat[4] *= scale.y; mat[8] *= scale.z; mat[12] += pos.x;
	mat[1] *= scale.x; mat[5] *= scale.y; mat[9] *= scale.z; mat[13] += pos.y;
	mat[2] *= scale.x; mat[6] *= scale.y; mat[10] *= scale.z; mat[14] += pos.z;

	cgm_mpretranslate(mat, -node->pivot.x, -node->pivot.y, -node->pivot.z);

	/* that's basically: pivot * rotation * translation * scaling * -pivot */
}

GOAT3DAPI void goat3d_get_node_matrix(const struct goat3d_node *node, float *matrix)
{
	if(!node->matrix_valid) {
		calc_node_matrix(node, (float*)node->matrix);
		((struct goat3d_node*)node)->matrix_valid = 1;
	}
	memcpy(matrix, node->matrix, sizeof node->matrix);
}

GOAT3DAPI void goat3d_get_matrix(const struct goat3d_node *node, float *matrix)
{
	goat3d_get_node_matrix(node, matrix);
	if(node->parent) {
		cgm_mmul(matrix, node->parent->matrix);
	}
}

GOAT3DAPI void goat3d_get_node_bounds(const struct goat3d_node *node, float *bmin, float *bmax)
{
	struct aabox box;
	g3dimpl_node_bounds(&box, (struct goat3d_node*)node);

	bmin[0] = box.bmin.x;
	bmin[1] = box.bmin.y;
	bmin[2] = box.bmin.z;
	bmax[0] = box.bmax.x;
	bmax[1] = box.bmax.y;
	bmax[2] = box.bmax.z;
}


/* tracks */
#define BASETYPE(type)	((int)(type) & 0xff)
static const int key_val_sz[] = {1, 3, 4, 4};

GOAT3DAPI struct goat3d_track *goat3d_create_track(void)
{
	int i;
	struct goat3d_track *trk;

	if(!(trk = calloc(1, sizeof *trk))) {
		return 0;
	}

	for(i=0; i<4; i++) {
		if(anm_init_track(trk->trk + i) == -1) {
			while(--i >= 0) {
				anm_destroy_track(trk->trk + i);
			}
			free(trk);
			return 0;
		}
	}

	return trk;
}

GOAT3DAPI void goat3d_destroy_track(struct goat3d_track *trk)
{
	int i;

	if(!trk) return;

	free(trk->name);

	for(i=0; i<4; i++) {
		anm_destroy_track(trk->trk + i);
	}
}

GOAT3DAPI int goat3d_set_track_name(struct goat3d_track *trk, const char *name)
{
	SETNAME(trk->name, name);
}

GOAT3DAPI const char *goat3d_get_track_name(const struct goat3d_track *trk)
{
	return trk->name;
}

GOAT3DAPI void goat3d_set_track_type(struct goat3d_track *trk, enum goat3d_track_type type)
{
	trk->type = type;

	switch(BASETYPE(type)) {
	case GOAT3D_TRACK_QUAT:
	case GOAT3D_TRACK_VEC4:
		anm_set_track_default(trk->trk + 3, 1);
	case GOAT3D_TRACK_VEC3:
		anm_set_track_default(trk->trk + 1, 0);
		anm_set_track_default(trk->trk + 2, 0);
	case GOAT3D_TRACK_VAL:
		anm_set_track_default(trk->trk + 0, 0);
	}
}

GOAT3DAPI enum goat3d_track_type goat3d_get_track_type(const struct goat3d_track *trk)
{
	return trk->type;
}

GOAT3DAPI void goat3d_set_track_node(struct goat3d_track *trk, struct goat3d_node *node)
{
	trk->node = node;
}

GOAT3DAPI struct goat3d_node *goat3d_get_track_node(const struct goat3d_track *trk)
{
	return trk->node;
}

GOAT3DAPI void goat3d_set_track_interp(struct goat3d_track *trk, enum goat3d_interp in)
{
	int i;
	for(i=0; i<4; i++) {
		anm_set_track_interpolator(trk->trk + i, in);
	}
}

GOAT3DAPI enum goat3d_interp goat3d_get_track_interp(const struct goat3d_track *trk)
{
	return trk->trk[0].interp;
}

GOAT3DAPI void goat3d_set_track_extrap(struct goat3d_track *trk, enum goat3d_extrap ex)
{
	int i;
	for(i=0; i<4; i++) {
		anm_set_track_extrapolator(trk->trk + i, ex);
	}
}

GOAT3DAPI enum goat3d_extrap goat3d_get_track_extrap(const struct goat3d_track *trk)
{
	return trk->trk[0].extrap;
}

GOAT3DAPI int goat3d_set_track_key(struct goat3d_track *trk, const struct goat3d_key *key)
{
	int i, num;
	enum goat3d_track_type basetype;
	long tm = ANM_MSEC2TM(key->tm);

	basetype = BASETYPE(trk->type);	/* e.g. ROT -> QUAT */
	num = key_val_sz[basetype];

	for(i=0; i<num; i++) {
		if(anm_set_value(trk->trk + i, tm, key->val[i]) == -1) {
			return -1;
		}
	}
	return 0;
}

GOAT3DAPI int goat3d_get_track_key(const struct goat3d_track *trk, int idx, struct goat3d_key *key)
{
	int i, num;
	struct anm_keyframe *akey;
	enum goat3d_track_type basetype;

	basetype = BASETYPE(trk->type);
	num = key_val_sz[basetype];

	for(i=0; i<num; i++) {
		if(!(akey = anm_get_keyframe(trk->trk + i, idx))) {
			return -1;
		}
		if(i == 0) {
			key->tm = ANM_TM2MSEC(akey->time);
		}
		key->val[i] = akey->val;
	}
	return 0;
}

GOAT3DAPI int goat3d_get_track_key_count(const struct goat3d_track *trk)
{
	return trk->trk[0].count;
}

GOAT3DAPI int goat3d_set_track_val(struct goat3d_track *trk, long msec, float val)
{
	struct goat3d_key key = {0};
	enum goat3d_track_type basetype;

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_VAL) {
		goat3d_logmsg(LOG_WARNING, "goat3d_set_track_val called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return -1;
	}

	key.tm = msec;
	key.val[0] = val;
	return goat3d_set_track_key(trk, &key);
}

GOAT3DAPI int goat3d_set_track_vec3(struct goat3d_track *trk, long msec, float x, float y, float z)
{
	struct goat3d_key key = {0};
	enum goat3d_track_type basetype;

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_VEC3) {
		goat3d_logmsg(LOG_WARNING, "goat3d_set_track_vec3 called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return -1;
	}

	key.tm = msec;
	key.val[0] = x;
	key.val[1] = y;
	key.val[2] = z;
	return goat3d_set_track_key(trk, &key);
}

GOAT3DAPI int goat3d_set_track_vec4(struct goat3d_track *trk, long msec, float x, float y, float z, float w)
{
	struct goat3d_key key = {0};
	enum goat3d_track_type basetype;

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_VEC4) {
		goat3d_logmsg(LOG_WARNING, "goat3d_set_track_vec4 called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return -1;
	}

	key.tm = msec;
	key.val[0] = x;
	key.val[1] = y;
	key.val[2] = z;
	key.val[3] = w;
	return goat3d_set_track_key(trk, &key);
}

GOAT3DAPI int goat3d_set_track_quat(struct goat3d_track *trk, long msec, float x, float y, float z, float w)
{
	struct goat3d_key key = {0};
	enum goat3d_track_type basetype;

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_QUAT) {
		goat3d_logmsg(LOG_WARNING, "goat3d_set_track_quat called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return -1;
	}

	key.tm = msec;
	key.val[0] = x;
	key.val[1] = y;
	key.val[2] = z;
	key.val[3] = w;
	return goat3d_set_track_key(trk, &key);
}


GOAT3DAPI void goat3d_get_track_val(const struct goat3d_track *trk, long msec, float *valp)
{
	enum goat3d_track_type basetype;
	anm_time_t tm = ANM_MSEC2TM(msec);

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_VAL) {
		goat3d_logmsg(LOG_WARNING, "goat3d_get_track_val called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return;
	}

	*valp = anm_get_value(trk->trk, tm);
}

GOAT3DAPI void goat3d_get_track_vec3(const struct goat3d_track *trk, long msec, float *xp, float *yp, float *zp)
{
	enum goat3d_track_type basetype;
	anm_time_t tm = ANM_MSEC2TM(msec);

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_VEC3) {
		goat3d_logmsg(LOG_WARNING, "goat3d_get_track_vec3 called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return;
	}

	*xp = anm_get_value(trk->trk, tm);
	*yp = anm_get_value(trk->trk + 1, tm);
	*zp = anm_get_value(trk->trk + 2, tm);
}

GOAT3DAPI void goat3d_get_track_vec4(const struct goat3d_track *trk, long msec, float *xp, float *yp, float *zp, float *wp)
{
	enum goat3d_track_type basetype;
	anm_time_t tm = ANM_MSEC2TM(msec);

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_VEC4) {
		goat3d_logmsg(LOG_WARNING, "goat3d_get_track_vec4 called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return;
	}

	*xp = anm_get_value(trk->trk, tm);
	*yp = anm_get_value(trk->trk + 1, tm);
	*zp = anm_get_value(trk->trk + 2, tm);
	*wp = anm_get_value(trk->trk + 3, tm);
}

GOAT3DAPI void goat3d_get_track_quat(const struct goat3d_track *trk, long msec, float *xp, float *yp, float *zp, float *wp)
{
	enum goat3d_track_type basetype;
	float quat[4];
	anm_time_t tm = ANM_MSEC2TM(msec);

	basetype = BASETYPE(trk->type);

	if(basetype != GOAT3D_TRACK_QUAT) {
		goat3d_logmsg(LOG_WARNING, "goat3d_get_track_quat called on %s track\n",
				g3dimpl_trktypestr(trk->type));
		return;
	}

	anm_get_quat(trk->trk, trk->trk + 1, trk->trk + 2, trk->trk + 3, tm, quat);
	*xp = quat[0];
	*yp = quat[1];
	*zp = quat[2];
	*wp = quat[3];
}

GOAT3DAPI long goat3d_get_track_timeline(const struct goat3d_track *trk, long *tstart, long *tend)
{
	int i, j, num;
	enum goat3d_track_type basetype;
	struct anm_keyframe *key;
	anm_time_t start = ANM_TIME_MAX;
	anm_time_t end = ANM_TIME_MIN;

	basetype = BASETYPE(trk->type);
	num = key_val_sz[basetype];

	for(i=0; i<num; i++) {
		for(j=0; j<trk->trk[i].count; j++) {
			key = anm_get_keyframe(trk->trk + i, j);
			if(key->time < start) start = key->time;
			if(key->time > end) end = key->time;
		}
	}

	if(end < start) {
		return -1;
	}
	*tstart = ANM_TM2MSEC(start);
	*tend = ANM_TM2MSEC(end);
	return *tend - *tstart;
}

/* animation */
GOAT3DAPI int goat3d_add_anim(struct goat3d *g, struct goat3d_anim *anim)
{
	struct goat3d_anim **arr;
	if(!(arr = dynarr_push(g->anims, &anim))) {
		return -1;
	}
	g->anims = arr;
	return 0;
}

GOAT3DAPI int goat3d_get_anim_count(const struct goat3d *g)
{
	return dynarr_size(g->anims);
}

GOAT3DAPI struct goat3d_anim *goat3d_get_anim(const struct goat3d *g, int idx)
{
	return g->anims[idx];
}

GOAT3DAPI struct goat3d_anim *goat3d_get_anim_by_name(const struct goat3d *g, const char *name)
{
	int i, num = dynarr_size(g->anims);
	for(i=0; i<num; i++) {
		if(strcmp(g->anims[i]->name, name) == 0) {
			return g->anims[i];
		}
	}
	return 0;
}

GOAT3DAPI struct goat3d_anim *goat3d_create_anim(void)
{
	struct goat3d_anim *anim;

	if(!(anim = malloc(sizeof *anim))) {
		return 0;
	}
	if(g3dimpl_anim_init(anim) == -1) {
		free(anim);
		return 0;
	}
	return anim;
}

GOAT3DAPI void goat3d_destroy_anim(struct goat3d_anim *anim)
{
	g3dimpl_anim_destroy(anim);
	free(anim);
}

GOAT3DAPI int goat3d_set_anim_name(struct goat3d_anim *anim, const char *name)
{
	SETNAME(anim->name, name);
}

GOAT3DAPI const char *goat3d_get_anim_name(const struct goat3d_anim *anim)
{
	return anim->name;
}

GOAT3DAPI int goat3d_add_anim_track(struct goat3d_anim *anim, struct goat3d_track *trk)
{
	struct goat3d_track **tmptrk;

	if(!(tmptrk = dynarr_push(anim->tracks, &trk))) {
		return -1;
	}
	anim->tracks = tmptrk;
	return 0;
}

GOAT3DAPI struct goat3d_track *goat3d_get_anim_track(const struct goat3d_anim *anim, int idx)
{
	return anim->tracks[idx];
}

GOAT3DAPI struct goat3d_track *goat3d_get_anim_track_by_name(const struct goat3d_anim *anim, const char *name)
{
	int i, num = dynarr_size(anim->tracks);
	for(i=0; i<num; i++) {
		if(strcmp(anim->tracks[i]->name, name) == 0) {
			return anim->tracks[i];
		}
	}
	return 0;
}

GOAT3DAPI struct goat3d_track *goat3d_get_anim_track_by_type(const struct goat3d_anim *anim, enum goat3d_track_type type)
{
	int i, num = dynarr_size(anim->tracks);
	for(i=0; i<num; i++) {
		if(anim->tracks[i]->type == type) {
			return anim->tracks[i];
		}
	}
	return 0;
}

GOAT3DAPI int goat3d_get_anim_track_count(const struct goat3d_anim *anim)
{
	return dynarr_size(anim->tracks);
}

GOAT3DAPI long goat3d_get_anim_timeline(const struct goat3d_anim *anim, long *tstart, long *tend)
{
	int i, num = dynarr_size(anim->tracks);
	long start, end, trkstart, trkend;

	start = LONG_MAX;
	end = LONG_MIN;

	for(i=0; i<num; i++) {
		if(goat3d_get_track_timeline(anim->tracks[i], &trkstart, &trkend) != -1) {
			if(trkstart < start) start = trkstart;
			if(trkend > end) end = trkend;
		}
	}

	if(end < start) {
		return -1;
	}

	*tstart = start;
	*tend = end;
	return end - start;
}




static long read_file(void *buf, size_t bytes, void *uptr)
{
	return (long)fread(buf, 1, bytes, (FILE*)uptr);
}

static long write_file(const void *buf, size_t bytes, void *uptr)
{
	return (long)fwrite(buf, 1, bytes, (FILE*)uptr);
}

static long seek_file(long offs, int whence, void *uptr)
{
	if(fseek((FILE*)uptr, offs, whence) == -1) {
		return -1;
	}
	return ftell((FILE*)uptr);
}

static char *clean_filename(char *str)
{
	char *last_slash, *ptr;

	if(!(last_slash = strrchr(str, '/'))) {
		last_slash = strrchr(str, '\\');
	}
	if(last_slash) {
		str = last_slash + 1;
	}

	ptr = str;
	while(*ptr) {
		char c = tolower(*ptr);
		*ptr++ = c;
	}
	*ptr = 0;
	return str;
}
