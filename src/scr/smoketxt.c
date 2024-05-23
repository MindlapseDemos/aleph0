#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "imago2.h"
#include "smoketxt.h"
#include "3dgfx.h"
#include "util.h"
#include "noise.h"

/* if defined, use bilinear interpolation for dispersion field vectors */
#define BILERP_FIELD
/* if defined randomize field vectors by RAND_FIELD_MAX */
#define RANDOMIZE_FIELD

#define RAND_FIELD_MAX	0.7

#define DFL_PCOUNT		4000
#define DFL_MAX_LIFE	7.0f
#define DFL_PALPHA		1.0f
#define DFL_ZBIAS		0.25
#define DFL_DRAG		0.95
#define DFL_FORCE		0.07
#define DFL_FREQ		0.085


static int init_emitter(struct emitter *em, int num, unsigned char *map, int xsz, int ysz);
static int load_vfield(struct vfield *vf, const char *fname);
static void vfield_eval(struct vfield *vf, float x, float y, cgm_vec2 *dir);

struct smktxt *create_smktxt(const char *imgname, const char *vfieldname)
{
	struct smktxt *stx;

	if(!imgname) return 0;

	if(!(stx = calloc(sizeof *stx, 1))) {
		return 0;
	}
	if(!(stx->img_pixels = img_load_pixels(imgname, &stx->img_xsz, &stx->img_ysz, IMG_FMT_GREY8))) {
		fprintf(stderr, "create_smktxt: failed to load particle spawnmap: %s\n", imgname);
		free(stx);
		return 0;
	}

	stx->em.pcount = DFL_PCOUNT;
	stx->em.wind.z = 0.01;
	stx->em.drag = DFL_DRAG;
	stx->em.max_life = DFL_MAX_LIFE;

	if(vfieldname) {
		if(load_vfield(&stx->vfield, vfieldname) == -1) {
			img_free_pixels(stx->img_pixels);
			free(stx);
			return 0;
		}
	}

	return stx;
}

void destroy_smktxt(struct smktxt *stx)
{
	if(!stx) return;

	img_free_pixels(stx->img_pixels);
	free(stx);
}

int gen_smktxt_vfield(struct smktxt *stx, int xres, int yres, float xfreq, float yfreq)
{
	int i, j;
	unsigned int tmp;
	cgm_vec2 *vptr;
	struct vfield *vf = &stx->vfield;

	if(!(vf->v = malloc(xres * yres * sizeof *vf->v))) {
		fprintf(stderr, "failed to allocate %dx%d vector field\n", xres, yres);
		return -1;
	}

	vf->width = xres;
	vf->height = yres;
	vf->pos.x = vf->pos.y = 0.0f;
	vf->size.x = vf->size.y = 1.0f;

	/* assume xres is pow2 otherwise fuck you */
	tmp = xres - 1;
	vf->xshift = 0;
	while(tmp) {
		++vf->xshift;
		tmp >>= 1;
	}

	vptr = vf->v;
	for(i=0; i<yres; i++) {
		for(j=0; j<xres; j++) {
			float x = noise3(j * xfreq, i * yfreq, 0.5);
			float y = noise3(j * xfreq, i * yfreq, 10.5);
			float len = sqrt(x * x + y * y);
			if(len == 0.0f) len = 1.0f;

			vptr->x = x / len;
			vptr->y = y / len;

			++vptr;
		}
	}
	return 0;
}

int dump_smktxt_vfield(struct smktxt *stx, const char *fname)
{
	FILE *fp;
	int xsz, ysz;

	if(!(fp = fopen(fname, "wb"))) {
		fprintf(stderr, "failed to open %s for writing: %s\n", fname, strerror(errno));
		return -1;
	}

	xsz = stx->vfield.width;
	ysz = stx->vfield.height;

	fwrite(&xsz, sizeof xsz, 1, fp);
	fwrite(&ysz, sizeof ysz, 1, fp);
	fwrite(stx->vfield.v, sizeof *stx->vfield.v, xsz * ysz, fp);
	fclose(fp);
	return 0;
}

void set_smktxt_wind(struct smktxt *stx, float x, float y, float z)
{
	stx->em.wind.x = x;
	stx->em.wind.y = y;
	stx->em.wind.z = z;
}

void set_smktxt_plife(struct smktxt *stx, float life)
{
	stx->em.max_life = life;
}

void set_smktxt_pcount(struct smktxt *stx, int count)
{
	free(stx->em.plist);
	stx->em.plist = 0;
	stx->em.pcount = count;
}

void set_smktxt_drag(struct smktxt *stx, float drag)
{
	stx->em.drag = drag;
}

#define DT	(1.0f / 30.0f)
void update_smktxt(struct smktxt *stx)
{
	int i;
	cgm_vec2 accel;
	struct particle *p;
	struct g3d_vertex *v;

	if(!stx->em.plist) {
		if(init_emitter(&stx->em, stx->em.pcount, stx->img_pixels, stx->img_xsz, stx->img_ysz) == -1) {
			fprintf(stderr, "failed to initialize emitter with %d particles\n", stx->em.pcount);
			return;
		}
	}

	p = stx->em.plist;
	v = stx->em.varr;

	for(i=0; i<stx->em.pcount; i++) {
		vfield_eval(&stx->vfield, p->x, p->y, &accel);
		p->x += p->vx * stx->em.drag * DT;
		p->y += p->vy * stx->em.drag * DT;
		p->z += p->vz * stx->em.drag * DT;
		p->vx += (stx->em.wind.x + accel.x * DFL_FORCE) * DT;
		p->vy += (stx->em.wind.y + accel.y * DFL_FORCE) * DT;
		p->vz += (stx->em.wind.z + p->z * DFL_ZBIAS) * DT;
		p->life -= DT;
		if(p->life < 0.0f) p->life = 0.0f;

		v->x = p->x;
		v->y = p->y;
		v->z = p->z;
		v->w = 1.0f;
		v->a = cround64(p->life * 255.0f / stx->em.max_life);
		v->r = 0;
		v->g = (v->a & 0xe0) >> 3;
		v->b = (v->a & 0x1f) << 3;
		++v;

		++p;
	}
}

void draw_smktxt(struct smktxt *stx)
{
	g3d_draw(G3D_POINTS, stx->em.varr, stx->em.pcount);
}


static int init_emitter(struct emitter *em, int num, unsigned char *map, int xsz, int ysz)
{
	int i, x, y;
	float aspect = (float)xsz / (float)ysz;
	struct particle *p;

	free(em->varr);
	if(!(em->varr = malloc(num * sizeof *em->varr))) {
		fprintf(stderr, "failed to allocate particle vertex array (%d verts)\n", num);
		return -1;
	}

	free(em->plist);
	if(!(em->plist = malloc(num * sizeof *em->plist))) {
		free(em->varr);
		return -1;
	}
	em->pcount = num;

	p = em->plist;
	for(i=0; i<num; i++) {
		do {
			x = rand() % xsz;
			y = rand() % ysz;
		} while(map[y * xsz + x] < 128);

		p->x = (float)x / (float)xsz - 0.5;
		p->y = -(float)y / (float)xsz + 0.5 / aspect;
		p->z = ((float)i / (float)num * 2.0 - 1.0) * 0.005;
		p->r = p->g = p->b = 255;
		p->vx = p->vy = p->vz = 0.0f;
		p->life = em->max_life;
		++p;
	}
	return 0;
}

static int load_vfield(struct vfield *vf, const char *fname)
{
	FILE *fp;
	int tmp;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open vector field: %s\n", fname);
		return -1;
	}
	if(fread(&vf->width, sizeof vf->width, 1, fp) < 1 ||
			fread(&vf->height, sizeof vf->height, 1, fp) < 1) {
		fprintf(stderr, "load_vfield: unexpected end of file while reading header\n");
		fclose(fp);
		return -1;
	}

	/* assume xsz is pow2 otherwise fuck you */
	tmp = vf->width - 1;
	vf->xshift = 0;
	while(tmp) {
		++vf->xshift;
		tmp >>= 1;
	}

	if(!(vf->v = malloc(vf->width * vf->height * sizeof *vf->v))) {
		fprintf(stderr, "failed to allocate %dx%d vector field\n", vf->width, vf->height);
		fclose(fp);
		return -1;
	}
	if(fread(vf->v, sizeof *vf->v, vf->width * vf->height, fp) < vf->width * vf->height) {
		fprintf(stderr, "load_vfield: unexpected end of file while reading %dx%d vector field\n",
				vf->width, vf->height);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	vf->pos.x = vf->pos.y = 0;
	vf->size.x = vf->size.y = 1;
	return 0;
}

static void vfield_eval(struct vfield *vf, float x, float y, cgm_vec2 *dir)
{
	int px, py;
	float tx, ty;
	cgm_vec2 *p1, *p2;
	cgm_vec2 left, right;

	x = ((x - vf->pos.x) / vf->size.x + 0.5f) * vf->width;
	y = ((y - vf->pos.y) / vf->size.y + 0.5f) * vf->height;
	x = floor(x);
	y = floor(y);

	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x > vf->width - 2) x = vf->width - 2;
	if(y > vf->height - 2) y = vf->height - 2;

	px = (int)x;
	py = (int)y;

	p1 = vf->v + (py << vf->xshift) + px;
#ifdef BILERP_FIELD
	p2 = p1 + vf->width;

	tx = fmod(x, 1.0f);
	ty = fmod(y, 1.0f);

	left.x = p1->x + (p2->x - p1->x) * ty;
	left.y = p1->y + (p2->y - p1->y) * ty;
	++p1;
	++p2;
	right.x = p1->x + (p2->x - p1->x) * ty;
	right.y = p1->y + (p2->y - p1->y) * ty;

	dir->x = left.x + (right.x - left.x) * tx;
	dir->y = left.y + (right.y - left.y) * ty;
#else
	dir->x = p1->x;
	dir->y = p1->y;
#endif

#ifdef RANDOMIZE_FIELD
	dir->x += ((float)rand() / (float)RAND_MAX - 0.5) * RAND_FIELD_MAX;
	dir->y += ((float)rand() / (float)RAND_MAX - 0.5) * RAND_FIELD_MAX;
#endif
}
