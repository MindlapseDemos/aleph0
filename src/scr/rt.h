#ifndef RT_H_
#define RT_H_

#include "image.h"
#include "cgmath/cgmath.h"

struct rtmaterial {
	cgm_vec3 kd, ks;
	float shin;
	struct image *tex;
};

enum rt_obj_type { RT_SPH, RT_PLANE, RT_CSG };
enum rt_csg_op { RT_UNION, RT_ISECT, RT_DIFF };

#define OBJ_COMMON	\
	enum rt_obj_type type; \
	struct rtmaterial mtl

struct rtany {
	OBJ_COMMON;
};

struct rtsphere {
	OBJ_COMMON;
	cgm_vec3 p;
	float r;
};

struct rtplane {
	OBJ_COMMON;
	cgm_vec3 n;
	float d;
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
	struct rtplane p;
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
};

/* scene management */
void rt_init(struct rtscene *scn);
void rt_destroy(struct rtscene *scn);

void rt_ambient(float r, float g, float b);
void rt_color(float r, float g, float b);
void rt_specular(float r, float g, float b);
void rt_shininess(float s);

int rt_remove_object(struct rtscene *scn, union rtobject *obj);

union rtobject *rt_add_sphere(struct rtscene *scn, float x, float y, float z, float r);
union rtobject *rt_add_plane(struct rtscene *scn, float nx, float ny, float nz, float d);
union rtobject *rt_add_csg(struct rtscene *scn, enum rt_csg_op op, union rtobject *a, union rtobject *b);
struct rtlight *rt_add_light(struct rtscene *scn, float x, float y, float z);

/* returns 0 for no hit */
int ray_trace(cgm_ray *ray, struct rtscene *scn, int lvl, cgm_vec3 *color);

int ray_scene(cgm_ray *ray, struct rtscene *scn, float maxt, struct rayhit *hit);

int ray_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayhit *hit);
int ray_sphere(cgm_ray *ray, struct rtsphere *sph, float maxt, struct rayhit *hit);
int ray_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayhit *hit);

/* csg functions return the number of ray intervals found (up to RT_CSG_MAX_IVAL).
 * ivlist must be a pointer to an array of at least RT_CSG_MAX_IVAL rayival structures.
 */
int ray_csg_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayival *ivlist);
int ray_csg_sphere(cgm_ray *ray, struct rtsphere *sph, float maxt, struct rayival *ivlist);
int ray_csg_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayival *ivlist);

#endif	/* RT_H_ */
