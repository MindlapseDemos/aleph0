#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "demo.h"
#include "3dgfx.h"
#include "screen.h"
#include "cfgopt.h"
#include "imago2.h"
#include "util.h"

#define PCOUNT		4000
#define MAX_LIFE	6.0f
#define PALPHA		1.0f
#define ZBIAS		0.25
#define DRAG		0.95
#define FORCE		0.07
#define FREQ		0.085
static float wind[] = {-0.0, 0.0, 0.01};


struct vec2 {
	float x, y;
};

struct vec3 {
	float x, y, z;
};

struct particle {
	float x, y, z;
	float vx, vy, vz;	/* velocity */
	int r, g, b;
	float life;
};

struct emitter {
	struct particle *plist;
	int pcount;
};

struct vfield {
	struct vec2 pos, size;

	int width, height;
	int xshift;
	struct vec2 *v;
};


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

int init_emitter(struct emitter *em, int num, unsigned char *map, int xsz, int ysz);
void update_particles(struct emitter *em, float dt);
void draw_particles(struct emitter *em);

int init_vfield_load(struct vfield *vf, const char *fname);
void vfield_eval(struct vfield *vf, float x, float y, struct vec2 *dir);


static struct screen scr = {
	"greets",
	init,
	destroy,
	start, 0,
	draw
};

static struct emitter em;
static struct vfield vfield;
static struct g3d_vertex *varr;
static long start_time;

static float cam_theta, cam_phi = 25;
static float cam_dist = 3;

struct screen *greets_screen(void)
{
	return &scr;
}


static int init(void)
{
	int xsz, ysz;
	unsigned char *pixels;

	if(!(pixels = img_load_pixels("data/greets1.png", &xsz, &ysz, IMG_FMT_GREY8))) {
		fprintf(stderr, "failed to load particle spawn map\n");
		return -1;
	}

	init_emitter(&em, PCOUNT, pixels, xsz, ysz);
	img_free_pixels(pixels);

	if(!(varr = malloc(PCOUNT * sizeof *varr))) {
		perror("failed to allocate particle vertex buffer\n");
		return -1;
	}

	if(init_vfield_load(&vfield, "data/vfield1") == -1) {
		return -1;
	}

	return 0;
}

static void destroy(void)
{
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	start_time = time_msec;
}

static void update(void)
{
	static long prev_msec;
	static int prev_mx, prev_my;
	static unsigned int prev_bmask;

	long msec = time_msec - start_time;
	float dt = (msec - prev_msec) / 1000.0f;
	prev_msec = msec;

	if(mouse_bmask) {
		if((mouse_bmask ^ prev_bmask) == 0) {
			int dx = mouse_x - prev_mx;
			int dy = mouse_y - prev_my;

			if(dx || dy) {
				if(mouse_bmask & 1) {
					cam_theta += dx * 1.0;
					cam_phi += dy * 1.0;

					if(cam_phi < -90) cam_phi = -90;
					if(cam_phi > 90) cam_phi = 90;
				}
				if(mouse_bmask & 4) {
					cam_dist += dy * 0.5;

					if(cam_dist < 0) cam_dist = 0;
				}
			}
		}
	}
	prev_mx = mouse_x;
	prev_my = mouse_y;
	prev_bmask = mouse_bmask;

	update_particles(&em, dt);
}

static void draw(void)
{
	update();

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	} else {
		g3d_rotate(cam_phi, 1, 0, 0);
		g3d_rotate(cam_theta, 0, 1, 0);
	}

	draw_particles(&em);

	swap_buffers(fb_pixels);
}



int init_emitter(struct emitter *em, int num, unsigned char *map, int xsz, int ysz)
{
	int i, x, y;
	float aspect = (float)xsz / (float)ysz;
	struct particle *p;

	if(!(em->plist = malloc(num * sizeof *em->plist))) {
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
		p->r = 0;
		p->g = 0x1f;
		p->b = 255;
		p->vx = p->vy = p->vz = 0.0f;
		p->life = MAX_LIFE;
		++p;
	}
	return 0;
}

void update_particles(struct emitter *em, float dt)
{
	int i;
	struct vec2 accel;
	struct particle *p = em->plist;
	struct g3d_vertex *v = varr;

	for(i=0; i<em->pcount; i++) {
		vfield_eval(&vfield, p->x, p->y, &accel);
		p->x += p->vx * DRAG * dt;
		p->y += p->vy * DRAG * dt;
		p->z += p->vz * DRAG * dt;
		p->vx += (wind[0] + accel.x * FORCE) * dt;
		p->vy += (wind[1] + accel.y * FORCE) * dt;
		p->vz += (wind[2] + p->z * ZBIAS) * dt;
		p->life -= dt;
		if(p->life < 0.0f) p->life = 0.0f;

		v->x = p->x;
		v->y = p->y;
		v->z = p->z;
		v->w = 1.0f;
		v->r = p->r;
		v->g = p->g;
		v->b = p->b;
		v->a = cround64(p->life * 255.0 / MAX_LIFE);
		++v;

		++p;
	}
}

void draw_particles(struct emitter *em)
{
	g3d_enable(G3D_BLEND);
	g3d_draw(G3D_POINTS, varr, PCOUNT);
	g3d_disable(G3D_BLEND);
}


int init_vfield_load(struct vfield *vf, const char *fname)
{
	FILE *fp;
	int tmp;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open vector field: %s\n", fname);
		return -1;
	}
	if(fread(&vf->width, sizeof vf->width, 1, fp) < 1 ||
			fread(&vf->height, sizeof vf->height, 1, fp) < 1) {
		fprintf(stderr, "init_vfield_load: unexpected end of file while reading header\n");
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
		fprintf(stderr, "init_vfield_load: unexpected end of file while reading %dx%d vector field\n",
				vf->width, vf->height);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	vf->pos.x = vf->pos.y = 0;
	vf->size.x = vf->size.y = 1;
	return 0;
}

void vfield_eval(struct vfield *vf, float x, float y, struct vec2 *dir)
{
	int px, py;
	float tx, ty;
	struct vec2 *p1, *p2;
	struct vec2 left, right;

	x = ((x - vf->pos.x) / vf->size.x + 0.5f) * vf->width;
	y = ((y - vf->pos.y) / vf->size.y + 0.5f) * vf->height;
	x = floor(x);
	y = floor(y);

	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x > vf->width - 2) x = vf->width - 2;
	if(y > vf->height - 2) y = vf->height - 2;

	tx = fmod(x, 1.0f);
	ty = fmod(y, 1.0f);

	px = (int)x;
	py = (int)y;

	p1 = vf->v + (py << vf->xshift) + px;
	p2 = p1 + vf->width;

	left.x = p1->x + (p2->x - p1->x) * ty;
	left.y = p1->y + (p2->y - p1->y) * ty;
	++p1;
	++p2;
	right.x = p1->x + (p2->x - p1->x) * ty;
	right.y = p1->y + (p2->y - p1->y) * ty;

	dir->x = left.x + (right.x - left.x) * tx;
	dir->y = left.y + (right.y - left.y) * ty;

	dir->x += ((float)rand() / RAND_MAX - 0.5) * 0.7;
	dir->y += ((float)rand() / RAND_MAX - 0.5) * 0.7;
}
