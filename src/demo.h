#ifndef DEMO_H_
#define DEMO_H_

#include "inttypes.h"

extern int fb_width, fb_height, fb_bpp;
extern uint16_t *fb_pixels;	/* system-RAM pixel buffer: use swap_buffers(fb_pixels) */
/* video memory pointers. might both point to the front buffer if there is not
 * enough memory for page flipping. use swap_buffers(0) to flip. */
extern uint16_t *vmem_back, *vmem_front;

extern unsigned long time_msec;
extern int mouse_x, mouse_y;
extern unsigned int mouse_bmask;

enum {
	MOUSE_BN_LEFT		= 1,
	MOUSE_BN_RIGHT		= 2,
	MOUSE_BN_MIDDLE		= 4
};

extern float sball_matrix[16];

int demo_init(int argc, char **argv);
void demo_cleanup(void);

void demo_draw(void);

void demo_keyboard(int key, int press);


/* defined in main_*.c */
void demo_quit(void);
unsigned long get_msec(void);
void set_palette(int idx, int r, int g, int b);

/* pass 0 to just swap vmem_back/vmem_front with page flipping
 * pass a pointer to a system-ram pixel buffer to copy it to vmem_front,
 * instead of flipping.
 */
void swap_buffers(void *pixels);

/* call each frame to get 3D viewing spherical coordinates */
void mouse_orbit_update(float *theta, float *phi, float *dist);

void draw_mouse_pointer(uint16_t *fb);

#endif	/* DEMO_H_ */
