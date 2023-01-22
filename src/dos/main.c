#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "demo.h"
#include "keyb.h"
#include "timer.h"
#include "gfx.h"
#include "logger.h"
#include "cdpmi.h"
#include "audio.h"
#include "mouse.h"
#include "sball.h"
#include "vmath.h"
#include "cpuid.h"
#include "pci.h"

static int handle_sball_event(sball_event *ev);
static void recalc_sball_matrix(float *xform);

static int quit;

int force_snd_config;

static int use_sball;
static vec3_t pos = {0, 0, 0};
static quat_t rot = {0, 0, 0, 1};

int dblsize;

int main(int argc, char **argv)
{
	int i;
	int vmidx, status = 0;

	for(i=1; i<argc; i++) {
		if(strcmp(argv[i], "-setup") == 0) {
			force_snd_config = 1;
		}
	}

#ifdef __DJGPP__
	__djgpp_nearptr_enable();
#endif

	init_logger("demo.log");

#ifdef __WATCOMC__
	printf("watcom build\n");
#elif defined(__DJGPP__)
	printf("djgpp build\n");
#endif

	if(read_cpuid(&cpuid) == 0) {
		print_cpuid(&cpuid);
	}

	/* au_init needs to be called early, before init_timer, and also before
	 * we enter graphics mode, to use the midas configuration tool if necessary
	 */
	if(au_init() == -1) {
		return 1;
	}

	init_timer(100);
	kb_init(32);

	if(init_pci() != -1) {
		/* TODO detect and initialize S3 virge */
	}

	if(init_video() == -1) {
		return 1;
	}

	if((vmidx = match_video_mode(FB_WIDTH, FB_HEIGHT, FB_BPP)) == -1) {
		if((vmidx = match_video_mode(FB_WIDTH * 2, FB_HEIGHT * 2, FB_BPP)) == -1) {
			return 1;
		}
		dblsize = 1;
		printf("Warning: failed to find suitable, %dx%d video mode. Will use 2x scaling, performance might suffer\n",
				FB_WIDTH, FB_HEIGHT);
	}
	if(!(vmem = set_video_mode(vmidx, 1))) {
		return 1;
	}

	if(opt.mouse) {
		if((opt.mouse = have_mouse())) {
			printf("initializing mouse input\n");
			set_mouse_limits(0, 0, FB_WIDTH - 1, FB_HEIGHT - 1);
			set_mouse(FB_WIDTH / 2, FB_HEIGHT / 2);
		}
	}

	if(demo_init(argc, argv) == -1) {
		status = -1;
		goto break_evloop;
	}

	if(opt.sball && sball_init() == 0) {
		use_sball = 1;
	}

	reset_timer();

	for(;;) {
		int key;
		while((key = kb_getkey()) != -1) {
			demo_keyboard(key, 1);
			if(quit) goto break_evloop;
		}

		if(opt.mouse) {
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
	demo_cleanup();
	set_text_mode();
	cleanup_video();
	kb_shutdown();
	au_shutdown();
	if(use_sball) {
		sball_shutdown();
	}
	return status;
}

void demo_quit(void)
{
	quit = 1;
}

void demo_abort(void)
{
	set_text_mode();
	stop_logger();
	printf("demo_abort called. see demo.log for details. Last lines:\n\n");
	print_tail("demo.log");
	abort();
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
