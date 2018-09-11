#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <conio.h>
#include "demo.h"
#include "keyb.h"
#include "mouse.h"
#include "timer.h"
#include "gfx.h"
#include "vmath.h"
#include "sball.h"
#include "cfgopt.h"
#include "logger.h"
#include "tinyfps.h"

#undef NOKEYB

static int handle_sball_event(sball_event *ev);
static void recalc_sball_matrix(float *xform);

static int quit;
static int use_mouse;
static long fbsize;

static int use_sball;
static vec3_t pos = {0, 0, 0};
static quat_t rot = {0, 0, 0, 1};

int main(int argc, char **argv)
{
	fbsize = fb_width * fb_height * fb_bpp / CHAR_BIT;

	init_logger("demo.log");

	init_timer(100);
#ifndef NOKEYB
	kb_init(32);
#endif

	if((use_mouse = have_mouse())) {
		printf("initializing mouse input\n");
		set_mouse_limits(0, 0, fb_width, fb_height);
		set_mouse(fb_width / 2, fb_height / 2);
	}

	if(!(fb_pixels = malloc(fbsize))) {
		fprintf(stderr, "failed to allocate backbuffer\n");
		return 1;
	}

	if(!(vmem_back = set_video_mode(fb_width, fb_height, fb_bpp))) {
		return 1;
	}
	if(!(vmem_front = page_flip(FLIP_NOW))) {
		fprintf(stderr, "page flipping not supported. falling back to double buffering\n");
		vmem_front = vmem_back;
	} else {
		assert(vmem_back != vmem_front);
	}

	if(demo_init(argc, argv) == -1) {
		set_text_mode();
		return 1;
	}

	if(opt.sball && sball_init() == 0) {
		use_sball = 1;
	}

	reset_timer();

	while(!quit) {
#ifndef NOKEYB
		int key;
		while((key = kb_getkey()) != -1) {
			demo_keyboard(key, 1);
		}
#else
		if(kbhit()) {
			demo_keyboard(getch(), 1);
		}
#endif
		if(quit) goto break_evloop;

		if(use_mouse) {
			mouse_bmask = read_mouse(&mouse_x, &mouse_y);
		}
		if(use_sball && sball_pending()) {
			sball_event ev;
			while(sball_getevent(&ev)) {
				handle_sball_event(&ev);
			}
			recalc_sball_matrix(sball_matrix);
		}

		time_msec = get_msec();
		demo_draw();
	}

break_evloop:
	set_text_mode();
	demo_cleanup();
#ifndef NOKEYB
	kb_shutdown();
#endif
	if(use_sball) {
		sball_shutdown();
	}
	return 0;
}

void demo_quit(void)
{
	quit = 1;
}

void swap_buffers(void *pixels)
{
	if(pixels) {
		/* just memcpy to the front buffer */
		if(opt.vsync) {
			wait_vsync();
		}
		drawFps(pixels);
		memcpy(vmem_front, pixels, fbsize);

	} else {
		/* attempt page flipping */
		void *next;

		drawFps(vmem_back);
		if((next = page_flip(opt.vsync ? FLIP_VBLANK_WAIT : FLIP_NOW))) {
			assert(next == vmem_back);
			vmem_back = vmem_front;
			vmem_front = next;
		} else {
			/* failed to page flip, assume we drew in the front buffer then
			 * and just wait for vsync if necessary
			 */
			if(opt.vsync) {
				wait_vsync();
			}
		}
	}
}


#define TX(ev)	((ev)->motion.motion[0])
#define TY(ev)	((ev)->motion.motion[1])
#define TZ(ev)	((ev)->motion.motion[2])
#define RX(ev)	((ev)->motion.motion[3])
#define RY(ev)	((ev)->motion.motion[4])
#define RZ(ev)	((ev)->motion.motion[5])

static int handle_sball_event(sball_event *ev)
{
	switch(ev->type) {
	case SBALL_EV_MOTION:
		if(RX(ev) | RY(ev) | RZ(ev)) {
			float rx = (float)RX(ev);
			float ry = (float)RY(ev);
			float rz = (float)RZ(ev);
			float axis_len = sqrt(rx * rx + ry * ry + rz * rz);
			if(axis_len > 0.0) {
				rot = quat_rotate(rot, axis_len * 0.001, -rx / axis_len,
						-ry / axis_len, -rz / axis_len);
			}
		}

		pos.x += TX(ev) * 0.001;
		pos.y += TY(ev) * 0.001;
		pos.z += TZ(ev) * 0.001;
		break;

	case SBALL_EV_BUTTON:
		if(ev->button.pressed) {
			pos = v3_cons(0, 0, 0);
			rot = quat_cons(1, 0, 0, 0);
		}
		break;
	}

	return 0;
}

void recalc_sball_matrix(float *xform)
{
	quat_to_mat(xform, rot);
	xform[12] = pos.x;
	xform[13] = pos.y;
	xform[14] = pos.z;
}
