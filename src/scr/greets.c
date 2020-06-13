#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "demo.h"
#include "3dgfx.h"
#include "screen.h"
#include "cfgopt.h"
#include "imago2.h"
#include "util.h"
#include "gfxutil.h"
#include "timer.h"
#include "smoketxt.h"

#ifdef MSDOS
#include "dos/gfx.h"	/* for wait_vsync assembly macro */
#else
void wait_vsync(void);
#endif

#define BLUR_RAD	5


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);


static struct screen scr = {
	"greets",
	init,
	destroy,
	start, 0,
	draw
};

static long start_time;

static struct smktxt *stx;

static uint16_t *cur_smokebuf, *prev_smokebuf;
static int smokebuf_size;
#define smokebuf_start	(cur_smokebuf < prev_smokebuf ? cur_smokebuf : prev_smokebuf)
#define swap_smoke_buffers() \
	do { \
		uint16_t *tmp = cur_smokebuf; \
		cur_smokebuf = prev_smokebuf; \
		prev_smokebuf = tmp; \
	} while(0)

static float cam_theta, cam_phi = 25;
static float cam_dist = 3;

struct screen *greets_screen(void)
{
	return &scr;
}


static int init(void)
{
	if(!(stx = create_smktxt("data/greets1.png", "data/vfield1"))) {
		return -1;
	}

	smokebuf_size = FB_WIDTH * FB_HEIGHT * sizeof *cur_smokebuf;
	if(!(cur_smokebuf = malloc(smokebuf_size * 2))) {
		perror("failed to allocate smoke framebuffer");
		return -1;
	}
	prev_smokebuf = cur_smokebuf + FB_WIDTH * FB_HEIGHT;

	return 0;
}

static void destroy(void)
{
	free(smokebuf_start);
	destroy_smktxt(stx);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	memset(smokebuf_start, 0, smokebuf_size * 2);

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

	update_smktxt(stx, dt);
}

static void draw(void)
{
	int i, j;
	uint16_t *dest, *src;
	unsigned long msec;
	static unsigned long last_swap;

	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	memcpy(cur_smokebuf, prev_smokebuf, smokebuf_size);

	g3d_framebuffer(FB_WIDTH, FB_HEIGHT, cur_smokebuf);
	draw_smktxt(stx);
	g3d_framebuffer(FB_WIDTH, FB_HEIGHT, fb_pixels);

	memcpy(fb_pixels, cur_smokebuf, smokebuf_size);

	/*perf_start();*/
	blur_horiz(prev_smokebuf, cur_smokebuf, FB_WIDTH, FB_HEIGHT, BLUR_RAD, 240);
	/*
	perf_end();
	printf("blur perf: %lu\n", (unsigned long)perf_interval_count);
	*/
	blur_vert(cur_smokebuf, prev_smokebuf, FB_WIDTH, FB_HEIGHT, BLUR_RAD, 240);
	swap_smoke_buffers();

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
