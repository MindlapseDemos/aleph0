#ifndef DEMO_H_
#define DEMO_H_

#include "inttypes.h"

extern int fb_width, fb_height, fb_bpp;
extern uint16_t *fb_pixels;	/* system-RAM pixel buffer: use swap_buffers(fb_pixels) */
extern uint16_t *vmem;		/* visible video memory pointer */

extern unsigned long time_msec;
extern int mouse_x, mouse_y;
extern unsigned int mouse_bmask;

enum {
	MOUSE_BN_LEFT		= 1,
	MOUSE_BN_RIGHT		= 2,
	MOUSE_BN_MIDDLE		= 4
};

/* special keys */
enum {
	KB_ESC = 27,
	KB_BACKSP = 127,

	KB_NUM_0, KB_NUM_1, KB_NUM_2, KB_NUM_3, KB_NUM_4,
	KB_NUM_5, KB_NUM_6, KB_NUM_7, KB_NUM_8, KB_NUM_9,
	KB_NUM_DOT, KB_NUM_DIV, KB_NUM_MUL, KB_NUM_MINUS, KB_NUM_PLUS, KB_NUM_ENTER, KB_NUM_EQUALS,
	KB_UP, KB_DOWN, KB_RIGHT, KB_LEFT,
	KB_INSERT, KB_HOME, KB_END, KB_PGUP, KB_PGDN,
	KB_F1, KB_F2, KB_F3, KB_F4, KB_F5, KB_F6,
	KB_F7, KB_F8, KB_F9, KB_F10, KB_F11, KB_F12,
	KB_F13, KB_F14, KB_F15,
	KB_NUMLK, KB_CAPSLK, KB_SCRLK,
	KB_RSHIFT, KB_LSHIFT, KB_RCTRL, KB_LCTRL, KB_RALT, KB_LALT,
	KB_RMETA, KB_LMETA, KB_LSUPER, KB_RSUPER, KB_MODE, KB_COMPOSE,
	KB_HELP, KB_PRINT, KB_SYSRQ, KB_BREAK
};

extern float sball_matrix[16];

int demo_init(int argc, char **argv);
void demo_cleanup(void);

void demo_draw(void);
void demo_post_draw(void *pixels);

void demo_keyboard(int key, int press);


/* defined in main_*.c */
void demo_quit(void);
unsigned long get_msec(void);
void set_palette(int idx, int r, int g, int b);

/* if pixels is 0, it defaults to fb_pixels */
void swap_buffers(void *pixels);

void change_screen(int idx);

/* call each frame to get 3D viewing spherical coordinates */
void mouse_orbit_update(float *theta, float *phi, float *dist);

void draw_mouse_pointer(uint16_t *fb);

/* compiled sprites available */
typedef void (*cs_font_func)(void *, int, int, int);
void cs_dbgfont(void *fb, int x, int y, int idx);
void cs_confont(void *fb, int x, int y, int idx);

/* helper to print text with cs_font */
void cs_puts_font(cs_font_func csfont, int sz, void *fb, int x, int y, const char *str);

#define cs_dputs(fb, x, y, idx)	cs_puts_font(cs_dbgfont, 9, fb, x, y, idx)
#define cs_cputs(fb, x, y, idx)	cs_puts_font(cs_confont, 6, fb, x, y, idx)

#endif	/* DEMO_H_ */
