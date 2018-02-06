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

void wait_vsync(void);

#ifdef __cplusplus
}
#endif

#endif	/* GFX_H_ */
