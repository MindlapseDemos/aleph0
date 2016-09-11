#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "demo.h"
#include "keyb.h"
#include "mouse.h"
#include "timer.h"
#include "gfx.h"

static int quit;
static int use_mouse;
static long fbsize;

int main(int argc, char **argv)
{
	fbsize = fb_width * fb_height * fb_bpp / CHAR_BIT;

	init_timer(100);
	kb_init(32);

	if((use_mouse = have_mouse())) {
		set_mouse_limits(0, 0, fb_width, fb_height);
		set_mouse(fb_width / 2, fb_height / 2);
	}

	if(!(fb_pixels = malloc(fbsize))) {
		fprintf(stderr, "failed to allocate backbuffer\n");
		return 1;
	}

	if(!(vmem_front = set_video_mode(fb_width, fb_height, fb_bpp))) {
		return 1;
	}
	/* TODO implement multiple video memory pages for flipping */
	vmem_back = vmem_front;

	if(demo_init(argc, argv) == -1) {
		set_text_mode();
		return 1;
	}
	reset_timer();

	while(!quit) {
		int key;
		while((key = kb_getkey()) != -1) {
			demo_keyboard(key, 1);
		}
		if(quit) goto break_evloop;

		if(use_mouse) {
			mouse_bmask = read_mouse(&mouse_x, &mouse_y);
		}

		time_msec = get_msec();
		demo_draw();
	}

break_evloop:
	set_text_mode();
	demo_cleanup();
	kb_shutdown();
	return 0;
}

void demo_quit(void)
{
	quit = 1;
}

void swap_buffers(void *pixels)
{
	/* TODO implement page flipping */
	if(pixels) {
		/*wait_vsync();*/
		memcpy(vmem_front, pixels, fbsize);
	}
}
