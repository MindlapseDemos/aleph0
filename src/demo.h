#ifndef DEMO_H_
#define DEMO_H_

extern int fb_width, fb_height, fb_bpp;
extern unsigned char *fb_pixels;
extern unsigned long time_msec;

int demo_init(int argc, char **argv);
void demo_cleanup(void);

void demo_draw(void);

void demo_keyboard(int key, int state);

/* defined in main_*.c */
void demo_quit(void);
unsigned long get_msec(void);
void set_palette(int idx, int r, int g, int b);

#endif	/* DEMO_H_ */
