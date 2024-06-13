#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <SDL/SDL.h>
#include "demo.h"
#include "tinyfps.h"
#include "timer.h"
#include "cfgopt.h"
#include "sball.h"
#include "vmath.h"
#include "gfxutil.h"
#include "cpuid.h"
#include "util.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


static void mainloop_iter(void);
static void handle_event(SDL_Event *ev);
static void toggle_fullscreen(void);

static int handle_sball_event(sball_event *ev);
static void recalc_sball_matrix(float *xform);

static int sdlkey_to_demokey(int sdlkey, unsigned int mod);


static int quit;
static SDL_Surface *fbsurf;

static int fbscale = 3;
static int xsz, ysz;
static unsigned int sdl_flags = SDL_SWSURFACE;

#define MODE(w, h)	\
	{0, w, h, 16, w * 2, 5, 6, 5, 11, 5, 0, 0xf800, 0x7e0, 0x1f, 0xbadf00d, 2, 0}
static struct video_mode vmodes[] = {
	MODE(320, 240), MODE(400, 300), MODE(512, 384), MODE(640, 480),
	MODE(800, 600), MODE(1024, 768), MODE(1280, 960), MODE(1280, 1024),
	MODE(1920, 1080), MODE(1600, 1200), MODE(1920, 1200)
};
static struct video_mode *cur_vmode;

static unsigned int num_pressed;
static unsigned char keystate[256];

static int use_sball;
cgm_vec3 sball_pos = {0, 0, 0};
cgm_quat sball_rot = {0, 0, 0, 1};


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
	if(!(fbsurf = SDL_SetVideoMode(xsz, ysz, 32, sdl_flags))) {
		fprintf(stderr, "failed to set video mode %dx%d %dbpp\n", FB_WIDTH, FB_HEIGHT, FB_BPP);
		/*free(fb_pixels);*/
		SDL_Quit();
		return 1;
	}
	SDL_WM_SetCaption("dosdemo/SDL", 0);
	SDL_ShowCursor(0);

#ifndef __EMSCRIPTEN__
	if(read_cpuid(&cpuid) == 0) {
		print_cpuid(&cpuid);
	}
#endif

	if(!set_video_mode(match_video_mode(FB_WIDTH, FB_HEIGHT, FB_BPP), 1)) {
		return 1;
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

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(mainloop_iter, -1, 0);
	return 0;
#else
	while(!quit) {
		mainloop_iter();
	}
#endif

	demo_cleanup();
	SDL_Quit();
	return 0;
}

static void mainloop_iter(void)
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev)) {
		handle_event(&ev);
		if(quit) return;
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

void demo_quit(void)
{
	quit = 1;
}

void demo_abort(void)
{
	abort();
}

struct video_mode *video_modes(void)
{
	return vmodes;
}

int num_video_modes(void)
{
	return sizeof vmodes / sizeof *vmodes;
}

struct video_mode *get_video_mode(int idx)
{
	if(idx == VMODE_CURRENT) {
		return cur_vmode;
	}
	return vmodes + idx;
}

int match_video_mode(int xsz, int ysz, int bpp)
{
	struct video_mode *vm = vmodes;
	int i, count = num_video_modes();

	for(i=0; i<count; i++) {
		if(vm->xsz == xsz && vm->ysz == ysz && vm->bpp == bpp) {
			return i;
		}
		vm++;
	}
	return -1;
}

void *set_video_mode(int idx, int nbuf)
{
	struct video_mode *vm = vmodes + idx;

	if(cur_vmode == vm) {
		return vmem;
	}

	if(demo_resizefb(vm->xsz, vm->ysz, vm->bpp) == -1) {
		fprintf(stderr, "failed to allocate virtual framebuffer\n");
		return 0;
	}
	vmem = fb_pixels;

	cur_vmode = vm;
	return vmem;
}


void wait_vsync(void)
{
	unsigned long start = SDL_GetTicks();
	unsigned long until = (start | 0xf) + 1;
	while(SDL_GetTicks() <= until);
}

void blit_frame(void *pixels, int vsync)
{
	int i, j;
	unsigned short *sptr;
	uint32_t *dptr;

	demo_post_draw(pixels);

	if(vsync) {
		wait_vsync();
	}

	if(SDL_MUSTLOCK(fbsurf)) {
		SDL_LockSurface(fbsurf);
	}

	sptr = fb_pixels;
	dptr = (uint32_t*)fbsurf->pixels + (fbsurf->w - xsz) / 2;
	for(i=0; i<FB_HEIGHT; i++) {
		for(j=0; j<FB_WIDTH; j++) {
			int x, y;
			unsigned short pixel = *sptr++;

			int r = UNPACK_R16(pixel);
			int g = UNPACK_G16(pixel);
			int b = UNPACK_B16(pixel);
#ifdef __EMSCRIPTEN__
			uint32_t pix32 = PACK_RGB32(b, g, r);
#else
			uint32_t pix32 = PACK_RGB32(r, g, b);
#endif

			for(y=0; y<fbscale; y++) {
				for(x=0; x<fbscale; x++) {
					dptr[y * fbsurf->w + x] = pix32;
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

int kb_isdown(int key)
{
	switch(key) {
	case KB_ANY:
		return num_pressed;

	case KB_ALT:
		return keystate[KB_LALT] + keystate[KB_RALT];

	case KB_CTRL:
		return keystate[KB_LCTRL] + keystate[KB_RCTRL];
	}

	if(isalpha(key)) {
		key = tolower(key);
	}
	return keystate[key];
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
		if(ev->key.state == SDL_PRESSED) {
			keystate[key] = 1;
			demo_keyboard(key, 1);
		} else {
			keystate[key] = 0;
			demo_keyboard(key, 0);
		}
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

static void recalc_sball_matrix(float *xform)
{
	cgm_mrotation_quat(xform, &sball_rot);
	xform[12] = sball_pos.x;
	xform[13] = sball_pos.y;
	xform[14] = sball_pos.z;
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
