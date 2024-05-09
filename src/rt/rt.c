#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "demo.h"
#include "rt.h"
#include "util.h"
#include "gfxutil.h"
#include "darray.h"
#include "treestor.h"

struct texture {
	char *name;
	struct image *img;
};
static struct texture *texlist;

struct mtlentry {
	char *name;
	struct rtmaterial mtl;
	char *texfile;
};
static struct mtlentry *mtllist;
static struct rtmaterial defmtl = {{1, 1, 1}, {0, 0, 0}, 60.0f, 0, 0};

static cgm_vec3 ambient;
static cgm_vec3 cur_col, cur_spec;
static float cur_shin, cur_refl;
static struct image *cur_tex;
static cgm_vec2 cur_uvscale;
static char *cur_name;

static char *dirname;

static union rtobject *load_object(struct rtscene *scn, struct ts_node *node);
static union rtobject *load_csg(struct rtscene *scn, struct ts_node *node, int op);
static int load_material(struct ts_node *node);
static struct image *get_texture(const char *fname);
static void print_tree_rec(union rtobject *tree, int lvl, const char *prefix);
static INLINE void normalize(cgm_vec3 *v);

void rt_init(struct rtscene *scn)
{
	scn->obj = darr_alloc(0, sizeof *scn->obj);
	scn->num_obj = 0;
	scn->lt = darr_alloc(0, sizeof *scn->lt);
	scn->num_lt = 0;
	scn->disobj = darr_alloc(0, sizeof *scn->disobj);

	cgm_vcons(&cur_col, 1, 1, 1);
	cgm_vcons(&cur_spec, 0, 0, 0);
	cur_shin = 1;
	cur_tex = 0;
	cur_uvscale.x = cur_uvscale.y = 1;
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
	darr_free(scn->disobj);

	memset(scn, 0, sizeof *scn);
}

int rt_load(struct rtscene *scn, const char *fname)
{
	static float defamb[] = {0.15, 0.15, 0.15};
	int i, len;
	struct ts_node *root, *c;
	float *vec;
	char *dirend;

	if((dirend = strrchr(fname, '/'))) {
		len = dirend - fname;
		dirname = alloca(len + 1);
		memcpy(dirname, fname, len);
		dirname[len] = 0;
	} else {
		dirname = 0;
	}

	if(!texlist) {
		texlist = darr_alloc(0, sizeof *texlist);
	}

	mtllist = darr_alloc(0, sizeof *mtllist);

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
		if(strcmp(c->name, "material") == 0) {
			load_material(c);
		} else {
			load_object(scn, c);
		}
		c = c->next;
	}

	for(i=0; i<darr_size(mtllist); i++) {
		free(mtllist[i].name);
		free(mtllist[i].texfile);
	}
	darr_free(mtllist);
	ts_free_tree(root);

	printf("rt_load(%s): %d objects (%d top-level), %d lights\n", fname,
			scn->num_obj + darr_size(scn->disobj), scn->num_obj, scn->num_lt);
	rt_print_tree(scn);

	return 0;
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

void rt_reflect(float refl)
{
	cur_refl = refl;
}

void rt_texmap(struct image *img)
{
	cur_tex = img;
}

void rt_uvscale(float su, float sv)
{
	cur_uvscale.x = su;
	cur_uvscale.y = sv;
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
	obj->x.mtl.refl = cur_refl;
	obj->x.mtl.tex = cur_tex;
	obj->x.mtl.uvscale = cur_uvscale;

	obj->x.mtl.krefl = obj->x.mtl.ks;
	cgm_vscale(&obj->x.mtl.krefl, obj->x.mtl.refl);

	darr_push(scn->obj, &obj);
	scn->num_obj = darr_size(scn->obj);
	return obj;
}

union rtobject *rt_find_object(struct rtscene *scn, const char *name)
{
	int i, sz;

	for(i=0; i<scn->num_obj; i++) {
		if(scn->obj[i]->x.name && strcmp(scn->obj[i]->x.name, name) == 0) {
			return scn->obj[i];
		}
	}

	sz = darr_size(scn->disobj);
	for(i=0; i<sz; i++) {
		if(scn->disobj[i]->x.name && strcmp(scn->disobj[i]->x.name, name) == 0) {
			return scn->disobj[i];
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

	last = darr_size(scn->disobj) - 1;
	for(i=0; i<darr_size(scn->disobj); i++) {
		if(scn->disobj[i] == obj) {
			if(i < last) {
				scn->disobj[i] = scn->disobj[last];
			}
			darr_pop(scn->disobj);
			return 0;
		}
	}
	return -1;
}

int rt_disable_object(struct rtscene *scn, union rtobject *obj)
{
	if(rt_remove_object(scn, obj) == -1) {
		return -1;
	}
	darr_push(scn->disobj, &obj);
	return 0;
}

union rtobject *rt_add_sphere(struct rtscene *scn, float x, float y, float z, float r)
{
	union rtobject *obj = add_object(scn, RT_SPH);
	cgm_vcons(&obj->s.p, x, y, z);
	obj->s.r = r;
	return obj;
}

union rtobject *rt_add_cylinder(struct rtscene *scn, float x, float y, float z, float r, float y0, float y1)
{
	union rtobject *obj = add_object(scn, RT_CYL);
	cgm_vcons(&obj->c.p, x, y, z);
	obj->c.r = r;
	obj->c.y0 = y0;
	obj->c.y1 = y1;
	return obj;
}

union rtobject *rt_add_plane(struct rtscene *scn, float nx, float ny, float nz, float d)
{
	union rtobject *obj = add_object(scn, RT_PLANE);
	cgm_vcons(&obj->p.n, nx, ny, nz);
	obj->p.d = d;
	obj->p.rad_sq = FLT_MAX;
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

	rt_disable_object(scn, a);
	rt_disable_object(scn, b);

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
	int i, tx, ty;
	uint16_t texel;
	float ndotl, vdotr, spec;
	cgm_ray ray;
	cgm_vec3 col, rdir;
	struct rtlight *lt;
	struct rtmaterial *mtl = &hit->obj->x.mtl;
	struct image *tex;

	col = mtl->kd;
	if(mtl->tex) {
		tex = mtl->tex;
		tx = cround64(hit->u * mtl->uvscale.x * (float)tex->width) & tex->xmask;
		ty = cround64(hit->v * mtl->uvscale.y * (float)tex->height) & tex->ymask;
		texel = tex->pixels[(ty << tex->xshift) + tx];
		col.x *= UNPACK_R16(texel) / 255.0f;
		col.y *= UNPACK_G16(texel) / 255.0f;
		col.z *= UNPACK_B16(texel) / 255.0f;
	}

	if(cgm_vdot(&hit->n, &hit->ray->dir) > 0.0f) {
		cgm_vneg(&hit->n);	/* faceforward */
	}

	ray.origin = hit->p;
	normalize(&hit->n);
	normalize(&hit->ray->dir);

	color->x = col.x * ambient.x;
	color->y = col.y * ambient.y;
	color->z = col.z * ambient.z;

	for(i=0; i<scn->num_lt; i++) {
		lt = scn->lt[i];
		ray.dir = lt->p;
		cgm_vsub(&ray.dir, &ray.origin);

		if(ray_scene(&ray, scn, 1.0f, 0)) continue;

		normalize(&ray.dir);
		ndotl = cgm_vdot(&ray.dir, &hit->n);
		if(ndotl < 0.0f) ndotl = 0.0f;

		rdir = hit->ray->dir;
		cgm_vreflect(&rdir, &hit->n);
		vdotr = cgm_vdot(&ray.dir, &rdir);
		if(vdotr < 0.0f) vdotr = 0.0f;
		spec = pow(vdotr, mtl->shin);

		color->x += (col.x * ndotl + mtl->ks.x * spec) * lt->color.x;
		color->y += (col.y * ndotl + mtl->ks.y * spec) * lt->color.y;
		color->z += (col.z * ndotl + mtl->ks.z * spec) * lt->color.z;
	}

	if(mtl->refl > 0 && lvl < RT_MAX_ITER) {
		ray.dir = hit->ray->dir;
		cgm_vreflect(&ray.dir, &hit->n);

		if(ray_trace(&ray, scn, lvl + 1, &col)) {
			color->x += col.x * mtl->krefl.x;
			color->y += col.y * mtl->krefl.y;
			color->z += col.z * mtl->krefl.z;
		}
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

void rt_print_tree(struct rtscene *scn)
{
	int i;
	for(i=0; i<scn->num_obj; i++) {
		print_tree_rec(scn->obj[i], 1, i == scn->num_obj - 1 ? "\\-" : "+-");
	}
}

static union rtobject *load_object(struct rtscene *scn, struct ts_node *node)
{
	static float zerovec[3], defnorm[] = {0, 1, 0}, defsize[] = {1, 1, 1};
	int i, num;
	float *kd, *ks, *uvs;
	union rtobject *obj = 0;
	struct ts_node *c;
	const char *mtlname, *texfile = 0;
	struct rtmaterial *mtl = &defmtl;

	rt_name(ts_get_attr_str(node, "name", 0));

	if((mtlname = ts_get_attr_str(node, "material", 0))) {
		num = darr_size(mtllist);
		for(i=0; i<num; i++) {
			if(strcmp(mtllist[i].name, mtlname) == 0) {
				mtl = &mtllist[i].mtl;
				texfile = mtllist[i].texfile;
				break;
			}
		}
	}

	kd = ts_get_attr_vec(node, "color", &mtl->kd.x);
	ks = ts_get_attr_vec(node, "specular", &mtl->ks.x);

	rt_color(kd[0], kd[1], kd[2]);
	rt_specular(ks[0], ks[1], ks[2]);
	rt_shininess(ts_get_attr_num(node, "shininess", mtl->shin));
	rt_reflect(ts_get_attr_num(node, "reflect", mtl->refl));

	if((texfile = ts_get_attr_str(node, "texmap", texfile))) {
		rt_texmap(get_texture(texfile));
	} else {
		rt_texmap(0);
	}
	uvs = ts_get_attr_vec(node, "uvscale", defsize);
	rt_uvscale(uvs[0], uvs[1]);


	if(strcmp(node->name, "sphere") == 0) {
		float rad = ts_get_attr_num(node, "r", 1.0f);
		float *pos = ts_get_attr_vec(node, "pos", zerovec);

		obj = rt_add_sphere(scn, pos[0], pos[1], pos[2], rad);

	} else if(strcmp(node->name, "cylinder") == 0) {
		float rad = ts_get_attr_num(node, "r", 1.0f);
		float *pos = ts_get_attr_vec(node, "pos", zerovec);
		float y0 = ts_get_attr_num(node, "y0", 0);
		float y1 = ts_get_attr_num(node, "y1", 1);

		obj = rt_add_cylinder(scn, pos[0], pos[1], pos[2], rad, y0, y1);

	} else if(strcmp(node->name, "plane") == 0) {
		float *n = ts_get_attr_vec(node, "n", defnorm);
		float d = ts_get_attr_num(node, "d", 0);
		float r = ts_get_attr_num(node, "rad", 0);

		obj = rt_add_plane(scn, n[0], n[1], n[2], d);
		if(r > 0.0f) {
			obj->p.rad_sq = r * r;
		}

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

	} else if(strcmp(node->name, "light") == 0) {
		float *pos = ts_get_attr_vec(node, "pos", zerovec);

		rt_add_light(scn, pos[0], pos[1], pos[2]);

	} else {
		fprintf(stderr, "raytrace: ignoring unknown node: %s\n", node->name);
	}

	return obj;
}

static union rtobject *load_csg(struct rtscene *scn, struct ts_node *node, int op)
{
	union rtobject *root = 0, *obj[2], *otmp;
	int num_obj = 0;
	struct ts_node *cn = node->child_list;
	const char *name;

	name = ts_get_attr_str(node, "name", 0);

	while(cn) {
		if(!(obj[num_obj] = load_object(scn, cn))) {
			cn = cn->next;
			continue;
		}
		if(++num_obj >= 2) {
			rt_name(0);
			if(!(otmp = rt_add_csg(scn, op, obj[0], obj[1]))) {
				demo_abort();
			}
			root = otmp;
			obj[0] = otmp;
			num_obj = 1;
		}
		cn = cn->next;
	}

	if(name) {
		root->csg.name = strdup_nf(name);
	}
	return root;
}

static int load_material(struct ts_node *node)
{
	const char *name, *texfname;
	struct mtlentry m;
	float *vec;

	if(!(name = ts_get_attr_str(node, "name", 0))) {
		return -1;
	}
	m.name = strdup_nf(name);
	m.mtl = defmtl;

	if((vec = ts_get_attr_vec(node, "color", 0))) {
		cgm_vcons(&m.mtl.kd, vec[0], vec[1], vec[2]);
	}
	if((vec = ts_get_attr_vec(node, "specular", 0))) {
		cgm_vcons(&m.mtl.ks, vec[0], vec[1], vec[2]);
	}
	m.mtl.shin = ts_get_attr_num(node, "shininess", defmtl.shin);
	m.mtl.refl = ts_get_attr_num(node, "reflect", defmtl.refl);

	if((texfname = ts_get_attr_str(node, "texmap", 0))) {
		m.texfile = strdup_nf(texfname);
	} else {
		m.texfile = 0;
	}

	if((vec = ts_get_attr_vec(node, "uvscale", 0))) {
		m.mtl.uvscale.x = vec[0];
		m.mtl.uvscale.y = vec[1];
	} else {
		m.mtl.uvscale.x = m.mtl.uvscale.y = 1;
	}

	m.mtl.krefl = m.mtl.ks;
	cgm_vscale(&m.mtl.krefl, m.mtl.refl);

	darr_push(mtllist, &m);
	return 0;
}

static struct image *get_texture(const char *fname)
{
	int i, num;
	struct texture tex;
	struct image img;
	char *path;

	if(dirname) {
		path = alloca(strlen(fname) + strlen(dirname) + 2);
		sprintf(path, "%s/%s", dirname, fname);
	} else {
		path = (char*)fname;
	}

	num = darr_size(texlist);
	for(i=0; i<num; i++) {
		if(strcmp(texlist[i].name, path) == 0) {
			return texlist[i].img;
		}
	}

	if(load_image(&img, path) == -1) {
		return 0;
	}

	tex.name = strdup_nf(path);
	tex.img = malloc_nf(sizeof *tex.img);
	*tex.img = img;
	darr_push(texlist, &tex);
	return tex.img;
}

static void ind(int lvl)
{
	int i;
	if(lvl > 35) lvl = 35;
	for(i=0; i<lvl; i++) {
		if(!i) {
			fputs("   ", stdout);
		} else {
			fputs("|  ", stdout);
		}
	}
}

static const char *typename(int type, int op)
{
	switch(type) {
	case RT_SPH: return "sphere";
	case RT_CYL: return "cylinder";
	case RT_PLANE: return "plane";
	case RT_BOX: return "box";
	case RT_CSG:
		switch(op) {
		case RT_UNION: return "union";
		case RT_ISECT: return "intersection";
		case RT_DIFF: return "difference";
		default: return "unknown-csg";
		}
	default:
		break;
	}
	return "unknown";
}

static void print_tree_rec(union rtobject *tree, int lvl, const char *prefix)
{
	if(!tree) return;

	ind(lvl);
	printf("%s %s", prefix, typename(tree->x.type, tree->csg.op));
	if(tree->x.name) {
		printf("(\"%s\")\n", tree->x.name);
	} else {
		printf("(%p)\n", (void*)tree);
	}

	if(tree->x.type == RT_CSG) {
		print_tree_rec(tree->csg.a, lvl + 1, "+-");
		print_tree_rec(tree->csg.b, lvl + 1, "\\-");
	}
}

static INLINE void normalize(cgm_vec3 *v)
{
	float s = rsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
	v->x *= s;
	v->y *= s;
	v->z *= s;
}
