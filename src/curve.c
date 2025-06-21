#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "cgmath/cgmath.h"
#include "curve.h"
#include "dynarr.h"
#include "treestor.h"
#include "util.h"


struct curve *crv_alloc(void)
{
	struct curve *cu;

	if(!(cu = malloc(sizeof *cu))) {
		fprintf(stderr, "failed to allocate curve\n");
		return 0;
	}
	if(crv_init(cu) == -1) {
		free(cu);
		return 0;
	}
	return cu;
}

void crv_free(struct curve *cu)
{
	crv_destroy(cu);
	free(cu);
}

int crv_init(struct curve *cu)
{
	if(!(cu->cp = dynarr_alloc(0, sizeof *cu->cp))) {
		fprintf(stderr, "failed to allocate curve control point array\n");
		return -1;
	}
	cu->bbvalid = 0;
	cu->type = CURVE_BSPLINE;
	return 0;
}

void crv_destroy(struct curve *cu)
{
	dynarr_free(cu->cp);
}

void crv_clear(struct curve *cu)
{
	cu->cp = dynarr_clear(cu->cp);
}

int crv_add(struct curve *cu, const cgm_vec3 *p)
{
	void *tmp = dynarr_push(cu->cp, (cgm_vec3*)p);
	if(!tmp) return -1;
	cu->cp = tmp;
	cu->bbvalid = 0;
	return 0;
}

int crv_insert(struct curve *cu, const cgm_vec3 *p)
{
	return -1;	/* TODO (see curvedraw/src/curve.cc) */
}

int crv_remove(struct curve *cu, int idx)
{
	void *tmp;
	int last = dynarr_size(cu->cp) - 1;

	if(idx < 0 || idx > last) return -1;

	if(idx != last) {
		memmove(cu->cp + idx, cu->cp + idx + 1, (last - idx) * sizeof *cu->cp);
	}
	if(!(tmp = dynarr_pop(cu->cp))) {
		return -1;
	}
	cu->cp = tmp;
	cu->bbvalid = 0;
	return 0;
}

int crv_num_points(const struct curve *cu)
{
	return dynarr_size(cu->cp);
}

int crv_nearest(const struct curve *cu, const cgm_vec3 *p)
{
	int i, count, res = -1;
	float bestsq = FLT_MAX;

	count = dynarr_size(cu->cp);
	for(i=0; i<count; i++) {
		float d = cgm_vdist_sq(cu->cp + i, p);
		if(d < bestsq) {
			bestsq = d;
			res = i;
		}
	}
	return res;
}

void crv_get_bbox(const struct curve *cu, cgm_vec3 *bbmin, cgm_vec3 *bbmax)
{
	if(!cu->bbvalid) {
		crv_calc_bbox((struct curve*)cu);
	}
	*bbmin = cu->bbmin;
	*bbmax = cu->bbmax;
}

void crv_calc_bbox(struct curve *cu)
{
	int i, count;
	cgm_vec3 bmin, bmax, *vptr;

	cu->bbvalid = 1;

	if((count = dynarr_size(cu->cp)) <= 0) {
		cgm_vcons(&cu->bbmin, 0, 0, 0);
		cgm_vcons(&cu->bbmax, 0, 0, 0);
		return;
	}

	bmin = cu->cp[0];
	bmax = bmin;
	vptr = cu->cp;
	for(i=0; i<count; i++) {
		if(vptr->x < bmin.x) bmin.x = vptr->x;
		if(vptr->y < bmin.y) bmin.y = vptr->y;
		if(vptr->z < bmin.z) bmin.z = vptr->z;
		if(vptr->x > bmax.x) bmax.x = vptr->x;
		if(vptr->y > bmax.y) bmax.y = vptr->y;
		if(vptr->z > bmax.z) bmax.z = vptr->z;
		vptr++;
	}
	cu->bbmin = bmin;
	cu->bbmax = bmax;
}

/* normalize the curve's bounds to coincide with the unit cube */
void crv_normalize(struct curve *cu)
{
	int i, count;
	cgm_vec3 bsize, boffs, bscale, *vptr;

	if(!cu->bbvalid) {
		crv_calc_bbox(cu);
	}

	cgm_vcsub(&bsize, &cu->bbmax, &cu->bbmin);
	cgm_vcadd(&boffs, &cu->bbmin, &cu->bbmax);
	cgm_vscale(&boffs, 0.5f);

	bscale.x = bsize.x == 0.0f ? 1.0f : 1.0f / bsize.x;
	bscale.y = bsize.y == 0.0f ? 1.0f : 1.0f / bsize.y;
	bscale.z = bsize.z == 0.0f ? 1.0f : 1.0f / bsize.z;

	count = dynarr_size(cu->cp);
	vptr = cu->cp;
	for(i=0; i<count; i++) {
		vptr->x = (vptr->x - boffs.x) * bscale.x;
		vptr->y = (vptr->y - boffs.y) * bscale.y;
		vptr->z = (vptr->z - boffs.z) * bscale.z;
	}
	cu->bbvalid = 0;
}

/* project a point to the curve (nearest point on the curve) */
float crv_proj_param(const struct curve *cu, const cgm_vec3 *p, float refine_thres)
{
	int i;
	int num_cp = dynarr_size(cu->cp);
	int num_steps = num_cp * 5;	/* arbitrary number; sounds ok */
	float dt = 1.0f / (float)(num_steps - 1);

	float dist, dist_sq, best_distsq = FLT_MAX;
	float best_t = 0.0f;
	cgm_vec3 pp;

	float t = 0.0f;
	for(i=0; i<num_steps; i++) {
		crv_eval(cu, t, &pp);
		dist_sq = cgm_vdist_sq(&pp, p);
		if(dist_sq < best_distsq) {
			best_distsq = dist_sq;
			best_t = t;
		}
		t += dt;
	}

	/* refine by gradient descent */
	dist = best_distsq;
	t = best_t;
	dt *= 0.05;
	for(;;) {
		cgm_vec3 vn, vp;
		float dn, dp;
		float tn = t + dt;
		float tp = t - dt;

		crv_eval(cu, tn, &vn);
		crv_eval(cu, tp, &vp);

		dn = cgm_vdist_sq(&vn, p);
		dp = cgm_vdist_sq(&vp, p);

		if(fabs(dn - dp) < refine_thres * refine_thres) {
			break;
		}

		if(dn < dist) {
			t = tn;
			dist = dn;
		} else if(dp < dist) {
			t = tp;
			dist = dp;
		} else {
			break;	/* found the minimum */
		}
	}
	return t;
}

void crv_proj_point(const struct curve *cu, const cgm_vec3 *p, float refine_thres,
		cgm_vec3 *res)
{
	float t = crv_proj_param(cu, p, refine_thres);
	crv_eval(cu, t, res);
}

/* equivalent to length(proj_point(p) - p) */
float crv_dist(const struct curve *cu, const cgm_vec3 *p)
{
	cgm_vec3 pp;
	crv_proj_point(cu, p, 0.01f, &pp);
	return cgm_vdist(&pp, p);
}

float crv_dist_sq(const struct curve *cu, const cgm_vec3 *p)
{
	cgm_vec3 pp;
	crv_proj_point(cu, p, 0.01f, &pp);
	return cgm_vdist_sq(&pp, p);
}

void crv_eval_seg(const struct curve *cu, int a, int b, float t, cgm_vec3 *res)
{
	int num_cp, prev, next;

	num_cp = dynarr_size(cu->cp);

	if(t < 0.0f) t = 0.0f;
	if(t > 1.0f) t = 1.0f;

	if(cu->type == CURVE_LINEAR || num_cp == 2) {
		cgm_vlerp(res, cu->cp + a, cu->cp + b, t);
		return;
	}

	prev = a <= 0 ? a : a - 1;
	next = b >= num_cp - 1 ? b : b + 1;

	if(cu->type == CURVE_HERMITE) {
		res->x = cgm_spline(cu->cp[prev].x, cu->cp[a].x, cu->cp[b].x, cu->cp[next].x, t);
		res->y = cgm_spline(cu->cp[prev].y, cu->cp[a].y, cu->cp[b].y, cu->cp[next].y, t);
		res->z = cgm_spline(cu->cp[prev].z, cu->cp[a].z, cu->cp[b].z, cu->cp[next].z, t);
	} else {
		res->x = cgm_bspline(cu->cp[prev].x, cu->cp[a].x, cu->cp[b].x, cu->cp[next].x, t);
		res->y = cgm_bspline(cu->cp[prev].y, cu->cp[a].y, cu->cp[b].y, cu->cp[next].y, t);
		res->z = cgm_bspline(cu->cp[prev].z, cu->cp[a].z, cu->cp[b].z, cu->cp[next].z, t);
	}
}

void crv_eval(const struct curve *cu, float t, cgm_vec3 *res)
{
	int num_cp, idx0, idx1;
	float dt, t0, t1;

	if(!(num_cp = dynarr_size(cu->cp))) {
		cgm_vcons(res, 0, 0, 0);
		return;
	}
	if(num_cp == 1) {
		*res = cu->cp[0];
		return;
	}

	if(t < 0.0f) t = 0.0f;
	if(t > 1.0f) t = 1.0f;

	idx0 = cround64(t * (num_cp - 1));
	if(num_cp - 2 < idx0) idx0 = num_cp - 2;
	idx1 = idx0 + 1;

	dt = 1.0f / (float)(num_cp - 1);
	t0 = (float)idx0 * dt;
	t1 = (float)idx1 * dt;

	t = (t - t0) / (t1 - t0);
	crv_eval_seg(cu, idx0, idx1, t, res);
}

int crv_save(const struct curve *cu, const char *fname)
{
	int i;
	FILE *fp;
	cgm_vec3 *vptr;
	static const char *curve_type_str[] = { "polyline", "hermite", "bspline" };

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "crv_save: failed to open %s for writing\n", fname);
		return -1;
	}

	fprintf(fp, "curve {\n");
	fprintf(fp, "    type = \"%s\"\n", curve_type_str[cu->type]);
	fprintf(fp, "    cpcount = %d\n", dynarr_size(cu->cp));
	vptr = cu->cp;
	for(i=0; i<dynarr_size(cu->cp); i++) {
		fprintf(fp, "    cp = [%g %g %g 1]\n", vptr->x, vptr->y, vptr->z);
		vptr++;
	}
	fprintf(fp, "}\n");
	fclose(fp);
	return 0;

}

int crv_load(struct curve *cu, const char *fname)
{
	cgm_vec3 cp;
	struct ts_node *ts;
	struct ts_attr *attr;

	if(!(ts = ts_load(fname))) {
		fprintf(stderr, "crv_load: failed to open: %s\n", fname);
		return -1;
	}
	if(strcmp(ts->name, "curve") != 0){
		fprintf(stderr, "crv_load: invalid curve file: %s\n", fname);
		goto err;
	}

	if(crv_init(cu) == -1) {
		goto err;
	}

	attr = ts->attr_list;
	while(attr) {
		if(strcmp(attr->name, "type") == 0) {
			if(strcmp(attr->val.str, "polyline") == 0) {
				cu->type = CURVE_LINEAR;
			} else if(strcmp(attr->val.str, "hermite") == 0) {
				cu->type = CURVE_HERMITE;
			} else if(strcmp(attr->val.str, "bspline") == 0) {
				cu->type = CURVE_BSPLINE;
			} else {
				fprintf(stderr, "curve (%s) has an invalid type\n", fname);
				cu->type = CURVE_LINEAR;
			}

		} else if(strcmp(attr->name, "cp") == 0) {
			if(attr->val.type != TS_VECTOR || attr->val.vec_size < 3) {
				fprintf(stderr, "crv_load: ignoring invalid cp: %s\n", attr->val.str);
				attr = attr->next;
				continue;
			}
			cgm_vcons(&cp, attr->val.vec[0], attr->val.vec[1], attr->val.vec[2]);
			crv_add(cu, &cp);
		}
		attr = attr->next;
	}

	ts_free_tree(ts);
	return 0;

err:
	ts_free_tree(ts);
}
