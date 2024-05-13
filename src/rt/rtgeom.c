#include "rt.h"
#include "util.h"

int ray_object(cgm_ray *ray, union rtobject *obj, float maxt, struct rayhit *hit)
{
	int i, numiv;
	struct rayival ivlist[RT_CSG_MAX_IVAL];

	switch(obj->type) {
	case RT_SPH:
		return ray_sphere(ray, &obj->s, maxt, hit);
	case RT_CYL:
		return ray_cylinder(ray, &obj->c, maxt, hit);
	case RT_PLANE:
		return ray_plane(ray, &obj->p, maxt, hit);
	case RT_BOX:
		return ray_box(ray, &obj->b, maxt, hit);
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

		if(sph->mtl.tex) {
			hit->u = atan2(hit->n.z, hit->n.x);
			hit->v = acos(hit->n.y);
		}

		hit->obj = (union rtobject*)sph;
	}
	return 1;
}

static INLINE int ray_cylcaps(cgm_ray *ray, struct rtcylinder *cyl, float maxt, struct rayhit *hit)
{
	int res = 0;
	float t, ttop, ndotdir, x, z, dx, dz;

	ndotdir = ray->dir.y;
	if(fabs(ndotdir) < 1e-5f) {
		return 0;
	}

	ttop = maxt;

	/* top cap */
	ttop = (cyl->y1 - ray->origin.y) / ndotdir;
	if(ttop >= 1e-5f && ttop < maxt) {
		x = ray->origin.x + ray->dir.x * ttop;
		z = ray->origin.z + ray->dir.z * ttop;
		dx = x - cyl->p.x;
		dz = z - cyl->p.z;
		if(dx * dx + dz * dz < cyl->r * cyl->r) {
			if(!hit) return 1;
			res = 1;
			hit->t = ttop;
			cgm_vcons(&hit->p, x, ray->origin.y + ray->dir.y * ttop, z);
			hit->n.x = hit->n.z = 0;
			hit->n.y = 1;
			hit->u = x / cyl->r;
			hit->v = z / cyl->r;
			hit->obj = (union rtobject*)cyl;
		}
	}
	/* bottom cap */
	t = -(cyl->y0 - ray->origin.y) / -ndotdir;
	if(t >= 1e-5f && t < ttop) {
		x = ray->origin.x + ray->dir.x * t;
		z = ray->origin.z + ray->dir.z * t;
		dx = x - cyl->p.x;
		dz = z - cyl->p.z;
		if(dx * dx + dz * dz < cyl->r * cyl->r) {
			if(!hit) return 1;
			hit->t = t;
			cgm_vcons(&hit->p, x, ray->origin.y + ray->dir.y * t, z);
			hit->n.x = hit->n.z = 0;
			hit->n.y = -1;
			hit->u = x / cyl->r;
			hit->v = z / cyl->r;
			hit->obj = (union rtobject*)cyl;
			return 1;
		}
	}
	return res;
}

int ray_cylinder(cgm_ray *ray, struct rtcylinder *cyl, float maxt, struct rayhit *hit)
{
	int res = 0;
	float a, a2, b, c, d, sqrt_d, t, tcaps, t1, t2;
	cgm_vec3 p;

	if(cyl->capped && ray_cylcaps(ray, cyl, maxt, hit)) {
		if(!hit) return 1;
		tcaps = hit->t;
		res = 1;
	} else {
		tcaps = maxt;
	}

	a = SQ(ray->dir.x) + SQ(ray->dir.z);
	b = 2.0f * ray->dir.x * (ray->origin.x - cyl->p.x) +
		2.0f * ray->dir.z * (ray->origin.z - cyl->p.z);
	c = SQ(cyl->p.x) + SQ(cyl->p.z) +
		SQ(ray->origin.x) + SQ(ray->origin.z) +
		2.0f * (-cyl->p.x * ray->origin.x - cyl->p.z * ray->origin.z) - SQ(cyl->r);

	if((d = SQ(b) - 4.0f * a * c) < 0.0f) return res;

	sqrt_d = sqrt(d);
	a2 = 2.0f * a;
	t1 = (-b + sqrt_d) / a2;
	t2 = (-b - sqrt_d) / a2;

	if((t1 < 1e-4f && t2 < 1e-4f) || (t1 > maxt && t2 > maxt)) {
		return res;
	}

	if(t1 < 1e-4f) {
		t1 = t2;
	} else if(t2 < 1e-4f) {
		t2 = t1;
	} else {
		if(t2 < t1) {
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
	}


	cgm_raypos(&p, ray, t1);
	if(p.y >= cyl->y0 && p.y <= cyl->y1) {
		t = t1;
	} else {
		if(t1 == t2) return res;
		t = t2;
		cgm_raypos(&p, ray, t2);
		if(p.y < cyl->y0 || p.y > cyl->y1) {
			return res;
		}
	}
	res = 1;

	if(hit && t < tcaps) {
		hit->t = t;
		hit->p = p;

		hit->n.x = hit->p.x - cyl->p.x;
		hit->n.y = 0;
		hit->n.z = hit->p.z - cyl->p.z;

		if(cyl->mtl.tex) {
			hit->u = atan2(hit->n.z, hit->n.x);
			hit->v = (hit->n.y - cyl->y0) / (cyl->y1 - cyl->y0);
		}

		hit->obj = (union rtobject*)cyl;
	}
	return res;
}

int ray_plane(cgm_ray *ray, struct rtplane *plane, float maxt, struct rayhit *hit)
{
	cgm_vec3 vo, p;
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

	cgm_raypos(&p, ray, t);
	if(plane->rad_sq < cgm_vlength_sq(&p)) {
		return 0;
	}

	if(hit) {
		hit->t = t;
		hit->p = p;
		hit->n = plane->n;

		if(plane->mtl.tex) {
			/* XXX: generalize if needed */
			hit->u = hit->p.x;
			hit->v = hit->p.z;
		}

		hit->obj = (union rtobject*)plane;
	}
	return 1;
}

int ray_box(cgm_ray *ray, struct rtbox *box, float maxt, struct rayhit *hit)
{
	int i, axis[2], sign[3];
	float param[2][3];
	float inv_dir[3];
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	cgm_vec3 p;

	for(i=0; i<3; i++) {
		param[0][i] = (&box->min.x)[i];
		param[1][i] = (&box->max.x)[i];

		inv_dir[i] = 1.0f / (&ray->dir.x)[i];
		sign[i] = inv_dir[i] < 0;
	}

	tmin = (param[sign[0]][0] - ray->origin.x) * inv_dir[0];
	tmax = (param[1 - sign[0]][0] - ray->origin.x) * inv_dir[0];
	tymin = (param[sign[1]][1] - ray->origin.y) * inv_dir[1];
	tymax = (param[1 - sign[1]][1] - ray->origin.y) * inv_dir[1];

	axis[0] = axis[1] = 0;

	if(tmin > tymax || tymin > tmax) {
		return 0;
	}
	if(tymin > tmin) {
		tmin = tymin;
		axis[0] = 1;
	}
	if(tymax < tmax) {
		tmax = tymax;
		axis[1] = 1;
	}

	tzmin = (param[sign[2]][2] - ray->origin.z) * inv_dir[2];
	tzmax = (param[1 - sign[2]][2] - ray->origin.z) * inv_dir[2];

	if(tmin > tzmax || tzmin > tmax) {
		return 0;
	}
	if(tzmin > tmin) {
		tmin = tzmin;
		axis[0] = 2;
	}
	if(tzmax < tmax) {
		tmax = tzmax;
		axis[1] = 2;
	}

	if((tmin < 1e-5 && tmax < 1e-5) || (tmin > maxt && tmax > maxt)) {
		return 0;
	}

	if(hit) {
		int ax;
		if(tmin < 0) {
			hit->t = tmax;
			ax = axis[1];
		} else {
			hit->t = tmin;
			ax = axis[0];
		}
		cgm_raypos(&p, ray, hit->t);
		hit->p = p;
		switch(ax) {
		case 0:
			hit->n.x = p.x > (box->min.x + box->max.x) / 2.0f ? 1.0f : -1.0f;
			hit->n.y = hit->n.z = 0.0f;
			break;
		case 1:
			hit->n.y = p.y > (box->min.y + box->max.y) / 2.0f ? 1.0f : -1.0f;
			hit->n.x = hit->n.z = 0.0f;
			break;
		case 2:
		default:
			hit->n.z = p.z > (box->min.z + box->max.z) / 2.0f ? 1.0f : -1.0f;
			hit->n.x = hit->n.y = 0.0f;
		}
		hit->ray = ray;
		hit->obj = (union rtobject*)box;
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

	case RT_BOX:
		return ray_csg_box(ray, &obj->b, maxt, ivlist);

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

int ray_csg_box(cgm_ray *ray, struct rtbox *box, float maxt, struct rayival *ival)
{
	int i, axis[2], sign[3];
	float param[2][3];
	float inv_dir[3], *vec;
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	cgm_vec3 p;
	struct rayhit *hit;

	for(i=0; i<3; i++) {
		param[0][i] = (&box->min.x)[i];
		param[1][i] = (&box->max.x)[i];

		vec = &ray->dir.x;
		inv_dir[i] = vec[i] == 0 ? 0 : 1.0f / vec[i];
		sign[i] = inv_dir[i] < 0;
	}

	tmin = (param[sign[0]][0] - ray->origin.x) * inv_dir[0];
	tmax = (param[1 - sign[0]][0] - ray->origin.x) * inv_dir[0];
	tymin = (param[sign[1]][1] - ray->origin.y) * inv_dir[1];
	tymax = (param[1 - sign[1]][1] - ray->origin.y) * inv_dir[1];

	axis[0] = axis[1] = 0;

	if(tmin > tymax || tymin > tmax) {
		return 0;
	}
	if(tymin > tmin) {
		tmin = tymin;
		axis[0] = 1;
	}
	if(tymax < tmax) {
		tmax = tymax;
		axis[1] = 1;
	}

	tzmin = (param[sign[2]][2] - ray->origin.z) * inv_dir[2];
	tzmax = (param[1 - sign[2]][2] - ray->origin.z) * inv_dir[2];

	if(tmin > tzmax || tzmin > tmax) {
		return 0;
	}
	if(tzmin > tmin) {
		tmin = tzmin;
		axis[0] = 2;
	}
	if(tzmax < tmax) {
		tmax = tzmax;
		axis[1] = 2;
	}

	if((tmin < 1e-5 && tmax < 1e-5) || (tmin > maxt && tmax > maxt)) {
		return 0;
	}

	ival->a.t = tmin;
	ival->b.t = tmax;
	hit = &ival->a;
	for(i=0; i<2; i++) {
		cgm_raypos(&p, ray, hit->t);
		hit->p = p;
		switch(axis[i]) {
		case 0:
			hit->n.x = p.x > (box->min.x + box->max.x) / 2.0f ? 1.0f : -1.0f;
			hit->n.y = hit->n.z = 0.0f;
			break;
		case 1:
			hit->n.y = p.y > (box->min.y + box->max.y) / 2.0f ? 1.0f : -1.0f;
			hit->n.x = hit->n.z = 0.0f;
			break;
		case 2:
		default:
			hit->n.z = p.z > (box->min.z + box->max.z) / 2.0f ? 1.0f : -1.0f;
			hit->n.x = hit->n.y = 0.0f;
		}
		hit->ray = ray;
		hit->obj = (union rtobject*)box;
		hit = &ival->b;
	}
	return 1;
}
