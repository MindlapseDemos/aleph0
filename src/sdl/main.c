#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <SDL/SDL.h>
#include "demo.h"

static void handle_event(SDL_Event *ev);

static int quit;
static long start_time;
static SDL_Surface *fbsurf;

static int fbscale = 2;
static int xsz, ysz;

int main(int argc, char **argv)
{
	int s, i, j;
	char *env;
	unsigned short *sptr, *dptr;
	unsigned int sdl_flags = SDL_SWSURFACE;

	if((env = getenv("FBSCALE")) && (s = atoi(env))) {
		fbscale = s;
		printf("Framebuffer scaling x%d\n", fbscale);
	}

	xsz = fb_width * fbscale;
	ysz = fb_height * fbscale;

	if(!(fb_pixels = malloc(fb_width * fb_height * fb_bpp / CHAR_BIT))) {
		fprintf(stderr, "failed to allocate virtual framebuffer\n");
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	if(!(fbsurf = SDL_SetVideoMode(xsz, ysz, fb_bpp, sdl_flags))) {
		fprintf(stderr, "failed to set video mode %dx%d %dbpp\n", fb_width, fb_height, fb_bpp);
		free(fb_pixels);
		SDL_Quit();
		return 1;
	}
	SDL_WM_SetCaption("dosdemo/SDL", 0);

	time_msec = 0;
	if(demo_init(argc, argv) == -1) {
		free(fb_pixels);
		SDL_Quit();
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
		demo_draw();

		if(SDL_MUSTLOCK(fbsurf)) {
			SDL_LockSurface(fbsurf);
		}

		sptr = fb_pixels;
		dptr = fbsurf->pixels;
		for(i=0; i<fb_height; i++) {
			for(j=0; j<fb_width; j++) {
				int x, y;
				unsigned short pixel = *sptr++;

				for(y=0; y<fbscale; y++) {
					for(x=0; x<fbscale; x++) {
						dptr[y * xsz + x] = pixel;
					}
				}
				dptr += fbscale;
			}
			dptr += xsz * (fbscale - 1);
		}

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
