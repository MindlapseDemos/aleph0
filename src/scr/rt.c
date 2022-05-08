#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "rt.h"
#include "util.h"
#include "darray.h"

static cgm_vec3 cur_col, cur_spec;
static float cur_shin;
static struct image *cur_tex;

void rt_init(struct rtscene *scn)
{
	scn->obj = darr_alloc(0, sizeof *scn->obj);
	scn->num_obj = 0;
	scn->lt = darr_alloc(0, sizeof *scn->lt);
	scn->num_lt = 0;

	cgm_vcons(&cur_col, 1, 1, 1);
	cgm_vcons(&cur_spec, 0, 0, 0);
	cur_shin = 1;
	cur_tex = 0;
}

void rt_destroy(struct rtscene *scn)
{
	darr_free(scn->obj);
	darr_free(scn->lt);
	memset(scn, 0, sizeof *scn);
}


void rt_color(float r, float g, float b)
{
	cgm_vcons(&cur_col, r, g, b);
}

void rt_specular(float r, float g, float b)
{
	cgm_vcons(&cur_spec, r, g, b);
}

void rt_shininess(float s)
{
	cur_shin = s;
}

static union rtobject *add_object(struct rtscene *scn, enum rt_obj_type type)
{
	union rtobject *obj;

	obj = calloc_nf(1, sizeof *obj);
	obj->type = type;

	obj->x.mtl.kd = cur_col;
	obj->x.mtl.ks = cur_spec;
	obj->x.mtl.shin = cur_shin;
	obj->x.mtl.tex = cur_tex;

	darr_push(scn->obj, &obj);
	scn->num_obj = darr_size(scn->obj);
	return obj;
}

union rtobject *rt_add_sphere(struct rtscene *scn, float x, float y, float z, float r)
{
	union rtobject *obj = add_object(scn, RT_SPH);
	cgm_vcons(&obj->s.p, x, y, z);
	obj->s.r = r;
	return obj;
}

union rtobject *rt_add_plane(struct rtscene *scn, float nx, float ny, float nz, float d)
{
	union rtobject *obj = add_object(scn, RT_PLANE);
	cgm_vcons(&obj->p.n, nx, ny, nz);
	obj->p.d = d;
	return obj;
}

struct rtlight *rt_add_light(struct rtscene *scn, float x, float y, float z)
{
	struct rtlight *lt = calloc_nf(1, sizeof *lt);

	cgm_vcons(&lt->p, x, y, z);
	lt->color = cur_col;

	darr_push(scn->lt, &lt);
	scn->num_lt = darr_size(scn->lt);
	return lt;
}


/* color is initialized to black */
static void shade(struct rayhit *hit, struct rtscene *scn, int lvl, cgm_vec3 *color)
{
	int i;
	float ndotl, vdotr, spec;
	cgm_ray sray;
	cgm_vec3 col, rdir;
	struct rtlight *lt;
	struct rtmaterial *mtl = &hit->obj->x.mtl;

	sray.origin = hit->p;
	cgm_vnormalize(&hit->n);
	cgm_vnormalize(&hit->ray->dir);

	for(i=0; i<scn->num_lt; i++) {
		lt = scn->lt[i];
		sray.dir = lt->p;
		cgm_vsub(&sray.dir, &sray.origin);

		if(ray_scene(&sray, scn, 1.0f, 0)) continue;

		cgm_vnormalize(&sray.dir);
		ndotl = cgm_vdot(&sray.dir, &hit->n);
		if(ndotl < 0.0f) ndotl = 0.0f;

		rdir = hit->ray->dir;
		cgm_vreflect(&rdir, &hit->n);
		vdotr = cgm_vdot(&sray.dir, &rdir);
		if(vdotr < 0.0f) vdotr = 0.0f;
		spec = pow(vdotr, mtl->shin);

		color->x += (mtl->kd.x * ndotl + mtl->ks.x * spec) * lt->color.x;
		color->y += (mtl->kd.y * ndotl + mtl->ks.y * spec) * lt->color.y;
		color->z += (mtl->kd.z * ndotl + mtl->ks.z * spec) * lt->color.z;
	}
}

int ray_trace(cgm_ray *ray, struct rtscene *scn, int lvl, cgm_vec3 *color)
{
	struct rayhit hit;

	color->x = color->y = color->z = 0.0f;
	if(!ray_scene(ray, scn, FLT_MAX, &hit)) {
		return 0;
	}
	hit.ray = ray;
	shade(&hit, scn, lvl, color);
	return 1;
}


int ray_scene(cgm_ray *ray, struct rtscene *scn, float maxt, struct rayhit *hit)
{
	int i;

	if(hit) {
		struct rayhit hit0 = {FLT_MAX};

		/* find nearest hit */
		for(i=0; i<scn->num_obj; i++) {
			if(ray_object(ray, scn->obj[i], maxt, hit) && hit->t < hit0.t) {
				hit0 = *hit;
			}
		}

		if(hit0.obj) {
			*hit = hit0;
			return 1;
		}
	} else {
		/* find any hit */
		for(i=0; i<scn->num_obj; i++) {
			if(ray_object(ray, scn->obj[i], maxt, 0)) {
				return 1;
			}
		}
	}

	return 0;
}

int ray_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayhit *hit)
{
	switch(obj->type) {
	case RT_SPH:
		return ray_sphere(ray, &obj->s, maxt, hit);
	case RT_PLANE:
		return ray_plane(ray, &obj->p, maxt, hit);
	default:
		break;
	}
	return 0;
}

#define SQ(x)	((x) * (x))
int ray_sphere(cgm_ray *ray, struct rtsphere *sph, float maxt, struct rayhit *hit)
{
	float a, a2, b, c, d, sqrt_d, t1, t2;

	a = SQ(ray->dir.x) + SQ(ray->dir.y) + SQ(ray->dir.z);
	b = 2.0f * ray->dir.x * (ray->origin.x - sph->p.x) +
		2.0f * ray->dir.y * (ray->origin.y - sph->p.y) +
		2.0f * ray->dir.z * (ray->origin.z - sph->p.z);
	c = SQ(sph->p.x) + SQ(sph->p.y) + SQ(sph->p.z) +
		SQ(ray->origin.x) + SQ(ray->origin.y) + SQ(ray->origin.z) +
		2.0f * (-sph->p.x * ray->origin.x - sph->p.y * ray->origin.y - sph->p.z * ray->origin.z) -
		SQ(sph->r);

	if((d = SQ(b) - 4.0f * a * c) < 0.0f) return 0;

	sqrt_d = sqrt(d);
	a2 = 2.0f * a;
	t1 = (-b + sqrt_d) / a2;
	t2 = (-b - sqrt_d) / a2;

	if((t1 < 1e-5f && t2 < 1e-5f) || (t1 > maxt && t2 > maxt)) {
		return 0;
	}

	if(hit) {
		float t;
		if(t1 < 1e-5f) {
			t = t2;
		} else if(t2 < 1e-5f) {
			t = t1;
		} else {
			t = t1 < t2 ? t1 : t2;
		}

		hit->t = t;
		cgm_raypos(&hit->p, ray, t);

		hit->n.x = hit->p.x - sph->p.x;
		hit->n.y = hit->p.y - sph->p.y;
		hit->n.z = hit->p.z - sph->p.z;

		hit->obj = (union rtobject*)sph;
	}
	return 1;
}

int ray_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayhit *hit)
{
	cgm_vec3 vo;
	float t, ndotdir;

	ndotdir = cgm_vdot(&plane->n, &ray->dir);
	if(fabs(ndotdir) < 1e-5) {
		return 0;
	}

	vo.x = plane->n.x * plane->d - ray->origin.x;
	vo.y = plane->n.y * plane->d - ray->origin.y;
	vo.z = plane->n.z * plane->d - ray->origin.z;
	t = cgm_vdot(&plane->n, &vo) / ndotdir;

	if(t < 1e-5 || t > maxt) return 0;

	if(hit) {
		hit->t = t;
		cgm_raypos(&hit->p, ray, t);
		hit->n = plane->n;

		hit->obj = (union rtobject*)plane;
	}
	return 1;
}
