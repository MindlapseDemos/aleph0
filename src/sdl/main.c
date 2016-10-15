#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <SDL/SDL.h>
#include "demo.h"
#include "tinyfps.h"

static void handle_event(SDL_Event *ev);
static void toggle_fullscreen(void);

static int quit;
static long start_time;
static SDL_Surface *fbsurf;

static int fbscale = 2;
static int xsz, ysz;
static unsigned int sdl_flags = SDL_SWSURFACE;

int main(int argc, char **argv)
{
	int s, i, j;
	char *env;
	unsigned short *sptr, *dptr;

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
	vmem_front = vmem_back = fb_pixels;

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
		drawFps(fb_pixels);

		if(SDL_MUSTLOCK(fbsurf)) {
			SDL_LockSurface(fbsurf);
		}

		sptr = fb_pixels;
		dptr = (unsigned short*)fbsurf->pixels + (fbsurf->w - xsz) / 2;
		for(i=0; i<fb_height; i++) {
			for(j=0; j<fb_width; j++) {
				int x, y;
				unsigned short pixel = *sptr++;

				for(y=0; y<fbscale; y++) {
					for(x=0; x<fbscale; x++) {
						dptr[y * fbsurf->w + x] = pixel;
					}
				}
				dptr += fbscale;
			}
			dptr += (fbsurf->w - fb_width) * fbscale;
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

void swap_buffers(void *pixels)
{
	/* do nothing, all pointers point to the same buffer */
}

static void handle_event(SDL_Event *ev)
{
	switch(ev->type) {
	case SDL_QUIT:
		quit = 1;
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		if(ev->key.keysym.sym == 'f') {
			if(ev->key.state == SDL_PRESSED) {
				toggle_fullscreen();
			}
			break;
		}
		demo_keyboard(ev->key.keysym.sym, ev->key.state == SDL_PRESSED ? 1 : 0);
		break;

	case SDL_MOUSEMOTION:
		mouse_x = ev->motion.x / fbscale;
		mouse_y = ev->motion.y / fbscale;
		break;

	case SDL_MOUSEBUTTONDOWN:
		mouse_bmask |= 1 << (ev->button.button - SDL_BUTTON_LEFT);
		if(0) {
	case SDL_MOUSEBUTTONUP:
			mouse_bmask &= ~(1 << (ev->button.button - SDL_BUTTON_LEFT));
		}
		mouse_x = ev->button.x / fbscale;
		mouse_y = ev->button.y / fbscale;
		break;

	default:
		break;
	}
}

static void toggle_fullscreen(void)
{
	SDL_Surface *newsurf;
	unsigned int newflags = sdl_flags ^ SDL_FULLSCREEN;

	if(!(newsurf = SDL_SetVideoMode(xsz, ysz, fb_bpp, newflags))) {
		fprintf(stderr, "failed to go %s\n", newflags & SDL_FULLSCREEN ? "fullscreen" : "windowed");
		return;
	}

	fbsurf = newsurf;
	sdl_flags = newflags;
}
