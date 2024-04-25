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
#include "cpuid.h"
#include "pci.h"

static int handle_sball_event(sball_event *ev);
static void recalc_sball_matrix(float *xform);

static int quit;

static int use_sball;
cgm_vec3 sball_pos = {0, 0, 0};
cgm_quat sball_rot = {0, 0, 0, 1};


int main(int argc, char **argv)
{
	int i;
	int vmidx, status = 0;

	if(demo_init_cfgopt(argc, argv) == -1) {
		return 1;
	}

#ifdef __DJGPP__
	__djgpp_nearptr_enable();
#endif

	init_logger(opt.logfile);

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
		if((vmidx = match_video_mode(FB_WIDTH, 200, FB_BPP)) == -1) {
			demo_abort();
		}
		printf("Warning: failed to find suitable, %dx%d video mode. fallback to a 200-line mode, part of the image will be cropped\n",
				FB_WIDTH, FB_HEIGHT);
	}
	if(!(vmem = set_video_mode(vmidx, 1))) {
		demo_abort();
	}

	if(opt.mouse) {
		if((opt.mouse = have_mouse())) {
			printf("initializing mouse input\n");
			set_mouse_limits(0, 0, FB_WIDTH - 1, FB_HEIGHT - 1);
			set_mouse(FB_WIDTH / 2, FB_HEIGHT / 2);
		}
	}

	if(demo_init() == -1) {
		status = -1;
		goto break_evloop;
	}

	if(opt.sball && sball_init() == 0) {
		use_sball = 1;
	}

	fflush(stdout);
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
	print_tail(opt.logfile);
	exit(1);
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

			float s = (float)rsqrt(rx * rx + ry * ry + rz * rz);
			cgm_qrotate(&sball_rot, 0.001 / s, -rx * s, -ry * s, - rz * s);
		}

		sball_pos.x += TX(ev) * 0.001;
		sball_pos.y += TY(ev) * 0.001;
		sball_pos.z += TZ(ev) * 0.001;
		break;

	case SBALL_EV_BUTTON:
		if(ev->button.pressed) {
			cgm_vcons(&sball_pos, 0, 0, 0);
			cgm_qcons(&sball_rot, 0, 0, 0, 1);
		}
		break;
	}

	return 0;
}

void recalc_sball_matrix(float *xform)
{
	cgm_mrotation_quat(xform, &sball_rot);
	xform[12] = sball_pos.x;
	xform[13] = sball_pos.y;
	xform[14] = sball_pos.z;
}
