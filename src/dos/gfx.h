#ifndef GFX_H_
#define GFX_H_

#ifdef __cplusplus
extern "C" {
#endif

void *set_video_mode(int xsz, int ysz, int bpp);
int set_text_mode(void);

int get_color_depth(void);
int get_color_bits(int *rbits, int *gbits, int *bbits);
int get_color_shift(int *rshift, int *gshift, int *bshift);
int get_color_mask(unsigned int *rmask, unsigned int *gmask, unsigned int *bmask);

void set_palette(int idx, int r, int g, int b);

enum {
	FLIP_NOW,
	FLIP_VBLANK,
	FLIP_VBLANK_WAIT
};
/* page flip and return pointer to the start of the display area (front buffer) */
void *page_flip(int vsync);

#ifdef __WATCOMC__
void wait_vsync(void);
#pragma aux wait_vsync = \
	"mov dx, 0x3da" \
	"l1:" \
	"in al, dx" \
	"and al, 0x8" \
	"jnz l1" \
	"l2:" \
	"in al, dx" \
	"and al, 0x8" \
	"jz l2" \
	modify[al dx];
#endif

#ifdef __DJGPP__
#define wait_vsync()  asm volatile ( \
	"mov $0x3da, %%dx\n\t" \
	"0:\n\t" \
	"in %%dx, %%al\n\t" \
	"and $8, %%al\n\t" \
	"jnz 0b\n\t" \
	"0:\n\t" \
	"in %%dx, %%al\n\t" \
	"and $8, %%al\n\t" \
	"jz 0b\n\t" \
	:::"%eax","%edx")
#endif


#ifdef __cplusplus
}
#endif

#endif	/* GFX_H_ */
