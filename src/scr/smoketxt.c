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
#include "cgmath/cgmath.h"

/* if defined, use bilinear interpolation for dispersion field vectors */
#undef BILERP_FIELD
/* if defined randomize field vectors by RAND_FIELD_MAX */
#define RANDOMIZE_FIELD

#define RAND_FIELD_MAX	((int32_t)(0.7 * 65536.0))

#define DFL_PCOUNT		4000
#define DFL_MAX_LIFE	(7 << 16)
#define DFL_PALPHA		1.0f
#define DFL_ZBIAS		0.25
#define DFL_DRAG		((int32_t)(0.95 * 65536.0))
#define DFL_FORCE		((int32_t)(0.07 * 65536.0))
#define DFL_FREQ		0.085


static int init_emitter(struct emitter *em, int num, unsigned char *map, int xsz, int ysz);
static int load_vfield(struct vfield *vf, const char *fname);
static void vfield_eval(struct vfield *vf, int32_t x, int32_t y, struct ivec2 *dir);

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
#if 0
	int i, j;
	unsigned int tmp;
	struct ivec2 *vptr;
	struct vfield *vf = &stx->vfield;

	if(!(vf->v = malloc(xres * yres * sizeof *vf->v))) {
		fprintf(stderr, "failed to allocate %dx%d vector field\n", xres, yres);
		return -1;
	}

	vf->width = xres;
	vf->height = yres;
	vf->pos.x = vf->pos.y = 0;
	vf->size.x = vf->size.y = 65536;

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

#endif
	return 0;
}

int dump_smktxt_vfield(struct smktxt *stx, const char *fname)
{
#if 0
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
#endif
	return 0;
}

void set_smktxt_wind(struct smktxt *stx, int32_t x, int32_t y)
{
	stx->em.wind.x = x;
	stx->em.wind.y = y;
}

void set_smktxt_plife(struct smktxt *stx, int32_t life)
{
	stx->em.max_life = life;
}

void set_smktxt_pcount(struct smktxt *stx, int count)
{
	free(stx->em.plist);
	stx->em.plist = 0;
	stx->em.pcount = count;
}

void set_smktxt_drag(struct smktxt *stx, int32_t drag)
{
	stx->em.drag = drag;
}

#define DT	(65536 / 30)
void update_smktxt(struct smktxt *stx)
{
	int i;
	struct ivec2 accel;
	struct particle *p;
	struct ivec2 *v;

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
		p->x += ((p->vx >> 4) * (stx->em.drag >> 4)) >> 12;
		p->y += ((p->vy >> 4) * (stx->em.drag >> 4)) >> 12;
		p->vx += ((accel.x >> 4) * (DFL_FORCE >> 4)) >> 12;
		p->vy += ((accel.y >> 4) * (DFL_FORCE >> 4)) >> 12;
		p->life -= DT;
		if(p->life < 0) p->life = 0;

		v->x = p->x;
		v->y = p->y;
		++v;

		++p;
	}
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

		p->x = (int32_t)(((float)x / (float)xsz - 0.5) * 65536.0);
		p->y = (int32_t)((-(float)y / (float)xsz + 0.5 / aspect) * 65536.0);
		p->r = p->g = p->b = 255;
		p->vx = p->vy = 0.0f;
		p->life = em->max_life;
		++p;
	}
	return 0;
}

static int load_vfield(struct vfield *vf, const char *fname)
{
	FILE *fp;
	int i, j, tmp;
	cgm_vec2 *vflt, *vsrc;
	struct ivec2 *vdst;

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
#ifdef BUILD_BIGENDIAN
	vf->width = BSWAP32(vf->width);
	vf->height = BSWAP32(vf->height);
#endif

	/* assume xsz is pow2 otherwise fuck you */
	tmp = vf->width - 1;
	vf->xshift = 0;
	while(tmp) {
		++vf->xshift;
		tmp >>= 1;
	}

	if(!(vflt = malloc(vf->width * vf->height * sizeof *vflt))) {
		fprintf(stderr, "failed to allocate temp %dx%d vector field\n", vf->width, vf->height);
		fclose(fp);
		return -1;
	}
	if(fread(vflt, sizeof *vflt, vf->width * vf->height, fp) < vf->width * vf->height) {
		fprintf(stderr, "load_vfield: unexpected end of file while reading %dx%d vector field\n",
				vf->width, vf->height);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	if(!(vf->v = malloc(vf->width * vf->height * sizeof *vf->v))) {
		fprintf(stderr, "failed to allocate %dx%d vector field\n", vf->width, vf->height);
		return -1;
	}

	vsrc = vflt;
	vdst = vf->v;
	for(i=0; i<vf->height; i++) {
		for(j=0; j<vf->width; j++) {
#ifdef BUILD_BIGENDIAN
			uint32_t intval = *(uint32_t*)&vsrc->x;
			intval = BSWAP32(intval);
			vsrc->x = *(float*)&intval;
			intval = *(uint32_t*)&vsrc->y;
			intval = BSWAP32(intval);
			vsrc->y = *(float*)&intval;
#endif
			vdst->x = (int32_t)(vsrc->x * 65536.0f);
			vdst->y = (int32_t)(vsrc->y * 65536.0f);
			vsrc++;
			vdst++;
		}
	}
	free(vflt);

	vf->pos.x = vf->pos.y = 0;
	vf->size.x = vf->size.y = 65536;
	return 0;
}

static void vfield_eval(struct vfield *vf, int32_t x, int32_t y, struct ivec2 *dir)
{
	int32_t px, py;
	int32_t tx, ty;
	struct ivec2 *p1, *p2;
	struct ivec2 left, right;

	/* after this, x/y is left in 24.8 */
	x = ((x - vf->pos.x) / (vf->size.x >> 8) + 128) * vf->width;
	y = ((y - vf->pos.y) / (vf->size.y >> 8) + 128) * vf->height;
	/* floor */
	x &= 0xffffff00;
	y &= 0xffffff00;

	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x > (vf->width - 2) << 8) x = (vf->width - 2) << 8;
	if(y > (vf->height - 2) << 8) y = (vf->height - 2) << 8;

	px = x >> 8;
	py = y >> 8;

	p1 = vf->v + (py << vf->xshift) + px;
#if 0
	p2 = p1 + vf->width;

	/* XXX wtf? x = floor(x) then fmod(x, 1.0f) makes no sense */
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
	dir->x += rand() % RAND_FIELD_MAX - 32768;
	dir->y += rand() % RAND_FIELD_MAX - 32768;
#endif
}
