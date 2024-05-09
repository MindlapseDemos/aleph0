#ifndef RT_H_
#define RT_H_

#include "image.h"
#include "cgmath/cgmath.h"

#define RT_MAX_ITER	4

struct rtmaterial {
	cgm_vec3 kd, ks;
	float shin;
	float refl;
	struct image *tex;
	cgm_vec2 uvscale;
};

enum rt_obj_type { RT_SPH, RT_CYL, RT_PLANE, RT_BOX, RT_CSG };
enum rt_csg_op { RT_UNION, RT_ISECT, RT_DIFF };

#define OBJ_COMMON	\
	enum rt_obj_type type; \
	char *name; \
	float xform[16], inv_xform[16]; \
	int xform_valid; \
	struct rtmaterial mtl

struct rtany {
	OBJ_COMMON;
};

struct rtsphere {
	OBJ_COMMON;
	cgm_vec3 p;
	float r;
};

struct rtcylinder {
	OBJ_COMMON;
	cgm_vec3 p;
	float r, y0, y1;
};

struct rtplane {
	OBJ_COMMON;
	cgm_vec3 n;
	float d, rad_sq;
};

struct rtbox {
	OBJ_COMMON;
	cgm_vec3 min, max;
};

struct rtcsg {
	OBJ_COMMON;
	enum rt_csg_op op;
	union rtobject *a, *b;
};

union rtobject {
	enum rt_obj_type type;
	struct rtany x;
	struct rtsphere s;
	struct rtcylinder c;
	struct rtplane p;
	struct rtbox b;
	struct rtcsg csg;
};

struct rtlight {
	cgm_vec3 p, color;
};

struct rayhit {
	float t;
	cgm_vec3 p, n;
	float u, v;
	cgm_ray *ray;
	union rtobject *obj;
};

struct rayival {
	struct rayhit a, b;	/* a is always the nearest of the two */
};

#define RT_CSG_MAX_IVAL	16

struct rtscene {
	union rtobject **obj;
	int num_obj;
	struct rtlight **lt;
	int num_lt;

	union rtobject **disobj;		/* disabled objects */
};

/* scene management */
void rt_init(struct rtscene *scn);
void rt_destroy(struct rtscene *scn);

int rt_load(struct rtscene *scn, const char *fname);

void rt_ambient(float r, float g, float b);

void rt_name(const char *name);
void rt_color(float r, float g, float b);
void rt_specular(float r, float g, float b);
void rt_shininess(float s);
void rt_reflect(float refl);
void rt_texmap(struct image *img);
void rt_uvscale(float su, float sv);

union rtobject *rt_find_object(struct rtscene *scn, const char *name);
int rt_remove_object(struct rtscene *scn, union rtobject *obj);
int rt_disable_object(struct rtscene *scn, union rtobject *obj);

union rtobject *rt_add_sphere(struct rtscene *scn, float x, float y, float z, float r);
union rtobject *rt_add_cylinder(struct rtscene *scn, float x, float y, float z, float r, float y0, float y1);
union rtobject *rt_add_plane(struct rtscene *scn, float nx, float ny, float nz, float d);
union rtobject *rt_add_box(struct rtscene *scn, float x, float y, float z, float dx, float dy, float dz);
union rtobject *rt_add_csg(struct rtscene *scn, enum rt_csg_op op, union rtobject *a, union rtobject *b);
struct rtlight *rt_add_light(struct rtscene *scn, float x, float y, float z);

/* returns 0 for no hit */
int ray_trace(cgm_ray *ray, struct rtscene *scn, int lvl, cgm_vec3 *color);

int ray_scene(cgm_ray *ray, struct rtscene *scn, float maxt, struct rayhit *hit);

int ray_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayhit *hit);
int ray_sphere(cgm_ray *ray, struct rtsphere *sph, float maxt, struct rayhit *hit);
int ray_cylinder(cgm_ray *ray, struct rtcylinder *cyl, float maxt, struct rayhit *hit);
int ray_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayhit *hit);
int ray_box(cgm_ray *ray, struct rtbox *box, float maxt, struct rayhit *hit);

/* csg functions return the number of ray intervals found (up to RT_CSG_MAX_IVAL).
 * ivlist must be a pointer to an array of at least RT_CSG_MAX_IVAL rayival structures.
 */
int ray_csg_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayival *ivlist);
int ray_csg_sphere(cgm_ray *ray, struct rtsphere *sph, float maxt, struct rayival *ivlist);
int ray_csg_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayival *ivlist);
int ray_csg_box(cgm_ray *ray, struct rtbox *box, float maxt, struct rayival *ivlist);

void rt_print_tree(struct rtscene *scn);

#endif	/* RT_H_ */
