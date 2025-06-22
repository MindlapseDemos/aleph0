#ifndef CURVE_H_
#define CURVE_H_

#include "cgmath/cgmath.h"

enum curve_type {
	CURVE_LINEAR,
	CURVE_HERMITE,
	CURVE_BSPLINE
};

struct curve {
	cgm_vec3 *cp;	/* dynarr */
	enum curve_type type;

	/* bounding box */
	cgm_vec3 bbmin, bbmax;
	int bbvalid;
};

struct curve *crv_alloc(void);
void crv_free(struct curve *cu);

int crv_init(struct curve *cu);
void crv_destroy(struct curve *cu);

void crv_clear(struct curve *cu);

int crv_add(struct curve *cu, const cgm_vec3 *p);
int crv_insert(struct curve *cu, const cgm_vec3 *p);
int crv_remove(struct curve *cu, int idx);
int crv_num_points(const struct curve *cu);

int crv_nearest(const struct curve *cu, const cgm_vec3 *p);

void crv_get_bbox(const struct curve *cu, cgm_vec3 *bbmin, cgm_vec3 *bbmax);
void crv_calc_bbox(struct curve *cu);

/* normalize the curve's bounds to coincide with the unit cube */
void crv_normalize(struct curve *cu);

/* project a point to the curve (nearest point on the curve) */
float crv_proj_param(const struct curve *cu, const cgm_vec3 *p, float refine_thres);
void crv_proj_point(const struct curve *cu, const cgm_vec3 *p, float refine_thres,
		cgm_vec3 *res);
/* equivalent to length(proj_point(p) - p) */
float crv_dist(const struct curve *cu, const cgm_vec3 *p);
float crv_dist_sq(const struct curve *cu, const cgm_vec3 *p);

void crv_eval_seg(const struct curve *cu, int a, int b, float t, cgm_vec3 *res);
void crv_eval(const struct curve *cu, float t, cgm_vec3 *res);

int crv_save(const struct curve *cu, const char *fname);
int crv_load(struct curve *cu, const char *fname);

#endif	/* CURVE_H_ */
