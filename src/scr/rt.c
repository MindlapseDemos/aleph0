#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "rt.h"
#include "util.h"
#include "darray.h"

static cgm_vec3 ambient;
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

void rt_ambient(float r, float g, float b)
{
	cgm_vcons(&ambient, r, g, b);
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

int rt_remove_object(struct rtscene *scn, union rtobject *obj)
{
	int i, last = scn->num_obj - 1;

	for(i=0; i<scn->num_obj; i++) {
		if(scn->obj[i] == obj) {
			if(i < last) {
				scn->obj[i] = scn->obj[last];
			}
			darr_pop(scn->obj);
			scn->num_obj--;
			return 0;
		}
	}
	return -1;
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

union rtobject *rt_add_csg(struct rtscene *scn, enum rt_csg_op op, union rtobject *a, union rtobject *b)
{
	union rtobject *obj = add_object(scn, RT_CSG);
	obj->csg.op = op;
	obj->csg.a = a;
	obj->csg.b = b;

	rt_remove_object(scn, a);
	rt_remove_object(scn, b);

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

	if(cgm_vdot(&hit->n, &hit->ray->dir) > 0.0f) {
		cgm_vneg(&hit->n);	/* faceforward */
	}

	sray.origin = hit->p;
	cgm_vnormalize(&hit->n);
	cgm_vnormalize(&hit->ray->dir);

	color->x = mtl->kd.x * ambient.x;
	color->y = mtl->kd.y * ambient.y;
	color->z = mtl->kd.z * ambient.z;

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
	int i, numiv;
	struct rayival ivlist[RT_CSG_MAX_IVAL];

	switch(obj->type) {
	case RT_SPH:
		return ray_sphere(ray, &obj->s, maxt, hit);
	case RT_PLANE:
		return ray_plane(ray, &obj->p, maxt, hit);
	case RT_CSG:
		if(!(numiv = ray_csg_object(ray, obj, maxt, ivlist))) {
			return 0;
		}
		for(i=0; i<numiv; i++) {
			/* return the first intersection point in front of the origin */
			if(ivlist[i].a.t >= 1e-5) {
				if(hit)	*hit = ivlist[i].a;
				return 1;
			}
			if(ivlist[i].b.t >= 1e-5) {
				if(hit) *hit = ivlist[i].b;
				return 1;
			}
		}
		return 0;	/* all intersection points behind the origin */

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

int ray_csg_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayival *ivlist)
{
	int i, j, numa, numb, num;
	struct rayival iva[RT_CSG_MAX_IVAL], ivb[RT_CSG_MAX_IVAL], *aptr, *bptr, *aend, *bend;

	switch(obj->type) {
	case RT_SPH:
		return ray_csg_sphere(ray, &obj->s, maxt, ivlist);

	case RT_PLANE:
		return ray_csg_plane(ray, &obj->p, maxt, ivlist);

	case RT_CSG:
		switch(obj->csg.op) {
		case RT_UNION:
			numa = ray_csg_object(ray, obj->csg.a, maxt, iva);
			numb = ray_csg_object(ray, obj->csg.b, maxt, ivb);
			if(!numa) {
				if(numb) memcpy(ivlist, ivb, numb * sizeof *ivlist);
				return numb;
			}
			if(!numb) {
				memcpy(ivlist, iva, numa * sizeof *ivlist);
				return numa;
			}
			aptr = iva;
			bptr = ivb;
			aend = iva + numa;
			bend = ivb + numb;
			if(aptr->a.t < bptr->a.t) {
				*ivlist = *aptr++;
			} else {
				*ivlist = *bptr++;
			}
			num = 1;
			while(num < RT_CSG_MAX_IVAL) {
				if(aptr == aend) {
					if(bptr == bend) break;
					*++ivlist = *bptr++;
					num++;
					continue;
				}
				if(bptr == bend) {
					*++ivlist = *aptr++;
					num++;
					continue;
				}
				/* both have intervals */
				if(aptr->a.t < bptr->a.t) {
					if(aptr->a.t < ivlist->b.t) {	/* current overlaps A */
						if(aptr->b.t > ivlist->b.t) {
							ivlist->b.t = aptr->b.t;	/* extend current */
						}
						aptr++;
					} else {
						*++ivlist = *aptr++;		/* no overlap, append A */
						num++;
					}
				} else {
					if(bptr->a.t < ivlist->b.t) {	/* overlaps B */
						if(bptr->b.t > ivlist->b.t) {
							ivlist->b.t = bptr->b.t;	/* extend current */
						}
						bptr++;
					} else {
						*++ivlist = *bptr++;
						num++;
					}
				}
			}
			return num;

		case RT_ISECT:
			if(!(numa = ray_csg_object(ray, obj->csg.a, maxt, iva))) return 0;
			if(!(numb = ray_csg_object(ray, obj->csg.b, maxt, ivb))) return 0;

			/* for each interval in A add all intersections with the intervals in B */
			num = 0;
			aptr = iva;
			for(i=0; i<numa; i++) {
				bptr = ivb;
				for(j=0; j<numb; j++) {
					/* if disjoint, skip */
					if(aptr->b.t < bptr->a.t || aptr->a.t > bptr->b.t) {
						bptr++;
						continue;
					}
					ivlist->a = aptr->a.t > bptr->a.t ? aptr->a : bptr->a;
					ivlist->b = aptr->b.t < bptr->b.t ? aptr->b : bptr->b;
					ivlist++;
					if(++num >= RT_CSG_MAX_IVAL) return num;
					bptr++;
				}
				aptr++;
			}
			return num;

		case RT_DIFF:
			if(!(numa = ray_csg_object(ray, obj->csg.a, maxt, iva))) return 0;
			if(!(numb = ray_csg_object(ray, obj->csg.b, maxt, ivb))) {
				memcpy(ivlist, iva, numa * sizeof *ivlist);
				return numa;
			}

			num = 0;
			for(i=0; i<numa; i++) {
				/* A is current ... */
				*ivlist = iva[i];

				bptr = ivb;
				for(j=0; j<numb; j++) {
					/* if disjoint, continue */
					if(ivlist->b.t <= bptr->a.t || ivlist->a.t >= bptr->b.t) {
						continue;
					}
					if(ivlist->a.t > bptr->a.t) ivlist->a = bptr->b;
					if(ivlist->b.t < bptr->b.t) ivlist->b = bptr->a;
					bptr++;

					if(ivlist->a.t >= ivlist->b.t) break;
				}

				if(ivlist->a.t < ivlist->b.t) {
					/* ended up with a valid interval, keep it */
					ivlist++;
					if(++num >= RT_CSG_MAX_IVAL) {
						break;
					}
				}
			}
			return num;

		default:
			break;
		}

	default:
		break;
	}
	return 0;
}

int ray_csg_sphere(cgm_ray *ray, struct rtsphere *sph, float maxt, struct rayival *ival)
{
	int i;
	float a, a2, b, c, d, sqrt_d, t[2];
	struct rayhit *hit;

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
	t[0] = (-b + sqrt_d) / a2;
	t[1] = (-b - sqrt_d) / a2;

	if((t[0] < 1e-5f && t[1] < 1e-5f) || (t[0] > maxt && t[1] > maxt)) {
		return 0;
	}

	if(t[1] < t[0]) {
		float tmp = t[0];
		t[0] = t[1];
		t[1] = tmp;
	}

	hit = &ival[0].a;
	for(i=0; i<2; i++) {
		hit->t = t[i];
		cgm_raypos(&hit->p, ray, t[i]);

		hit->n.x = hit->p.x - sph->p.x;
		hit->n.y = hit->p.y - sph->p.y;
		hit->n.z = hit->p.z - sph->p.z;

		hit->obj = (union rtobject*)sph;

		hit = &ival[0].b;
	}
	return 1;	/* always 1 interval */
}

int ray_csg_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayival *ival)
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

	ival->a.t = t;
	cgm_raypos(&ival->a.p, ray, t);
	ival->a.n = plane->n;
	ival->a.obj = (union rtobject*)plane;

	ival->b.t = maxt;
	cgm_raypos(&ival->b.p, ray, maxt);
	ival->b.n.x = -ray->dir.x;
	ival->b.n.y = -ray->dir.y;
	ival->b.n.z = -ray->dir.z;
	ival->b.obj = ival->a.obj;
	return 1;
}
