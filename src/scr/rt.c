#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "demo.h"
#include "rt.h"
#include "util.h"
#include "darray.h"
#include "treestor.h"

static cgm_vec3 ambient;
static cgm_vec3 cur_col, cur_spec;
static float cur_shin;
static struct image *cur_tex;
static char *cur_name;

static union rtobject *load_object(struct rtscene *scn, struct ts_node *node);
static union rtobject *load_csg(struct rtscene *scn, struct ts_node *node, int op);

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
	int i;

	for(i=0; i<darr_size(scn->obj); i++) {
		free(scn->obj[i]->x.name);
		free(scn->obj[i]);
	}
	darr_free(scn->obj);

	for(i=0; i<darr_size(scn->lt); i++) {
		free(scn->lt[i]);
	}
	darr_free(scn->lt);

	memset(scn, 0, sizeof *scn);
}

int rt_load(struct rtscene *scn, const char *fname)
{
	static float defamb[] = {0.15, 0.15, 0.15};
	struct ts_node *root, *c;
	float *vec;

	if(!(root = ts_load(fname))) {
		fprintf(stderr, "failed to open %s\n", fname);
		return -1;
	}
	if(strcmp(root->name, "rtscene") != 0) {
		fprintf(stderr, "invalid scene file: %s\n", fname);
		ts_free_tree(root);
		return -1;
	}

	vec = ts_get_attr_vec(root, "ambient", defamb);
	rt_ambient(vec[0], vec[1], vec[2]);

	c = root->child_list;
	while(c) {
		load_object(scn, c);
		c = c->next;
	}

	ts_free_tree(root);
	return 0;
}

static union rtobject *load_object(struct rtscene *scn, struct ts_node *node)
{
	static float zerovec[3], defnorm[] = {0, 1, 0}, defsize[] = {1, 1, 1};
	float *kd, *ks, defkd[] = {1, 1, 1};
	union rtobject *obj = 0;
	struct ts_node *c;

	rt_name(ts_get_attr_str(node, "name", 0));

	kd = ts_get_attr_vec(node, "color", defkd);
	ks = ts_get_attr_vec(node, "specular", zerovec);

	rt_color(kd[0], kd[1], kd[2]);
	rt_specular(ks[0], ks[1], ks[2]);
	rt_shininess(ts_get_attr_num(node, "shininess", 60.0f));

	if(strcmp(node->name, "sphere") == 0) {
		float rad = ts_get_attr_num(node, "r", 1.0f);
		float *pos = ts_get_attr_vec(node, "pos", zerovec);

		obj = rt_add_sphere(scn, pos[0], pos[1], pos[2], rad);

	} else if(strcmp(node->name, "plane") == 0) {
		float *n = ts_get_attr_vec(node, "n", defnorm);
		float d = ts_get_attr_num(node, "d", 0);

		obj = rt_add_plane(scn, n[0], n[1], n[2], d);

	} else if(strcmp(node->name, "box") == 0) {
		float *pos = ts_get_attr_vec(node, "pos", zerovec);
		float *size = ts_get_attr_vec(node, "size", defsize);

		obj = rt_add_box(scn, pos[0], pos[1], pos[2], size[0], size[1], size[2]);

	} else if(strcmp(node->name, "union") == 0) {
		obj = load_csg(scn, node, RT_UNION);

	} else if(strcmp(node->name, "intersection") == 0) {
		obj = load_csg(scn, node, RT_ISECT);

	} else if(strcmp(node->name, "difference") == 0) {
		obj = load_csg(scn, node, RT_DIFF);

	}

	return obj;
}

static union rtobject *load_csg(struct rtscene *scn, struct ts_node *node, int op)
{
	union rtobject *root = 0, *obj[2], *otmp;
	int num_obj = 0;
	struct ts_node *cn = node->child_list;

	while(cn) {
		if(!(obj[num_obj] = load_object(scn, cn))) {
			cn = cn->next;
			continue;
		}
		if(++num_obj >= 2) {
			if(!(otmp = rt_add_csg(scn, op, obj[0], obj[1]))) {
				demo_abort();
			}
			if(!root) root = otmp;
			obj[0] = otmp;
			num_obj = 1;
		}
		cn = cn->next;
	}
	return root;
}

void rt_ambient(float r, float g, float b)
{
	cgm_vcons(&ambient, r, g, b);
}

void rt_name(const char *name)
{
	free(cur_name);
	if(name) {
		cur_name = strdup_nf(name);
	} else {
		cur_name = 0;
	}
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

	if(cur_name) obj->x.name = strdup_nf(cur_name);

	obj->x.mtl.kd = cur_col;
	obj->x.mtl.ks = cur_spec;
	obj->x.mtl.shin = cur_shin;
	obj->x.mtl.tex = cur_tex;

	darr_push(scn->obj, &obj);
	scn->num_obj = darr_size(scn->obj);
	return obj;
}

union rtobject *rt_find_object(struct rtscene *scn, const char *name)
{
	int i;

	for(i=0; i<scn->num_obj; i++) {
		if(scn->obj[i]->x.name && strcmp(scn->obj[i]->x.name, name) == 0) {
			return scn->obj[i];
		}
	}
	return 0;
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

union rtobject *rt_add_box(struct rtscene *scn, float x, float y, float z, float dx, float dy, float dz)
{
	union rtobject *obj = add_object(scn, RT_BOX);

	dx /= 2;
	dy /= 2;
	dz /= 2;

	cgm_vcons(&obj->b.min, x - dx, y - dy, z - dz);
	cgm_vcons(&obj->b.max, x + dx, y + dy, z + dz);
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
	float inv_dir[3];
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	cgm_vec3 p;
	struct rayhit *hit;

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
