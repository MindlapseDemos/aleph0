#ifndef CMESH_H_
#define CMESH_H_

#include <stdio.h>
#include "cgmath/cgmath.h"

enum {
	CMESH_ATTR_VERTEX,
	CMESH_ATTR_NORMAL,
	CMESH_ATTR_TANGENT,
	CMESH_ATTR_TEXCOORD,
	CMESH_ATTR_COLOR,
	CMESH_ATTR_BONEWEIGHTS,
	CMESH_ATTR_BONEIDX,
	CMESH_ATTR_TEXCOORD2,

	CMESH_NUM_ATTR
};

struct cmesh;

/* global state */
void cmesh_set_attrib_sdrloc(int attr, int loc);
int cmesh_get_attrib_sdrloc(int attr);
void cmesh_clear_attrib_sdrloc(void);

/* mesh functions */
struct cmesh *cmesh_alloc(void);
void cmesh_free(struct cmesh *cm);

int cmesh_init(struct cmesh *cm);
void cmesh_destroy(struct cmesh *cm);

void cmesh_clear(struct cmesh *cm);
int cmesh_clone(struct cmesh *cmdest, struct cmesh *cmsrc);

int cmesh_set_name(struct cmesh *cm, const char *name);
const char *cmesh_name(struct cmesh *cm);

int cmesh_has_attrib(struct cmesh *cm, int attr);
int cmesh_indexed(struct cmesh *cm);

/* vdata can be 0, in which case only memory is allocated
 * returns pointer to the attribute array
 */
float *cmesh_set_attrib(struct cmesh *cm, int attr, int nelem, unsigned int num,
		const float *vdata);
float *cmesh_attrib(struct cmesh *cm, int attr);			/* invalidates VBO */
const float *cmesh_attrib_ro(struct cmesh *cm, int attr);	/* doesn't invalidate */
float *cmesh_attrib_at(struct cmesh *cm, int attr, int idx);
const float *cmesh_attrib_at_ro(struct cmesh *cm, int attr, int idx);
int cmesh_attrib_count(struct cmesh *cm, int attr);
int cmesh_push_attrib(struct cmesh *cm, int attr, float *v);
int cmesh_push_attrib1f(struct cmesh *cm, int attr, float x);
int cmesh_push_attrib2f(struct cmesh *cm, int attr, float x, float y);
int cmesh_push_attrib3f(struct cmesh *cm, int attr, float x, float y, float z);
int cmesh_push_attrib4f(struct cmesh *cm, int attr, float x, float y, float z, float w);

/* indices can be 0, in which case only memory is allocated
 * returns pointer to the index array
 */
unsigned int *cmesh_set_index(struct cmesh *cm, int num, const unsigned int *indices);
unsigned int *cmesh_index(struct cmesh *cm);	/* invalidates IBO */
const unsigned int *cmesh_index_ro(struct cmesh *cm);	/* doesn't invalidate */
int cmesh_index_count(struct cmesh *cm);
int cmesh_push_index(struct cmesh *cm, unsigned int idx);

int cmesh_poly_count(struct cmesh *cm);

/* attr can be -1 to invalidate all attributes */
void cmesh_invalidate_vbo(struct cmesh *cm, int attr);
void cmesh_invalidate_ibo(struct cmesh *cm);

int cmesh_append(struct cmesh *cmdest, struct cmesh *cmsrc);

/* submeshes */
void cmesh_clear_submeshes(struct cmesh *cm);
/* a submesh is defined as a consecutive range of faces */
int cmesh_submesh(struct cmesh *cm, const char *name, int fstart, int fcount);
int cmesh_remove_submesh(struct cmesh *cm, int idx);
int cmesh_find_submesh(struct cmesh *cm, const char *name);
int cmesh_submesh_count(struct cmesh *cm);
int cmesh_clone_submesh(struct cmesh *cmdest, struct cmesh *cm, int subidx);

/* immediate-mode style mesh construction interface */
int cmesh_vertex(struct cmesh *cm, float x, float y, float z);
void cmesh_normal(struct cmesh *cm, float nx, float ny, float nz);
void cmesh_tangent(struct cmesh *cm, float tx, float ty, float tz);
void cmesh_texcoord(struct cmesh *cm, float u, float v, float w);
void cmesh_boneweights(struct cmesh *cm, float w1, float w2, float w3, float w4);
void cmesh_boneidx(struct cmesh *cm, int idx1, int idx2, int idx3, int idx4);

/* dir_xform can be null, in which case it's calculated from xform */
void cmesh_apply_xform(struct cmesh *cm, float *xform, float *dir_xform);

void cmesh_flip(struct cmesh *cm);	/* flip faces (winding) and normals */
void cmesh_flip_faces(struct cmesh *cm);
void cmesh_flip_normals(struct cmesh *cm);

int cmesh_explode(struct cmesh *cm);	/* undo all vertex sharing */

/* this is only guaranteed to work on an exploded mesh */
void cmesh_calc_face_normals(struct cmesh *cm);

void cmesh_draw(struct cmesh *cm);
void cmesh_draw_range(struct cmesh *cm, int start, int count);
void cmesh_draw_submesh(struct cmesh *cm, int subidx);	/* XXX only for indexed meshes currently */
void cmesh_draw_wire(struct cmesh *cm, float linesz);
void cmesh_draw_vertices(struct cmesh *cm, float ptsz);
void cmesh_draw_normals(struct cmesh *cm, float len);
void cmesh_draw_tangents(struct cmesh *cm, float len);

/* get the bounding box in local space. The result will be cached and subsequent
 * calls will return the same box. The cache gets invalidated by any functions that
 * can affect the vertex data
 */
void cmesh_aabbox(struct cmesh *cm, cgm_vec3 *vmin, cgm_vec3 *vmax);

/* get the bounding sphere in local space. The result will be cached ... see above */
float cmesh_bsphere(struct cmesh *cm, cgm_vec3 *center, float *rad);

/* texture coordinate manipulation */
void cmesh_texcoord_apply_xform(struct cmesh *cm, float *xform);
void cmesh_texcoord_gen_plane(struct cmesh *cm, cgm_vec3 *norm, cgm_vec3 *tang);
void cmesh_texcoord_gen_box(struct cmesh *cm);
void cmesh_texcoord_gen_cylinder(struct cmesh *cm);

/* FILE I/O */
int cmesh_load(struct cmesh *cm, const char *fname);

int cmesh_dump(struct cmesh *cm, const char *fname);
int cmesh_dump_file(struct cmesh *cm, FILE *fp);
int cmesh_dump_obj(struct cmesh *cm, const char *fname);
int cmesh_dump_obj_file(struct cmesh *cm, FILE *fp, int voffs);



#endif	/* CMESH_H_ */
