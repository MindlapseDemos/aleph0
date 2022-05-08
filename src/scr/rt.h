#ifndef RT_H_
#define RT_H_

#include "image.h"
#include "cgmath/cgmath.h"

struct rtmaterial {
	cgm_vec3 kd, ks;
	float shin;
	struct image *tex;
};

enum rt_obj_type { RT_SPH, RT_PLANE };

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

union rtobject {
	enum rt_obj_type type;
	struct rtany x;
	struct rtsphere s;
	struct rtplane p;
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

struct rtscene {
	union rtobject **obj;
	int num_obj;
	struct rtlight **lt;
	int num_lt;
};

/* scene management */
void rt_init(struct rtscene *scn);
void rt_destroy(struct rtscene *scn);

void rt_color(float r, float g, float b);
void rt_specular(float r, float g, float b);
void rt_shininess(float s);

union rtobject *rt_add_sphere(struct rtscene *scn, float x, float y, float z, float r);
union rtobject *rt_add_plane(struct rtscene *scn, float nx, float ny, float nz, float d);
struct rtlight *rt_add_light(struct rtscene *scn, float x, float y, float z);

/* returns 0 for no hit */
int ray_trace(cgm_ray *ray, struct rtscene *scn, int lvl, cgm_vec3 *color);

int ray_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayhit *hit);
int ray_scene(cgm_ray *ray, struct rtscene *scn, float maxt, struct rayhit *hit);
int ray_sphere(cgm_ray *ray, struct rtsphere *sph, float maxt, struct rayhit *hit);
int ray_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayhit *hit);

#endif	/* RT_H_ */
