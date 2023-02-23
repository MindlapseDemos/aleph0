#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <SDL/SDL.h>
#include "demo.h"
#include "tinyfps.h"
#include "timer.h"
#include "cfgopt.h"
#include "sball.h"
#include "vmath.h"
#include "cpuid.h"

static void handle_event(SDL_Event *ev);
static void toggle_fullscreen(void);

static int handle_sball_event(sball_event *ev);
static void recalc_sball_matrix(float *xform);

static int sdlkey_to_demokey(int sdlkey, unsigned int mod);


static int quit;
static SDL_Surface *fbsurf;

static int fbscale = 2;
static int xsz, ysz;
static unsigned int sdl_flags = SDL_SWSURFACE;

static int use_sball;
static vec3_t pos = {0, 0, 0};
static quat_t rot = {0, 0, 0, 1};


int main(int argc, char **argv)
{
	int s;
	char *env;

	if((env = getenv("FBSCALE")) && (s = atoi(env))) {
		fbscale = s;
		printf("Framebuffer scaling x%d\n", fbscale);
	}

	xsz = FB_WIDTH * fbscale;
	ysz = FB_HEIGHT * fbscale;

	/* now start_loadscr sets up fb_pixels to the space used by the loading image,
	 * so no need to allocate another framebuffer
	 */
#if 0
	/* allocate 1 extra row as a guard band, until we fucking fix the rasterizer */
	if(!(fb_pixels = malloc(FB_WIDTH * (FB_HEIGHT + 1) * FB_BPP / CHAR_BIT))) {
		fprintf(stderr, "failed to allocate virtual framebuffer\n");
		return 1;
	}
#endif

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE);
	if(!(fbsurf = SDL_SetVideoMode(xsz, ysz, FB_BPP, sdl_flags))) {
		fprintf(stderr, "failed to set video mode %dx%d %dbpp\n", FB_WIDTH, FB_HEIGHT, FB_BPP);
		/*free(fb_pixels);*/
		SDL_Quit();
		return 1;
	}
	SDL_WM_SetCaption("dosdemo/SDL", 0);
	SDL_ShowCursor(0);

	if(read_cpuid(&cpuid) == 0) {
		print_cpuid(&cpuid);
	}

	time_msec = 0;
	if(demo_init_cfgopt(argc, argv) == -1 || demo_init() == -1) {
		/*free(fb_pixels);*/
		SDL_Quit();
		return 1;
	}
	vmem = fb_pixels;

	if(opt.sball && sball_init() == 0) {
		use_sball = 1;
	}

	reset_timer();

	while(!quit) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev)) {
			handle_event(&ev);
			if(quit) goto break_evloop;
		}

		if(use_sball) {
			while(sball_pending()) {
				sball_event ev;
				sball_getevent(&ev);
				handle_sball_event(&ev);
			}
			recalc_sball_matrix(sball_matrix);
		}

		time_msec = get_msec();
		demo_draw();
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

void wait_vsync(void)
{
	unsigned long start = SDL_GetTicks();
	unsigned long until = (start | 0xf) + 1;
	while(SDL_GetTicks() <= until);
}

void swap_buffers(void *pixels)
{
	int i, j;
	unsigned short *sptr, *dptr;

	demo_post_draw(pixels ? pixels : fb_pixels);

	if(opt.vsync) {
		wait_vsync();
	}

	if(SDL_MUSTLOCK(fbsurf)) {
		SDL_LockSurface(fbsurf);
	}

	sptr = fb_pixels;
	dptr = (unsigned short*)fbsurf->pixels + (fbsurf->w - xsz) / 2;
	for(i=0; i<FB_HEIGHT; i++) {
		for(j=0; j<FB_WIDTH; j++) {
			int x, y;
			unsigned short pixel = *sptr++;

			for(y=0; y<fbscale; y++) {
				for(x=0; x<fbscale; x++) {
					dptr[y * fbsurf->w + x] = pixel;
				}
			}
			dptr += fbscale;
		}
		dptr += (fbsurf->w - FB_WIDTH) * fbscale;
	}

	if(SDL_MUSTLOCK(fbsurf)) {
		SDL_UnlockSurface(fbsurf);
	}
	SDL_Flip(fbsurf);
}

static int bnmask(int sdlbn)
{
	switch(sdlbn) {
	case SDL_BUTTON_LEFT:
		return MOUSE_BN_LEFT;
	case SDL_BUTTON_RIGHT:
		return MOUSE_BN_RIGHT;
	case SDL_BUTTON_MIDDLE:
		return MOUSE_BN_MIDDLE;
	default:
		break;
	}
	return 0;
}

static void handle_event(SDL_Event *ev)
{
	int key;

	switch(ev->type) {
	case SDL_QUIT:
		quit = 1;
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		if(ev->key.keysym.sym == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT) &&
				ev->key.state == SDL_PRESSED) {
			toggle_fullscreen();
			break;
		}
		key = sdlkey_to_demokey(ev->key.keysym.sym, ev->key.keysym.mod);
		demo_keyboard(key, ev->key.state == SDL_PRESSED ? 1 : 0);
		break;

	case SDL_MOUSEMOTION:
		mouse_x = ev->motion.x / fbscale;
		mouse_y = ev->motion.y / fbscale;
		break;

	case SDL_MOUSEBUTTONDOWN:
		mouse_bmask |= bnmask(ev->button.button);
		if(0) {
	case SDL_MOUSEBUTTONUP:
			mouse_bmask &= ~bnmask(ev->button.button);
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

	if(!(newsurf = SDL_SetVideoMode(xsz, ysz, FB_BPP, newflags))) {
		fprintf(stderr, "failed to go %s\n", newflags & SDL_FULLSCREEN ? "fullscreen" : "windowed");
		return;
	}

	fbsurf = newsurf;
	sdl_flags = newflags;
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

static void recalc_sball_matrix(float *xform)
{
	quat_to_mat(xform, rot);
	xform[12] = pos.x;
	xform[13] = pos.y;
	xform[14] = pos.z;
}

#define SSORG	'\''
#define SSEND	'`'
static char symshift[] = {
	'"', 0, 0, 0, 0, '<', '_', '>', '?',
	')', '!', '@', '#', '$', '%', '^', '&', '*', '(',
	0, ':', 0, '+', 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'{', '|', '}', 0, 0, '~'
};


static int sdlkey_to_demokey(int sdlkey, unsigned int mod)
{
	if(sdlkey < 128) {
		if(mod & (KMOD_SHIFT)) {
			if(sdlkey >= 'a' && sdlkey <= 'z') {
				sdlkey = toupper(sdlkey);
			} else if(sdlkey >= SSORG && sdlkey <= SSEND) {
				sdlkey = symshift[sdlkey - SSORG];
			}
		}
		return sdlkey;
	}
	if(sdlkey < 256) return 0;
	return sdlkey - 128;
}
