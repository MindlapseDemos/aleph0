#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "demo.h"
#include "3dgfx.h"
#include "vmath.h"
#include "screen.h"
#include "util.h"
#include "cfgopt.h"
#include "mesh.h"


#ifdef MSDOS
#include "dos/gfx.h"	/* for wait_vsync assembly macro */
#else
void wait_vsync(void);
#endif

#define NSPAWNPOS	64
#define SPAWN_RATE	16.0f
/*#define GRAV		(-6.0f)*/
#define GRAV		0.0f
#define HAIR_LENGTH 20

struct particle {
	vec3_t pos;
	vec3_t vel;
	int r, g, b, a;
	int life;

	struct particle *next;
};

struct hairball {
	vec3_t pos;
	quat_t rot;
	float xform[16];

	struct particle *plist[NSPAWNPOS];
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void update_hairball(struct hairball *hb, float dt);
static void draw_hairball(struct hairball *hb);

static struct particle *palloc(void);
void pfree(struct particle *p);


static struct screen scr = {
	"hairball",
	init,
	destroy,
	start, 0,
	draw
};

static long start_time;

static float cam_theta, cam_phi = 15;
static float cam_dist = 8;

static vec3_t *spawnpos, *spawndir;
static struct hairball hball;
static struct g3d_mesh sphmesh;

struct screen *hairball_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, numpt = 0;

	gen_sphere_mesh(&sphmesh, 1.0f, 12, 6);

	hball.xform[0] = hball.xform[5] = hball.xform[10] = hball.xform[15] = 1.0f;
	hball.rot.w = 1.0f;

	if(!(spawnpos = malloc(NSPAWNPOS * sizeof *spawnpos)) ||
			!(spawndir = malloc(NSPAWNPOS * sizeof *spawndir))) {
		fprintf(stderr, "failed to allocate spawnpos/dir array\n");
		return -1;
	}

	while(numpt < NSPAWNPOS) {
		float best_dist = -1.0f;
		for(i=0; i<100; i++) {
			vec3_t pos = sphrand(1.0f);

			float mindist = FLT_MAX;
			for(j=0; j<numpt; j++) {
				float dx = pos.x - spawnpos[j].x;
				float dy = pos.y - spawnpos[j].y;
				float dz = pos.z - spawnpos[j].z;
				float dsq = dx * dx + dy * dy + dz * dz;
				if(dsq < mindist) {
					mindist = dsq;
				}
			}

			if(mindist > best_dist) {
				spawnpos[numpt] = pos;
				best_dist = mindist;
			}
		}

		spawndir[numpt] = spawnpos[numpt];
		++numpt;
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
	g3d_perspective(50.0, 1.33333, 0.5, 100.0);

	start_time = time_msec;
}

static void update(void)
{
	static float prev_mx, prev_my;
	static long prev_msec;
	int mouse_dx, mouse_dy;
	long msec = time_msec - start_time;
	float dt = (msec - prev_msec) / 1000.0f;
	prev_msec = msec;

	mouse_dx = mouse_x - prev_mx;
	mouse_dy = mouse_y - prev_my;
	prev_mx = mouse_x;
	prev_my = mouse_y;


	if(opt.sball) {
		memcpy(hball.xform, sball_matrix, 16 * sizeof(float));

		hball.pos.x = hball.xform[12];
		hball.pos.y = hball.xform[13];
		hball.pos.z = hball.xform[14];
	} else {
		if(mouse_bmask & MOUSE_BN_MIDDLE) {
			hball.pos.x += mouse_dx * 0.05;
			hball.pos.y -= mouse_dy * 0.05;
		}

		quat_to_mat(hball.xform, hball.rot);
		hball.xform[12] = hball.pos.x;
		hball.xform[13] = hball.pos.y;
		hball.xform[14] = hball.pos.z;
	}

	update_hairball(&hball, dt);

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);
}

static void draw(void)
{
	unsigned long msec;
	static unsigned long last_swap;

	update();

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);

	draw_hairball(&hball);

	msec = get_msec();
	if(msec - last_swap < 16) {
		wait_vsync();
	}
	if(!opt.vsync) {
		wait_vsync();
	}
	swap_buffers(fb_pixels);
	last_swap = get_msec();
}

static void update_hairball(struct hairball *hb, float dt)
{
	int i, j, spawn_count;
	struct particle pdummy, *prev, *p;
	static float spawn_acc;

	/* update particles */
	for(i=0; i<NSPAWNPOS; i++) {
		pdummy.next = hb->plist[i];
		prev = &pdummy;
		while(prev && prev->next) {
			p = prev->next;
			if(--p->life <= 0) {
				prev->next = p->next;
				pfree(p);
				prev = prev->next;
				continue;
			}

			p->pos.x += p->vel.x * dt;
			p->pos.y += p->vel.y * dt;
			p->pos.z += p->vel.z * dt;
			p->vel.y += GRAV * dt;

			p->r = p->g = p->b = (p->life << 8) / HAIR_LENGTH - 1;

			prev = p;
		}

		hb->plist[i] = pdummy.next;
	}

	/* spawn new particles */
	spawn_acc += dt;
	if(spawn_acc >= (1.0f / SPAWN_RATE)) {
		for(i=0; i<NSPAWNPOS; i++) {
			struct particle *p = palloc();
			float *mat = hb->xform;
			vec3_t pos = spawnpos[i];
			vec3_t dir = spawndir[i];

			p->pos.x = mat[0] * pos.x + mat[4] * pos.y + mat[8] * pos.z + mat[12];
			p->pos.y = mat[1] * pos.x + mat[5] * pos.y + mat[9] * pos.z + mat[13];
			p->pos.z = mat[2] * pos.x + mat[6] * pos.y + mat[10] * pos.z + mat[14];

			p->vel.x = mat[0] * dir.x + mat[4] * dir.y + mat[8] * dir.z;
			p->vel.y = mat[1] * dir.x + mat[5] * dir.y + mat[9] * dir.z;
			p->vel.z = mat[2] * dir.x + mat[6] * dir.y + mat[10] * dir.z;
			p->life = HAIR_LENGTH;

			p->next = hb->plist[i];
			hb->plist[i] = p;
		}
		spawn_acc -= 1.0f / SPAWN_RATE;
	}
}

static void draw_hairball(struct hairball *hb)
{
	int i, col;
	struct particle *p;
	vec3_t prevpos;
	vec3_t start[NSPAWNPOS];

	for(i=0; i<NSPAWNPOS; i++) {
		start[i].x = hb->xform[0] * spawnpos[i].x + hb->xform[4] * spawnpos[i].y + hb->xform[8] * spawnpos[i].z + hb->xform[12];
		start[i].y = hb->xform[1] * spawnpos[i].x + hb->xform[5] * spawnpos[i].y + hb->xform[9] * spawnpos[i].z + hb->xform[13];
		start[i].z = hb->xform[2] * spawnpos[i].x + hb->xform[6] * spawnpos[i].y + hb->xform[10] * spawnpos[i].z + hb->xform[14];
	}

	g3d_begin(G3D_LINES);
	for(i=0; i<NSPAWNPOS; i++) {
		if(start[i].z >= hb->pos.z) continue;
		p = hb->plist[i];
		prevpos = start[i];
		while(p) {
			g3d_color3b(p->r, p->g, p->b);
			g3d_vertex(prevpos.x, prevpos.y, prevpos.z);
			g3d_vertex(p->pos.x, p->pos.y, p->pos.z);
			prevpos = p->pos;
			p = p->next;
		}
	}
	g3d_end();


	g3d_push_matrix();
	g3d_mult_matrix(hb->xform);
	zsort_mesh(&sphmesh);
	draw_mesh(&sphmesh);
	g3d_pop_matrix();

	g3d_begin(G3D_LINES);
	for(i=0; i<NSPAWNPOS; i++) {
		if(start[i].z < hb->pos.z) continue;
		p = hb->plist[i];
		prevpos = start[i];
		while(p) {
			g3d_color3b(p->r, p->g, p->b);
			g3d_vertex(prevpos.x, prevpos.y, prevpos.z);
			g3d_vertex(p->pos.x, p->pos.y, p->pos.z);
			prevpos = p->pos;
			p = p->next;
		}
	}
	g3d_end();

}


static struct particle *palloc(void)
{
	return malloc(sizeof(struct particle));
}

void pfree(struct particle *p)
{
	free(p);
}
