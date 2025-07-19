#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#ifndef __APPLE__
#include "miniglut.h"
#else
#include <GLUT/glut.h>
#endif
#include "demo.h"
#include "gfx.h"
#include "gfxutil.h"
#include "timer.h"
#include "audio.h"
#include "cfgopt.h"
#include "cgmath/cgmath.h"
#include "util.h"
#include "cpuid.h"

static void display(void);
static void idle(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static int translate_special(int skey);
static void mouse_button(int bn, int st, int x, int y);
static void mouse_motion(int x, int y);
static void sball_motion(int x, int y, int z);
static void sball_rotate(int x, int y, int z);
static void sball_button(int bn, int st);
static void recalc_sball_matrix(void);
static void set_fullscreen(int fs);
static void set_vsync(int vsync);

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

static unsigned long start_time;
static unsigned int modkeys;

static int win_width, win_height;
static float win_aspect;
#ifndef NO_GLTEX
static unsigned int tex;
static int tex_xsz, tex_ysz;
#endif

#ifdef __unix__
#include <GL/glx.h>
static Display *xdpy;
static Window xwin;

typedef void (*swapint_ext_t)(Display*, Window, int);
typedef void (*swapint_mesa_t)(int);

static swapint_ext_t glx_swap_interval_ext;
static swapint_mesa_t glx_swap_interval_mesa;

#endif
#ifdef _WIN32
#include <windows.h>
static PROC wgl_swap_interval_ext;
#endif

static int use_sball;
cgm_vec3 sball_pos = {0, 0, 0};
cgm_quat sball_rot = {0, 0, 0, 1};

cgm_vec3 sball_view_pos = {0, 0, 0};
cgm_quat sball_view_rot = {0, 0, 0, 1};

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	if(glutGet(GLUT_SCREEN_HEIGHT) <= 960 && opt.scale > 2) {
		opt.scale = 2;	/* constrain default scale factor on low-res */
	}

	if(demo_init_cfgopt(argc, argv) == -1) {
		return 1;
	}
	glutInitWindowSize(320 * opt.scale, 240 * opt.scale);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("aleph null / mindlapse");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	if(opt.mouse) {
		glutMouseFunc(mouse_button);
		glutMotionFunc(mouse_motion);
		glutPassiveMotionFunc(mouse_motion);
	}
	if(opt.sball) {
		glutSpaceballMotionFunc(sball_motion);
		glutSpaceballRotateFunc(sball_rotate);
		glutSpaceballButtonFunc(sball_button);
	}

	glutSetCursor(GLUT_CURSOR_NONE);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);

#ifndef NO_ASM
	if(read_cpuid(&cpuid) == 0) {
		print_cpuid(&cpuid);
	}
#endif

	if(!set_video_mode(match_video_mode(FB_WIDTH, FB_HEIGHT, FB_BPP), 1)) {
		return 1;
	}

#ifdef __unix__
	xdpy = glXGetCurrentDisplay();
	xwin = glXGetCurrentDrawable();

	glx_swap_interval_ext = (swapint_ext_t)glXGetProcAddress((unsigned char*)"glXSwapIntervalEXT");
	glx_swap_interval_mesa = (swapint_mesa_t)glXGetProcAddress((unsigned char*)"glXSwapIntervalMESA");
#endif
#ifdef _WIN32
	wgl_swap_interval_ext = wglGetProcAddress("wglSwapIntervalEXT");
#endif

	if(au_init() == -1) {
		return 1;
	}
	time_msec = 0;

	if(opt.fullscreen) {
		set_fullscreen(opt.fullscreen);
		reshape(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
	} else {
		reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}

	if(demo_init() == -1) {
		return 1;
	}
	atexit(demo_cleanup);

	reset_timer(0);

	glutMainLoop();
	return 0;
}

void demo_quit(void)
{
	exit(0);
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

static uint32_t *convbuf;
static int convbuf_size;

void *set_video_mode(int idx, int nbuf)
{
	struct video_mode *vm = vmodes + idx;

	if(cur_vmode == vm) {
		return vmem;
	}

#ifdef NO_GLTEX
	glPixelZoom(opt.scale, opt.scale);
#else
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	tex_xsz = next_pow2(vm->xsz);
	tex_ysz = next_pow2(vm->ysz);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_xsz, tex_ysz, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, 0);
	if(opt.scaler == SCALER_LINEAR) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef((float)vm->xsz / tex_xsz, (float)vm->ysz / tex_ysz, 1);
#endif

	if(vm->xsz * vm->ysz > convbuf_size) {
		convbuf_size = vm->xsz * vm->ysz;
		free(convbuf);
		convbuf = malloc(convbuf_size * sizeof *convbuf);
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
}

void blit_frame(void *pixels, int vsync)
{
	unsigned int i, j;
	uint32_t *dptr = convbuf;
	uint16_t *sptr = pixels;
	static int prev_vsync = -1;

	if(vsync != prev_vsync) {
		set_vsync(vsync);
		prev_vsync = vsync;
	}

	demo_post_draw(pixels);

	sptr = (uint16_t*)pixels + (FB_HEIGHT - 1) * FB_WIDTH;
	for(i=0; i<FB_HEIGHT; i++) {
		for(j=0; j<FB_WIDTH; j++) {
			uint16_t pix = sptr[j];
			unsigned int r = UNPACK_R16(pix);
			unsigned int g = UNPACK_G16(pix);
			unsigned int b = UNPACK_B16(pix);
			*dptr++ = PACK_RGB32(b, g, r);
		}
		sptr -= FB_WIDTH;
	}

#ifdef NO_GLTEX
	glDrawPixels(FB_WIDTH, FB_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, convbuf);
#else
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FB_WIDTH, FB_HEIGHT, GL_RGBA,
			GL_UNSIGNED_BYTE, convbuf);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if(win_aspect >= FB_ASPECT) {
		glScalef(FB_ASPECT / win_aspect, 1, 1);
	} else {
		glScalef(1, win_aspect / FB_ASPECT, 1);
	}

	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(-1, 1);
	glEnd();
#endif

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
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

/* timer */
void init_timer(int res_hz)
{
}

void reset_timer(unsigned long ms)
{
	start_time = glutGet(GLUT_ELAPSED_TIME) - ms;
	time_msec = get_msec();
}

unsigned long get_msec(void)
{
	return glutGet(GLUT_ELAPSED_TIME) - start_time;
}

#ifdef _WIN32
#include <windows.h>

void sleep_msec(unsigned long msec)
{
	Sleep(msec);
}

#else
#include <unistd.h>

void sleep_msec(unsigned long msec)
{
	usleep(msec * 1000);
}
#endif

static void display(void)
{
	if(opt.sball) {
		recalc_sball_matrix();
	}

	time_msec = get_msec();
	demo_draw();
}

static void idle(void)
{
	glutPostRedisplay();
}

static void reshape(int x, int y)
{
	win_width = x;
	win_height = y;
	win_aspect = (float)x / (float)y;
	glViewport(0, 0, x, y);
}

static void keydown(unsigned char key, int x, int y)
{
	modkeys = glutGetModifiers();

	if((key == '\n' || key == '\r') && (modkeys & GLUT_ACTIVE_ALT)) {
		opt.fullscreen ^= 1;
		set_fullscreen(opt.fullscreen);
		return;
	}
	keystate[key] = 1;
	demo_keyboard(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	keystate[key] = 0;
	demo_keyboard(key, 0);
}

static void skeydown(int key, int x, int y)
{
#ifndef NO_GLTEX
	if(key == GLUT_KEY_F5) {
		opt.scaler = (opt.scaler + 1) % NUM_SCALERS;

		if(opt.scaler == SCALER_LINEAR) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}
#endif
	key = translate_special(key);
	keystate[key] = 1;
	demo_keyboard(key, 1);
}

static void skeyup(int key, int x, int y)
{
	key = translate_special(key);
	keystate[key] = 0;
	demo_keyboard(key, 0);
}

static int translate_special(int skey)
{
	switch(skey) {
	case 127:
		return 127;
	case GLUT_KEY_LEFT:
		return KB_LEFT;
	case GLUT_KEY_RIGHT:
		return KB_RIGHT;
	case GLUT_KEY_UP:
		return KB_UP;
	case GLUT_KEY_DOWN:
		return KB_DOWN;
	case GLUT_KEY_PAGE_UP:
		return KB_PGUP;
	case GLUT_KEY_PAGE_DOWN:
		return KB_PGDN;
	case GLUT_KEY_HOME:
		return KB_HOME;
	case GLUT_KEY_END:
		return KB_END;
	default:
		if(skey >= GLUT_KEY_F1 && skey <= GLUT_KEY_F12) {
			return KB_F1 + skey - GLUT_KEY_F1;
		}
	}
	return 0;
}

static void map_mouse_pos(int *xp, int *yp)
{
	int x = *xp;
	int y = *yp;

	/* TODO */
	*xp = x * FB_WIDTH / win_width;
	*yp = y * FB_HEIGHT / win_height;
}

static void mouse_button(int bn, int st, int x, int y)
{
	int bit;

	map_mouse_pos(&x, &y);
	mouse_x = x;
	mouse_y = y;

	switch(bn) {
	case GLUT_LEFT_BUTTON:
		bit = 0;
		break;
	case GLUT_RIGHT_BUTTON:
		bit = 1;
		break;
	case GLUT_MIDDLE_BUTTON:
		bit = 2;
		break;
	}

	if(st == GLUT_DOWN) {
		mouse_bmask |= 1 << bit;
	} else {
		mouse_bmask &= ~(1 << bit);
	}
}

static void mouse_motion(int x, int y)
{
	map_mouse_pos(&x, &y);
	mouse_x = x;
	mouse_y = y;
}

static cgm_vec3 sb_tmot, sb_rmot;

static void sball_motion(int x, int y, int z)
{
	cgm_vcons(&sb_tmot, x, y, -z);
	/*
	sball_pos.x += x * 0.001f;
	sball_pos.y += y * 0.001f;
	sball_pos.z -= z * 0.001f;
	*/
}

static void sball_rotate(int rx, int ry, int rz)
{
	cgm_vcons(&sb_rmot, -rx, -ry, rz);
	/*
	if(rx | ry | rz) {
		float s = (float)rsqrt(rx * rx + ry * ry + rz * rz);
		cgm_qrotate(&sball_rot, -0.001f / s, rx * s, ry * s, -rz * s);
	}
	*/
}

static void sball_button(int bn, int st)
{
	sball_pos.x = sball_pos.y = sball_pos.z = 0;
	sball_rot.x = sball_rot.y = sball_rot.z = 0;
	sball_rot.w = 1;
}

#define SBALL_RSPEED	0.0002f
#define SBALL_TSPEED	0.0004f

static void recalc_sball_matrix(void)
{
	float len;
	cgm_quat rot;
	cgm_vec3 vel;
	cgm_vec3 right, up, fwd;
	float rmat[16];

	if((len = cgm_vlength(&sb_rmot)) != 0.0f) {
		cgm_qrotation(&rot, len * SBALL_RSPEED, sb_rmot.x / len,
				sb_rmot.y / len, sb_rmot.z / len);
		cgm_qmul(&rot, &sball_rot);
		sball_rot = rot;
	}

	cgm_vcscale(&vel, &sb_tmot, SBALL_TSPEED);
	cgm_vadd(&sball_pos, &vel);

	cgm_mrotation_quat(rmat, &sball_rot);

	cgm_vmul_v3m3(&vel, rmat);
	cgm_vadd(&sball_view_pos, &vel);

	cgm_mtranslation(sball_view_matrix, -sball_view_pos.x, -sball_view_pos.y, -sball_view_pos.z);
	cgm_qnormalize(&sball_rot);
	cgm_mmul(sball_view_matrix, rmat);
	sball_view_rot = sball_rot;

	cgm_mrotation_quat(sball_matrix, &sball_rot);
	cgm_mtranspose(sball_matrix);
	sball_matrix[12] = sball_pos.x;
	sball_matrix[13] = sball_pos.y;
	sball_matrix[14] = sball_pos.z;

	cgm_vcons(&sb_rmot, 0, 0, 0);
	cgm_vcons(&sb_tmot, 0, 0, 0);
}


static void set_fullscreen(int fs)
{
	static int win_x, win_y;

	if(fs) {
		win_x = glutGet(GLUT_WINDOW_WIDTH);
		win_y = glutGet(GLUT_WINDOW_HEIGHT);
		glutFullScreen();
	} else {
		glutReshapeWindow(win_x, win_y);
	}
}

#ifdef __unix__
static void set_vsync(int vsync)
{
	if(glx_swap_interval_ext) {
		glx_swap_interval_ext(xdpy, xwin, vsync);
	} else if(glx_swap_interval_mesa) {
		glx_swap_interval_mesa(vsync);
	}
}
#endif
#ifdef WIN32
static void set_vsync(int vsync)
{
	if(wgl_swap_interval_ext) {
		wgl_swap_interval_ext(vsync);
	}
}
#endif
#ifdef __APPLE__
static void set_vsync(int vsync)
{
	/* TODO */
}
#endif
