#include <stdio.h>
#include "demo.h"
#include "keyb.h"
#include "timer.h"
#include "gfx.h"

static int quit;

int main(int argc, char **argv)
{
	init_timer(100);
	kb_init(32);

	if(!(fb_pixels = set_video_mode(fb_width, fb_height, fb_bpp))) {
		return 1;
	}

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

		/*wait_vsync();*/
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
