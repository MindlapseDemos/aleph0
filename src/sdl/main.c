#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include "demo.h"

static void handle_event(SDL_Event *ev);

static int quit;
static long start_time;
static SDL_Surface *fbsurf;

int main(int argc, char **argv)
{
	unsigned int sdl_flags = SDL_SWSURFACE;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	if(!(fbsurf = SDL_SetVideoMode(fb_width, fb_height, fb_bpp, sdl_flags))) {
		fprintf(stderr, "failed to set video mode %dx%d %dbpp\n", fb_width, fb_height, fb_bpp);
		return 1;
	}
	SDL_WM_SetCaption("dosdemo SDLemu", 0);

	time_msec = 0;
	if(demo_init(argc, argv) == -1) {
		return 1;
	}
	start_time = SDL_GetTicks();

	while(!quit) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev)) {
			handle_event(&ev);
			if(quit) goto break_evloop;
		}

		time_msec = SDL_GetTicks() - start_time;
		if(SDL_MUSTLOCK(fbsurf)) {
			SDL_LockSurface(fbsurf);
		}
		fb_pixels = fbsurf->pixels;

		demo_draw();

		if(SDL_MUSTLOCK(fbsurf)) {
			SDL_UnlockSurface(fbsurf);
		}
		SDL_Flip(fbsurf);
	}

break_evloop:
	demo_cleanup();
	SDL_Quit();
	return 0;
}

void demo_quit(void)
{
	quit = 1;
}

static void handle_event(SDL_Event *ev)
{
	switch(ev->type) {
	case SDL_QUIT:
		quit = 1;
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		demo_keyboard(ev->key.keysym.sym, ev->key.state == SDL_PRESSED ? 1 : 0);
		break;

	default:
		break;
	}
}
