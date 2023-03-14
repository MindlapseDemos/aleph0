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
#ifndef GOAT3D_H_
#define GOAT3D_H_

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define GOAT3DAPI	__declspec(dllexport)
#else
#ifdef __GNUC__
#define GOAT3DAPI	__attribute__((visibility("default")))
#else
#define GOAT3DAPI
#endif
#endif

#define GOAT3D_MAT_ATTR_DIFFUSE			"diffuse"
#define GOAT3D_MAT_ATTR_SPECULAR		"specular"
#define GOAT3D_MAT_ATTR_SHININESS		"shininess"
#define GOAT3D_MAT_ATTR_NORMAL			"normal"
#define GOAT3D_MAT_ATTR_BUMP			"bump"
#define GOAT3D_MAT_ATTR_REFLECTION		"reflection"
#define GOAT3D_MAT_ATTR_TRANSMISSION	"transmission"
#define GOAT3D_MAT_ATTR_IOR				"ior"
#define GOAT3D_MAT_ATTR_ALPHA			"alpha"

enum goat3d_mesh_attrib {
	GOAT3D_MESH_ATTR_VERTEX,
	GOAT3D_MESH_ATTR_NORMAL,
	GOAT3D_MESH_ATTR_TANGENT,
	GOAT3D_MESH_ATTR_TEXCOORD,
	GOAT3D_MESH_ATTR_SKIN_WEIGHT,
	GOAT3D_MESH_ATTR_SKIN_MATRIX,
	GOAT3D_MESH_ATTR_COLOR,

	NUM_GOAT3D_MESH_ATTRIBS
};

enum goat3d_node_type {
	GOAT3D_NODE_NULL,
	GOAT3D_NODE_MESH,
	GOAT3D_NODE_LIGHT,
	GOAT3D_NODE_CAMERA
};

/* immediate mode mesh construction primitive type */
enum goat3d_im_primitive {
	GOAT3D_TRIANGLES,
	GOAT3D_QUADS
};

enum goat3d_track_type {
	GOAT3D_TRACK_VAL,
	GOAT3D_TRACK_VEC3,
	GOAT3D_TRACK_VEC4,
	GOAT3D_TRACK_QUAT,
	GOAT3D_TRACK_POS	= GOAT3D_TRACK_VEC3 | 0x100,
	GOAT3D_TRACK_ROT	= GOAT3D_TRACK_QUAT | 0x200,
	GOAT3D_TRACK_SCALE	= GOAT3D_TRACK_VEC3 | 0x300
};

struct goat3d_key {
	long tm;
	float val[4];
};

/* track interpolation modes */
enum goat3d_interp {
	GOAT3D_INTERP_STEP,
	GOAT3D_INTERP_LINEAR,
	GOAT3D_INTERP_CUBIC
};
/* track extrapolation modes */
enum goat3d_extrap {
	GOAT3D_EXTRAP_EXTEND,
	GOAT3D_EXTRAP_CLAMP,
	GOAT3D_EXTRAP_REPEAT,
	GOAT3D_EXTRAP_PINGPONG
};

enum goat3d_option {
	GOAT3D_OPT_SAVEXML,		/* save in XML format (dropped) */
	GOAT3D_OPT_SAVETEXT,	/* save in text format */
	GOAT3D_OPT_SAVEBINDATA,	/* save mesh data in text files as binary blobs */
	GOAT3D_OPT_SAVEBIN,		/* not implemented yet */
	GOAT3D_OPT_SAVEGLTF,	/* not implemented yet */
	GOAT3D_OPT_SAVEGLB,		/* not implemented yet */

	NUM_GOAT3D_OPTIONS
};

struct goat3d;
struct goat3d_material;
struct goat3d_mtlattr;
struct goat3d_mesh;
struct goat3d_light;
struct goat3d_camera;
struct goat3d_node;
struct goat3d_anim;
struct goat3d_track;

struct goat3d_io {
	void *cls;	/* closure data */

	long (*read)(void *buf, size_t bytes, void *uptr);
	long (*write)(const void *buf, size_t bytes, void *uptr);
	long (*seek)(long offs, int whence, void *uptr);
};

#ifdef __cplusplus
extern "C" {
#endif

/* construction/destruction */
GOAT3DAPI struct goat3d *goat3d_create(void);
GOAT3DAPI void goat3d_free(struct goat3d *g);

GOAT3DAPI void goat3d_setopt(struct goat3d *g, enum goat3d_option opt, int val);
GOAT3DAPI int goat3d_getopt(const struct goat3d *g, enum goat3d_option opt);

/* load/save */
GOAT3DAPI int goat3d_load(struct goat3d *g, const char *fname);
GOAT3DAPI int goat3d_save(const struct goat3d *g, const char *fname);

GOAT3DAPI int goat3d_load_file(struct goat3d *g, FILE *fp);
GOAT3DAPI int goat3d_save_file(const struct goat3d *g, FILE *fp);

GOAT3DAPI int goat3d_load_io(struct goat3d *g, struct goat3d_io *io);
GOAT3DAPI int goat3d_save_io(const struct goat3d *g, struct goat3d_io *io);

/* load/save animation files (g must already be loaded to load animations) */
GOAT3DAPI int goat3d_load_anim(struct goat3d *g, const char *fname);
GOAT3DAPI int goat3d_save_anim(const struct goat3d *g, const char *fname);

GOAT3DAPI int goat3d_load_anim_file(struct goat3d *g, FILE *fp);
GOAT3DAPI int goat3d_save_anim_file(const struct goat3d *g, FILE *fp);

GOAT3DAPI int goat3d_load_anim_io(struct goat3d *g, struct goat3d_io *io);
GOAT3DAPI int goat3d_save_anim_io(const struct goat3d *g, struct goat3d_io *io);

/* misc scene properties */
GOAT3DAPI int goat3d_set_name(struct goat3d *g, const char *name);
GOAT3DAPI const char *goat3d_get_name(const struct goat3d *g);

GOAT3DAPI void goat3d_set_ambient(struct goat3d *g, const float *ambient);
GOAT3DAPI void goat3d_set_ambient3f(struct goat3d *g, float ar, float ag, float ab);
GOAT3DAPI const float *goat3d_get_ambient(const struct goat3d *g);

GOAT3DAPI int goat3d_get_bounds(const struct goat3d *g, float *bmin, float *bmax);

/* materials */
GOAT3DAPI int goat3d_add_mtl(struct goat3d *g, struct goat3d_material *mtl);
GOAT3DAPI int goat3d_get_mtl_count(struct goat3d *g);
GOAT3DAPI struct goat3d_material *goat3d_get_mtl(struct goat3d *g, int idx);
GOAT3DAPI struct goat3d_material *goat3d_get_mtl_by_name(struct goat3d *g, const char *name);

GOAT3DAPI struct goat3d_material *goat3d_create_mtl(void);
GOAT3DAPI void goat3d_destroy_mtl(struct goat3d_material *mtl);

GOAT3DAPI int goat3d_set_mtl_name(struct goat3d_material *mtl, const char *name);
GOAT3DAPI const char *goat3d_get_mtl_name(const struct goat3d_material *mtl);

GOAT3DAPI int goat3d_set_mtl_attrib(struct goat3d_material *mtl, const char *attrib, const float *val);
GOAT3DAPI int goat3d_set_mtl_attrib1f(struct goat3d_material *mtl, const char *attrib, float val);
GOAT3DAPI int goat3d_set_mtl_attrib3f(struct goat3d_material *mtl, const char *attrib, float r, float g, float b);
GOAT3DAPI int goat3d_set_mtl_attrib4f(struct goat3d_material *mtl, const char *attrib, float r, float g, float b, float a);
GOAT3DAPI const float *goat3d_get_mtl_attrib(struct goat3d_material *mtl, const char *attrib);

GOAT3DAPI int goat3d_set_mtl_attrib_map(struct goat3d_material *mtl, const char *attrib, const char *mapname);
GOAT3DAPI const char *goat3d_get_mtl_attrib_map(struct goat3d_material *mtl, const char *attrib);

GOAT3DAPI int goat3d_get_mtl_attrib_count(struct goat3d_material *mtl);
GOAT3DAPI const char *goat3d_get_mtl_attrib_name(struct goat3d_material *mtl, int idx);


/* meshes */
GOAT3DAPI int goat3d_add_mesh(struct goat3d *g, struct goat3d_mesh *mesh);
GOAT3DAPI int goat3d_get_mesh_count(struct goat3d *g);
GOAT3DAPI struct goat3d_mesh *goat3d_get_mesh(struct goat3d *g, int idx);
GOAT3DAPI struct goat3d_mesh *goat3d_get_mesh_by_name(struct goat3d *g, const char *name);

GOAT3DAPI struct goat3d_mesh *goat3d_create_mesh(void);
GOAT3DAPI void goat3d_destroy_mesh(struct goat3d_mesh *mesh);

GOAT3DAPI int goat3d_set_mesh_name(struct goat3d_mesh *mesh, const char *name);
GOAT3DAPI const char *goat3d_get_mesh_name(const struct goat3d_mesh *mesh);

GOAT3DAPI void goat3d_set_mesh_mtl(struct goat3d_mesh *mesh, struct goat3d_material *mtl);
GOAT3DAPI struct goat3d_material *goat3d_get_mesh_mtl(struct goat3d_mesh *mesh);

GOAT3DAPI int goat3d_get_mesh_vertex_count(struct goat3d_mesh *mesh);
GOAT3DAPI int goat3d_get_mesh_attrib_count(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib);
GOAT3DAPI int goat3d_get_mesh_face_count(struct goat3d_mesh *mesh);

/* sets all the data for a single vertex attribute array in one go.
 * vnum is the number of *vertices* to be set, not the number of floats, ints or whatever
 * data is expected to be something different depending on the attribute:
 *  - GOAT3D_MESH_ATTR_VERTEX       - 3 floats per vertex
 *  - GOAT3D_MESH_ATTR_NORMAL       - 3 floats per vertex
 *  - GOAT3D_MESH_ATTR_TANGENT      - 3 floats per vertex
 *  - GOAT3D_MESH_ATTR_TEXCOORD     - 2 floats per vertex
 *  - GOAT3D_MESH_ATTR_SKIN_WEIGHT  - 4 floats per vertex
 *  - GOAT3D_MESH_ATTR_SKIN_MATRIX  - 4 ints per vertex
 *  - GOAT3D_MESH_ATTR_COLOR        - 4 floats per vertex
 */
GOAT3DAPI int goat3d_set_mesh_attribs(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		const void *data, int vnum);
GOAT3DAPI int goat3d_add_mesh_attrib1f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib, float val);
GOAT3DAPI int goat3d_add_mesh_attrib2f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		float x, float y);
GOAT3DAPI int goat3d_add_mesh_attrib3f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		float x, float y, float z);
GOAT3DAPI int goat3d_add_mesh_attrib4f(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib,
		float x, float y, float z, float w);
/* returns a pointer to the beginning of the requested mesh attribute array */
GOAT3DAPI void *goat3d_get_mesh_attribs(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib);
/* returns a pointer to the requested mesh attribute */
GOAT3DAPI void *goat3d_get_mesh_attrib(struct goat3d_mesh *mesh, enum goat3d_mesh_attrib attrib, int idx);

/* sets all the faces in one go. data is an array of 3 int vertex indices per face */
GOAT3DAPI int goat3d_set_mesh_faces(struct goat3d_mesh *mesh, const int *data, int fnum);
GOAT3DAPI int goat3d_add_mesh_face(struct goat3d_mesh *mesh, int a, int b, int c);
/* returns a pointer to the beginning of the face index array */
GOAT3DAPI int *goat3d_get_mesh_faces(struct goat3d_mesh *mesh);
/* returns a pointer to a face index */
GOAT3DAPI int *goat3d_get_mesh_face(struct goat3d_mesh *mesh, int idx);

/* immediate mode OpenGL-like interface for setting mesh data
 *  NOTE: using this interface will result in no vertex sharing between faces
 * NOTE2: the immedate mode interface is not thread-safe, either use locks, or don't
 *        use it at all in multithreaded situations.
 */
GOAT3DAPI void goat3d_begin(struct goat3d_mesh *mesh, enum goat3d_im_primitive prim);
GOAT3DAPI void goat3d_end(void);
GOAT3DAPI void goat3d_vertex3f(float x, float y, float z);
GOAT3DAPI void goat3d_normal3f(float x, float y, float z);
GOAT3DAPI void goat3d_tangent3f(float x, float y, float z);
GOAT3DAPI void goat3d_texcoord2f(float x, float y);
GOAT3DAPI void goat3d_skin_weight4f(float x, float y, float z, float w);
GOAT3DAPI void goat3d_skin_matrix4i(int x, int y, int z, int w);
GOAT3DAPI void goat3d_color3f(float x, float y, float z);
GOAT3DAPI void goat3d_color4f(float x, float y, float z, float w);

GOAT3DAPI void goat3d_get_mesh_bounds(const struct goat3d_mesh *mesh, float *bmin, float *bmax);

/* lights (TODO) */
GOAT3DAPI int goat3d_add_light(struct goat3d *g, struct goat3d_light *lt);
GOAT3DAPI int goat3d_get_light_count(struct goat3d *g);
GOAT3DAPI struct goat3d_light *goat3d_get_light(struct goat3d *g, int idx);
GOAT3DAPI struct goat3d_light *goat3d_get_light_by_name(struct goat3d *g, const char *name);

GOAT3DAPI struct goat3d_light *goat3d_create_light(void);
GOAT3DAPI void goat3d_destroy_light(struct goat3d_light *lt);

GOAT3DAPI int goat3d_set_light_name(struct goat3d_light *lt, const char *name);
GOAT3DAPI const char *goat3d_get_light_name(const struct goat3d_light *lt);

/* cameras (TODO) */
GOAT3DAPI int goat3d_add_camera(struct goat3d *g, struct goat3d_camera *cam);
GOAT3DAPI int goat3d_get_camera_count(struct goat3d *g);
GOAT3DAPI struct goat3d_camera *goat3d_get_camera(struct goat3d *g, int idx);
GOAT3DAPI struct goat3d_camera *goat3d_get_camera_by_name(struct goat3d *g, const char *name);

GOAT3DAPI struct goat3d_camera *goat3d_create_camera(void);
GOAT3DAPI void goat3d_destroy_camera(struct goat3d_camera *cam);

GOAT3DAPI int goat3d_set_camera_name(struct goat3d_camera *cam, const char *name);
GOAT3DAPI const char *goat3d_get_camera_name(const struct goat3d_camera *cam);

/* nodes */
GOAT3DAPI int goat3d_add_node(struct goat3d *g, struct goat3d_node *node);
GOAT3DAPI int goat3d_get_node_count(struct goat3d *g);
GOAT3DAPI struct goat3d_node *goat3d_get_node(struct goat3d *g, int idx);
GOAT3DAPI struct goat3d_node *goat3d_get_node_by_name(struct goat3d *g, const char *name);

GOAT3DAPI struct goat3d_node *goat3d_create_node(void);
GOAT3DAPI void goat3d_destroy_node(struct goat3d_node *node);

GOAT3DAPI int goat3d_set_node_name(struct goat3d_node *node, const char *name);
GOAT3DAPI const char *goat3d_get_node_name(const struct goat3d_node *node);

GOAT3DAPI void goat3d_set_node_object(struct goat3d_node *node, enum goat3d_node_type type, void *obj);
GOAT3DAPI void *goat3d_get_node_object(const struct goat3d_node *node);
GOAT3DAPI enum goat3d_node_type goat3d_get_node_type(const struct goat3d_node *node);

GOAT3DAPI void goat3d_add_node_child(struct goat3d_node *node, struct goat3d_node *child);
GOAT3DAPI int goat3d_get_node_child_count(const struct goat3d_node *node);
GOAT3DAPI struct goat3d_node *goat3d_get_node_child(const struct goat3d_node *node, int idx);
GOAT3DAPI struct goat3d_node *goat3d_get_node_parent(const struct goat3d_node *node);

GOAT3DAPI void goat3d_set_node_position(struct goat3d_node *node, float x, float y, float z);
GOAT3DAPI void goat3d_set_node_rotation(struct goat3d_node *node, float qx, float qy, float qz, float qw);
GOAT3DAPI void goat3d_set_node_scaling(struct goat3d_node *node, float sx, float sy, float sz);

GOAT3DAPI void goat3d_get_node_position(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr);
GOAT3DAPI void goat3d_get_node_rotation(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr, float *wptr);
GOAT3DAPI void goat3d_get_node_scaling(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr);

GOAT3DAPI void goat3d_set_node_pivot(struct goat3d_node *node, float x, float y, float z);
GOAT3DAPI void goat3d_get_node_pivot(const struct goat3d_node *node, float *xptr, float *yptr, float *zptr);

GOAT3DAPI void goat3d_get_node_matrix(const struct goat3d_node *node, float *matrix);
/* same as above, but also takes hierarchy into account */
GOAT3DAPI void goat3d_get_matrix(const struct goat3d_node *node, float *matrix);

GOAT3DAPI void goat3d_get_node_bounds(const struct goat3d_node *node, float *bmin, float *bmax);

/* keyframe track */
GOAT3DAPI struct goat3d_track *goat3d_create_track(void);
GOAT3DAPI void goat3d_destroy_track(struct goat3d_track *trk);

GOAT3DAPI int goat3d_set_track_name(struct goat3d_track *trk, const char *name);
GOAT3DAPI const char *goat3d_get_track_name(const struct goat3d_track *trk);

GOAT3DAPI void goat3d_set_track_type(struct goat3d_track *trk, enum goat3d_track_type type);
GOAT3DAPI enum goat3d_track_type goat3d_get_track_type(const struct goat3d_track *trk);

GOAT3DAPI void goat3d_set_track_node(struct goat3d_track *trk, struct goat3d_node *node);
GOAT3DAPI struct goat3d_node *goat3d_get_track_node(const struct goat3d_track *trk);

GOAT3DAPI void goat3d_set_track_interp(struct goat3d_track *trk, enum goat3d_interp in);
GOAT3DAPI enum goat3d_interp goat3d_get_track_interp(const struct goat3d_track *trk);
GOAT3DAPI void goat3d_set_track_extrap(struct goat3d_track *trk, enum goat3d_extrap ex);
GOAT3DAPI enum goat3d_extrap goat3d_get_track_extrap(const struct goat3d_track *trk);

GOAT3DAPI int goat3d_set_track_key(struct goat3d_track *trk, const struct goat3d_key *key);
GOAT3DAPI int goat3d_get_track_key(const struct goat3d_track *trk, int keyidx, struct goat3d_key *key);
GOAT3DAPI int goat3d_get_track_key_count(const struct goat3d_track *trk);

GOAT3DAPI int goat3d_set_track_val(struct goat3d_track *trk, long msec, float val);
GOAT3DAPI int goat3d_set_track_vec3(struct goat3d_track *trk, long msec, float x, float y, float z);
GOAT3DAPI int goat3d_set_track_vec4(struct goat3d_track *trk, long msec, float x, float y, float z, float w);
GOAT3DAPI int goat3d_set_track_quat(struct goat3d_track *trk, long msec, float x, float y, float z, float w);

GOAT3DAPI void goat3d_get_track_val(const struct goat3d_track *trk, long msec, float *valp);
GOAT3DAPI void goat3d_get_track_vec3(const struct goat3d_track *trk, long msec, float *xp, float *yp, float *zp);
GOAT3DAPI void goat3d_get_track_vec4(const struct goat3d_track *trk, long msec, float *xp, float *yp, float *zp, float *wp);
GOAT3DAPI void goat3d_get_track_quat(const struct goat3d_track *trk, long msec, float *xp, float *yp, float *zp, float *wp);

GOAT3DAPI long goat3d_get_track_timeline(const struct goat3d_track *trk, long *tstart, long *tend);

/* animation */
GOAT3DAPI int goat3d_add_anim(struct goat3d *g, struct goat3d_anim *anim);
GOAT3DAPI int goat3d_get_anim_count(const struct goat3d *g);
GOAT3DAPI struct goat3d_anim *goat3d_get_anim(const struct goat3d *g, int idx);
GOAT3DAPI struct goat3d_anim *goat3d_get_anim_by_name(const struct goat3d *g, const char *name);

GOAT3DAPI struct goat3d_anim *goat3d_create_anim(void);
GOAT3DAPI void goat3d_destroy_anim(struct goat3d_anim *anim);

GOAT3DAPI int goat3d_set_anim_name(struct goat3d_anim *anim, const char *name);
GOAT3DAPI const char *goat3d_get_anim_name(const struct goat3d_anim *anim);

GOAT3DAPI int goat3d_add_anim_track(struct goat3d_anim *anim, struct goat3d_track *trk);
GOAT3DAPI struct goat3d_track *goat3d_get_anim_track(const struct goat3d_anim *anim, int idx);
GOAT3DAPI struct goat3d_track *goat3d_get_anim_track_by_name(const struct goat3d_anim *anim, const char *name);
GOAT3DAPI struct goat3d_track *goat3d_get_anim_track_by_type(const struct goat3d_anim *anim, enum goat3d_track_type type);
GOAT3DAPI int goat3d_get_anim_track_count(const struct goat3d_anim *anim);

GOAT3DAPI long goat3d_get_anim_timeline(const struct goat3d_anim *anim, long *tstart, long *tend);

#ifdef __cplusplus
}
#endif

#endif	/* GOAT3D_H_ */
