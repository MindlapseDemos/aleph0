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
#include <assert.h>
#include "treestor.h"
#include "goat3d.h"
#include "g3dscn.h"
#include "log.h"
#include "dynarr.h"
#include "track.h"
#include "util.h"

#if defined(__WATCOMC__) || defined(_WIN32) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif


struct key {
	long tm;
	cgm_vec4 val;
};

static struct goat3d_material *read_material(struct goat3d *g, struct ts_node *tsmtl);
static char *read_material_attrib(struct material_attrib *attr, struct ts_node *tsmattr);
static struct goat3d_mesh *read_mesh(struct goat3d *g, struct ts_node *tsmesh);
static void *read_veclist(void *arr, int dim, const char *nodename, const char *attrname, struct ts_node *tsnode);
static void *read_intlist(void *arr, int dim, const char *nodename, const char *attrname, struct ts_node *tsnode);
static void *read_bonelist(struct goat3d *g, struct goat3d_node **arr, struct ts_node *tsnode);
static int read_node(struct goat3d *g, struct goat3d_node *node, struct ts_node *tsnode);
static int read_anim(struct goat3d *g, struct ts_node *tsanim);
static struct goat3d_track *read_track(struct goat3d *g, struct ts_node *tstrk);


int g3dimpl_scnload(struct goat3d *g, struct goat3d_io *io)
{
	int idx;
	struct ts_io tsio;
	struct ts_node *tsroot, *c;
	const char *str;

	/* attempt to load it as gltf first */
	if((g3dimpl_loadgltf(g, io)) == 0) {
		return 0;
	}
	io->seek(0, SEEK_SET, io->cls);

	tsio.data = io->cls;
	tsio.read = io->read;
	tsio.write = io->write;

	if(!(tsroot = ts_load_io(&tsio))) {
		goat3d_logmsg(LOG_ERROR, "failed to load scene\n");
		return -1;
	}
	if(strcmp(tsroot->name, "scene") != 0) {
		goat3d_logmsg(LOG_ERROR, "invalid scene file, root node is not \"scene\"\n");
		ts_free_tree(tsroot);
		return -1;
	}

	/* read all materials */
	c = tsroot->child_list;
	while(c) {
		if(strcmp(c->name, "mtl") == 0) {
			struct goat3d_material *mtl = read_material(g, c);
			if(mtl) {
				goat3d_add_mtl(g, mtl);
			}
		}
		c = c->next;
	}

	/* create placeholder nodes, only populating the name field, so that bone
	 * references in meshes can be resolved. We'll read the rest of the node
	 * info, including their mesh/light/camera references at the end.
	 */
	c = tsroot->child_list;
	while(c) {
		if(strcmp(c->name, "node") == 0) {
			struct goat3d_node *node;

			if(!(node = goat3d_create_node())) {
				goat3d_logmsg(LOG_ERROR, "failed to allocate node\n");
				c = c->next;
				continue;
			}
			if((str = ts_get_attr_str(c, "name", 0))) {
				goat3d_set_node_name(node, str);
			}
			goat3d_add_node(g, node);
		}
		c = c->next;
	}

	/* read all meshes, cameras, lights, animations */
	c = tsroot->child_list;
	while(c) {
		if(strcmp(c->name, "mesh") == 0) {
			struct goat3d_mesh *mesh = read_mesh(g, c);
			if(mesh) {
				goat3d_add_mesh(g, mesh);
			}
		} else if(strcmp(c->name, "anim") == 0) {
			read_anim(g, c);
		}

		c = c->next;
	}

	/* now load the nodes properly */
	idx = 0;
	c = tsroot->child_list;
	while(c) {
		if(strcmp(c->name, "node") == 0) {
			struct goat3d_node *node = goat3d_get_node(g, idx++);
			assert(node);
			read_node(g, node, c);
		}
		c = c->next;
	}

	ts_free_tree(tsroot);
	return 0;
}

int g3dimpl_anmload(struct goat3d *g, struct goat3d_io *io)
{
	/*
	struct ts_io tsio;
	tsio.data = io->cls;
	tsio.read = io->read;
	tsio.write = io->write;
	*/
	return -1;
}

static struct goat3d_material *read_material(struct goat3d *g, struct ts_node *tsmtl)
{
	struct goat3d_material *mtl;
	struct material_attrib mattr, *arr;
	struct ts_node *c;
	const char *str;

	if(!(mtl = malloc(sizeof *mtl)) || g3dimpl_mtl_init(mtl) == -1) {
		free(mtl);
		goat3d_logmsg(LOG_ERROR, "read_material: failed to allocate material\n");
		return 0;
	}

	if(!(str = ts_get_attr_str(tsmtl, "name", 0)) || !*str) {
		/* XXX wait, we can refer to materials by index, why is the name important? */
		goat3d_logmsg(LOG_WARNING, "read_material: ignoring material without a name\n");
		g3dimpl_mtl_destroy(mtl);
		return 0;
	}
	goat3d_set_mtl_name(mtl, str);

	/* read all material attributes */
	c = tsmtl->child_list;
	while(c) {
		if(strcmp(c->name, "attr") == 0) {
			if(read_material_attrib(&mattr, c)) {
				if(!(arr = dynarr_push(mtl->attrib, &mattr))) {
					goat3d_logmsg(LOG_ERROR, "read_material: failed to resize material attribute array\n");
					g3dimpl_mtl_destroy(mtl);
					return 0;
				}
				mtl->attrib = arr;
			}
		}
		c = c->next;
	}

	/*if(dynarr_empty(mtl->attrib)) {
		goat3d_logmsg(LOG_WARNING, "read_material: ignoring empty material: %s\n", mtl->name);
		g3dimpl_mtl_destroy(mtl);
		return 0;
	}*/
	return mtl;
}

static char *read_material_attrib(struct material_attrib *attr, struct ts_node *tsnode)
{
	int i;
	struct ts_attr *tsattr;
	const char *name, *map;

	memset(attr, 0, sizeof *attr);

	if((tsattr = ts_get_attr(tsnode, "val"))) {
		attr->value.w = 1.0f;	/* default W to 1 if we get less than a float4 */

		switch(tsattr->val.type) {
		case TS_NUMBER:
			attr->value.x = tsattr->val.fnum;
			break;
		case TS_VECTOR:
			assert(tsattr->val.vec_size <= 4);
			for(i=0; i<tsattr->val.vec_size; i++) {
				(&attr->value.x)[i] = tsattr->val.vec[i];
			}
			break;
		default: /* no valid val attribute found */
			return 0;
		}
	}

	if(!(name = ts_get_attr_str(tsnode, "name", 0)) || !*name) {
		return 0;
	}
	if(!(attr->name = malloc(strlen(name) + 1))) {
		goat3d_logmsg(LOG_ERROR, "read_material_attrib: failed to allocate name\n");
		return 0;
	}
	strcpy(attr->name, name);

	if((map = ts_get_attr_str(tsnode, "map", 0)) && *map) {
		if(!(attr->map = malloc(strlen(map) + 1))) {
			goat3d_logmsg(LOG_ERROR, "read_material_attrib: failed to allocate map name\n");
			free(attr->name);
			return 0;
		}
		strcpy(attr->map, map);
	}
	return attr->name;
}

static struct goat3d_mesh *read_mesh(struct goat3d *g, struct ts_node *tsmesh)
{
	struct goat3d_mesh *mesh;
	struct goat3d_material *mtl;
	struct ts_node *c;
	const char *str;
	int num;
	void *tmp;

	if(!(mesh = malloc(sizeof *mesh)) || g3dimpl_obj_init((struct object*)mesh, OBJTYPE_MESH)) {
		goat3d_logmsg(LOG_ERROR, "read_mesh: failed to allocate mesh\n");
		goto fail;
	}

	if((str = ts_get_attr_str(tsmesh, "name", 0))) {
		goat3d_set_mesh_name(mesh, str);
	}

	/* material reference */
	if((num = ts_get_attr_num(tsmesh, "material", -1)) >= 0) {
		if((mtl = goat3d_get_mtl(g, num))) {
			goat3d_set_mesh_mtl(mesh, mtl);
		} else {
			goat3d_logmsg(LOG_WARNING, "read_mesh: mesh %s refers to invalid material: %d\n",
					mesh->name, num);
		}
	} else if((str = ts_get_attr_str(tsmesh, "material", 0))) {
		if((mtl = goat3d_get_mtl_by_name(g, str))) {
			goat3d_set_mesh_mtl(mesh, mtl);
		} else {
			goat3d_logmsg(LOG_WARNING, "read_mesh: mesh %s refers to invalid material: %s\n",
					mesh->name, str);
		}
	}

	/* external mesh data */
	if((str = ts_get_attr_str(tsmesh, "file", 0))) {
		const char *fname = str;
		char *pathbuf;

		if(g->search_path) {
			pathbuf = alloca(strlen(str) + strlen(g->search_path) + 2);
			sprintf(pathbuf, "%s/%s", g->search_path, str);
			fname = pathbuf;
		}

		if(g3dimpl_loadmesh(mesh, fname) == -1) {
			goat3d_logmsg(LOG_ERROR, "read_mesh: failed to load external mesh: %s\n", fname);
			goto fail;
		}

		/* done loading, we can't have both an external mesh AND internal data,
		 * if there's anything else hanging under this tsnode, ignore it.
		 */
		if(tsmesh->child_list) {
			goat3d_logmsg(LOG_WARNING, "read_mesh: external mesh node also has unexpected children; ignoring.\n");
		}
		return mesh;
	}

	c = tsmesh->child_list;
	while(c) {
		if(strcmp(c->name, "vertex_list") == 0) {
			if(!(tmp = read_veclist(mesh->vertices, 3, "vertex", "pos", c))) {
				goat3d_logmsg(LOG_ERROR, "read_mesh: failed to read vertex array for mesh %s\n",
						mesh->name);
				goto fail;
			}
			mesh->vertices = tmp;

		} else if(strcmp(c->name, "normal_list") == 0) {
			if(!(tmp = read_veclist(mesh->normals, 3, "normal", "dir", c))) {
				goat3d_logmsg(LOG_WARNING, "read_mesh: failed to read normals array for mesh %s\n",
						mesh->name);
			} else {
				mesh->normals = tmp;
			}

		} else if(strcmp(c->name, "tangent_list") == 0) {
			if(!(tmp = read_veclist(mesh->tangents, 3, "tangent", "dir", c))) {
				goat3d_logmsg(LOG_WARNING, "read_mesh: failed to read tangents array for mesh %s\n",
						mesh->name);
			} else {
				mesh->tangents = tmp;
			}

		} else if(strcmp(c->name, "texcoord_list") == 0) {
			if(!(tmp = read_veclist(mesh->texcoords, 2, "texcoord", "uv", c))) {
				goat3d_logmsg(LOG_WARNING, "read_mesh: failed to read texcoord array for mesh %s\n",
						mesh->name);
			} else {
				mesh->texcoords = tmp;
			}

		} else if(strcmp(c->name, "skinweight_list") == 0) {
			if(!(tmp = read_veclist(mesh->skin_weights, 4, "skinweight", "weights", c))) {
				goat3d_logmsg(LOG_WARNING, "read_mesh: failed to read skin weights array for mesh %s\n",
						mesh->name);
			} else {
				mesh->skin_weights = tmp;
			}

		} else if(strcmp(c->name, "skinmatrix_list") == 0) {
			if(!(tmp = read_intlist(mesh->skin_matrices, 4, "skinmatrix", "idx", c))) {
				goat3d_logmsg(LOG_WARNING, "read_mesh: failed to read skin matrix index array for mesh %s\n",
						mesh->name);
			} else {
				mesh->skin_matrices = tmp;
			}

		} else if(strcmp(c->name, "color_list") == 0) {
			if(!(tmp = read_veclist(mesh->colors, 4, "color", "color", c))) {
				goat3d_logmsg(LOG_WARNING, "read_mesh: failed to read color array for mesh %s\n",
						mesh->name);
			} else {
				mesh->colors = tmp;
			}

		} else if(strcmp(c->name, "bone_list") == 0) {
			if(!(tmp = read_bonelist(g, mesh->bones, c))) {
				goat3d_logmsg(LOG_WARNING, "read_mesh: failed to read bones array for mesh %s\n",
						mesh->name);
			} else {
				mesh->bones = tmp;
			}

		} else if(strcmp(c->name, "face_list") == 0) {
			if(!(tmp = read_intlist(mesh->faces, 3, "face", "idx", c))) {
				goat3d_logmsg(LOG_ERROR, "read_mesh: failed to read faces array for mesh %s\n",
						mesh->name);
				goto fail;
			}
			mesh->faces = tmp;
		}
		c = c->next;
	}
	return mesh;

fail:
	g3dimpl_obj_destroy((struct object*)mesh);
	return 0;
}

static void *read_veclist(void *arr, int dim, const char *nodename, const char *attrname, struct ts_node *tslist)
{
	int i, size, bufsz;
	struct ts_node *c;
	struct ts_attr *attr;
	float vec[4];
	const char *str;

	arr = dynarr_clear(arr);
	assert(arr);

	if((size = ts_get_attr_int(tslist, "list_size", -1)) <= 0) {
		goat3d_logmsg(LOG_WARNING, "read_veclist: list_size attribute missing or invalid\n");
		size = -1;
	}

	if((str = ts_get_attr_str(tslist, "base64", 0))) {
		if(size == -1) size = calc_b64_size(str);
		if(!(arr = dynarr_resize(arr, size))) {
			goat3d_logmsg(LOG_ERROR, "read_veclist: failed to resize %s array\n",
					nodename);
			return 0;
		}

		bufsz = size * dim * sizeof(float);
		b64decode(str, arr, &bufsz);
#ifdef GOAT3D_BIGEND
		goat3d_bswap32(arr, size * dim);
#endif
	}

	c = tslist->child_list;
	while(c) {
		if(strcmp(c->name, nodename) != 0) {
			c = c->next;
			continue;
		}

		if((attr = ts_get_attr(c, attrname)) && attr->val.type == TS_VECTOR) {
			for(i=0; i<dim; i++) {
				if(i >= attr->val.vec_size) {
					vec[i] = 0;
				} else {
					vec[i] = attr->val.vec[i];
				}
			}

			if(!(arr = dynarr_push(arr, vec))) {
				goat3d_logmsg(LOG_ERROR, "read_veclist: failed to resize %s array\n",
						nodename);
				return 0;
			}
		}
		c = c->next;
	}

	if(size > 0 && dynarr_size(arr) != size) {
		goat3d_logmsg(LOG_WARNING, "read_veclist: expected %d items, read %d\n", size, dynarr_size(arr));
	}
	return arr;
}

static void *read_intlist(void *arr, int dim, const char *nodename, const char *attrname, struct ts_node *tslist)
{
	int i, size, bufsz;
	struct ts_node *c;
	struct ts_attr *attr;
	int ivec[4];
	const char *str;

	arr = dynarr_clear(arr);
	assert(arr);

	if((size = ts_get_attr_int(tslist, "list_size", -1)) <= 0) {
		goat3d_logmsg(LOG_WARNING, "read_intlist: list_size attribute missing or invalid\n");
		size = -1;
	}

	if((str = ts_get_attr_str(tslist, "base64", 0))) {
		if(size == -1) size = calc_b64_size(str);
		if(!(arr = dynarr_resize(arr, size))) {
			goat3d_logmsg(LOG_ERROR, "read_intlist: failed to resize %s array\n",
					nodename);
			return 0;
		}

		bufsz = size * dim * sizeof(int);
		b64decode(str, arr, &bufsz);
#ifdef GOAT3D_BIGEND
		goat3d_bswap32(arr, size * dim);
#endif
	}

	c = tslist->child_list;
	while(c) {
		if(strcmp(c->name, nodename) != 0) {
			c = c->next;
			continue;
		}

		if((attr = ts_get_attr(c, attrname)) && attr->val.type == TS_VECTOR) {
			for(i=0; i<dim; i++) {
				if(i >= attr->val.vec_size) {
					ivec[i] = 0;
				} else {
					ivec[i] = attr->val.vec[i];
				}
			}

			if(!(arr = dynarr_push(arr, ivec))) {
				goat3d_logmsg(LOG_ERROR, "read_intlist: failed to resize %s array\n",
						nodename);
				return 0;
			}
		}
		c = c->next;
	}

	if(size > 0 && dynarr_size(arr) != size) {
		goat3d_logmsg(LOG_WARNING, "read_intlist: expected %d items, read %d\n", size, dynarr_size(arr));
	}
	return arr;
}

static void *read_bonelist(struct goat3d *g, struct goat3d_node **arr, struct ts_node *tslist)
{
	int size, idx;
	struct ts_node *c;
	struct goat3d_node *bone;
	const char *str;

	arr = dynarr_clear(arr);
	assert(arr);

	if((size = ts_get_attr_int(tslist, "list_size", -1)) <= 0) {
		goat3d_logmsg(LOG_WARNING, "read_bonelist: list_size attribute missing or invalid\n");
		size = -1;
	}

	/* TODO base64 data support */

	c = tslist->child_list;
	while(c) {
		if(strcmp(c->name, "bone") != 0) {
			c = c->next;
			continue;
		}

		bone = 0;

		if((idx = ts_get_attr_int(c, "bone", -1)) >= 0) {
			if(!(bone = goat3d_get_node(g, idx))) {
				goat3d_logmsg(LOG_ERROR, "read_bonelist: reference to invalid bone: %d\n", idx);
				return 0;
			}

		} else if((str = ts_get_attr_str(c, "bone", 0))) {
			if(!(bone = goat3d_get_node_by_name(g, str))) {
				goat3d_logmsg(LOG_ERROR, "read_bonelist: reference to invalid bone: %s\n", str);
				return 0;
			}
		}

		if(bone && !(arr = dynarr_push(arr, &bone))) {
			goat3d_logmsg(LOG_ERROR, "read_bonelist: failed to resize bone array\n");
			return 0;
		}
		c = c->next;
	}

	if(size > 0 && dynarr_size(arr) != size) {
		goat3d_logmsg(LOG_WARNING, "read_bonelist: expected %d items, read %d\n", size, dynarr_size(arr));
	}
	return arr;
}

#define GETREF(ptr, typestr, getname) \
	do { \
		ptr = 0; \
		if((idx = ts_get_attr_int(tsnode, typestr, -1)) >= 0) { \
			if(!(ptr = goat3d_get_##getname(g, idx))) { \
				goat3d_logmsg(LOG_ERROR, "read_node: ignoring reference to invalid " typestr ": %d\n", idx); \
			} \
		} else if((str = ts_get_attr_str(tsnode, typestr, 0))) { \
			if(!(ptr = goat3d_get_##getname##_by_name(g, str))) { \
				goat3d_logmsg(LOG_ERROR, "read_node: ignoring reference to invalid " typestr ": %s\n", str); \
			} \
		} \
	} while(0)

static int read_node(struct goat3d *g, struct goat3d_node *node, struct ts_node *tsnode)
{
	int idx;
	const char *str;
	struct goat3d_node *parent;
	float *vec;

	GETREF(parent, "parent", node);
	if(parent) {
		goat3d_add_node_child(parent, node);
	}

	node->type = GOAT3D_NODE_MESH;
	GETREF(node->obj, "mesh", mesh);
	if(!node->obj) {
		node->type = GOAT3D_NODE_LIGHT;
		GETREF(node->obj, "light", light);
	}
	if(!node->obj) {
		node->type = GOAT3D_NODE_CAMERA;
		GETREF(node->obj, "camera", camera);
	}
	if(!node->obj) {
		node->type = GOAT3D_NODE_NULL;
	}

	if((vec = ts_get_attr_vec(tsnode, "pos", 0))) {
		goat3d_set_node_position(node, vec[0], vec[1], vec[2]);
	}
	if((vec = ts_get_attr_vec(tsnode, "rot", 0))) {
		goat3d_set_node_rotation(node, vec[0], vec[1], vec[2], vec[3]);
	}
	if((vec = ts_get_attr_vec(tsnode, "scale", 0))) {
		goat3d_set_node_scaling(node, vec[0], vec[1], vec[2]);
	}
	if((vec = ts_get_attr_vec(tsnode, "pivot", 0))) {
		goat3d_set_node_pivot(node, vec[0], vec[1], vec[2]);
	}

	return 0;
}

static int read_anim(struct goat3d *g, struct ts_node *tsanim)
{
	struct ts_node *c;
	const char *str;
	struct goat3d_anim *anim;
	struct goat3d_track *trk;

	if(!(str = ts_get_attr_str(tsanim, "name", 0))) {
		goat3d_logmsg(LOG_WARNING, "read_anim: ignoring animation without a name\n");
		return -1;
	}

	if(!(anim = goat3d_create_anim())) {
		goat3d_logmsg(LOG_ERROR, "read_anim: failed to initialize animation: %s\n", str);
		return -1;
	}
	goat3d_set_anim_name(anim, str);

	c = tsanim->child_list;
	while(c) {
		if(strcmp(c->name, "track") == 0) {
			if(!(trk = read_track(g, c))) {
				c = c->next;
				continue;
			}
			goat3d_add_anim_track(anim, trk);
		}
		c = c->next;
	}

	goat3d_add_anim(g, anim);
	return 0;
}

static int parsetype(const char *str)
{
	if(strcmp(str, "pos") == 0) return GOAT3D_TRACK_POS;
	if(strcmp(str, "rot") == 0) return GOAT3D_TRACK_ROT;
	if(strcmp(str, "scale") == 0) return GOAT3D_TRACK_SCALE;
	if(strcmp(str, "val") == 0) return GOAT3D_TRACK_VAL;
	if(strcmp(str, "vec3") == 0) return GOAT3D_TRACK_VEC3;
	if(strcmp(str, "vec4") == 0) return GOAT3D_TRACK_VEC4;
	if(strcmp(str, "quat") == 0) return GOAT3D_TRACK_QUAT;
	return -1;
}

static int parseinterp(const char *str)
{
	if(strcmp(str, "step") == 0) return GOAT3D_INTERP_STEP;
	if(strcmp(str, "linear") == 0) return GOAT3D_INTERP_LINEAR;
	if(strcmp(str, "cubic") == 0) return GOAT3D_INTERP_CUBIC;
	return -1;
}

static int parseextrap(const char *str)
{
	if(strcmp(str, "extend") == 0) return GOAT3D_EXTRAP_EXTEND;
	if(strcmp(str, "clamp") == 0) return GOAT3D_EXTRAP_CLAMP;
	if(strcmp(str, "repeat") == 0) return GOAT3D_EXTRAP_REPEAT;
	if(strcmp(str, "pingpong") == 0) return GOAT3D_EXTRAP_PINGPONG;
	return -1;
}

static struct goat3d_track *read_track(struct goat3d *g, struct ts_node *tstrk)
{
	int i, idx;
	const char *str;
	struct goat3d_track *trk;
	struct goat3d_node *node;
	struct goat3d_key key;
	int type, in, ex;
	struct ts_node *c;
	struct ts_attr *tsattr;

	if(!(str = ts_get_attr_str(tstrk, "type", 0)) || (type = parsetype(str)) == -1) {
		goat3d_logmsg(LOG_WARNING, "read_track: ignoring track with missing or invalid type attribute\n");
		return 0;
	}

	if((idx = ts_get_attr_int(tstrk, "node", -1)) >= 0) {
		if(!(node = goat3d_get_node(g, idx))) {
			goat3d_logmsg(LOG_WARNING, "read_track: ignoring track with invalid node reference (%d)\n", idx);
			return 0;
		}
	} else if((str = ts_get_attr_str(tstrk, "node", 0))) {
		if(!(node = goat3d_get_node_by_name(g, str))) {
			goat3d_logmsg(LOG_WARNING, "read_track: ignoring track with invalid node reference (%s)\n", str);
			return 0;
		}
	} else {
		goat3d_logmsg(LOG_WARNING, "read_track: ignoring track with missing node reference\n");
		return 0;
	}

	if(!(trk = goat3d_create_track())) {
		goat3d_logmsg(LOG_ERROR, "read_track: failed to create new keyframe track\n");
		return 0;
	}
	goat3d_set_track_node(trk, node);
	goat3d_set_track_type(trk, type);

	if((str = ts_get_attr_str(tstrk, "name", 0))) {
		goat3d_set_track_name(trk, str);
	}

	if((str = ts_get_attr_str(tstrk, "interp", 0))) {
		if((in = parseinterp(str)) == -1) {
			goat3d_logmsg(LOG_WARNING, "read_track: ignoring invalid interpolation mode: %s\n", str);
		} else {
			goat3d_set_track_interp(trk, in);
		}
	}
	if((str = ts_get_attr_str(tstrk, "extrap", 0))) {
		if((ex = parseextrap(str)) == -1) {
			goat3d_logmsg(LOG_WARNING, "read_track: ignoring invalid extrapolation mode: %s\n", str);
		} else {
			goat3d_set_track_extrap(trk, ex);
		}
	}

	c = tstrk->child_list;
	while(c) {
		if(strcmp(c->name, "key") == 0) {
			if((key.tm = ts_get_attr_int(c, "time", INT_MIN)) == INT_MIN) {
				goat3d_logmsg(LOG_WARNING, "read_track: ignoring keyframe with missing or invalid time (%s)\n",
						ts_get_attr_str(c, "time", ""));
				c = c->next;
				continue;
			}
			if(!(tsattr = ts_get_attr(c, "value")) || (tsattr->val.type != TS_NUMBER &&
						tsattr->val.type != TS_VECTOR)) {
				goat3d_logmsg(LOG_WARNING, "read_track: ignoring keyframe with missing or invalid value (%s)\n",
						ts_get_attr_str(c, "value", ""));
				c = c->next;
				continue;
			}

			if(tsattr->val.type == TS_NUMBER) {
				key.val[0] = tsattr->val.fnum;
			} else {
				for(i=0; i<4; i++) {
					if(i < tsattr->val.vec_size) {
						key.val[i] = tsattr->val.vec[i];
					} else {
						key.val[i] = 0;
					}
				}
			}
			goat3d_set_track_key(trk, &key);
		}
		c = c->next;
	}

	/* force lazy re-sorting of keyframes if necessary */
	goat3d_get_track_key(trk, 0, &key);

	return trk;
}
