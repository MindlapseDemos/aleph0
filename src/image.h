#ifndef IMAGE_H_
#define IMAGE_H_

#include "inttypes.h"

#define IMG_NOANIM

struct image {
	int width, height, scanlen;
	uint16_t *pixels;
	unsigned char *alpha;

	unsigned int xmask, ymask, xshift;

#ifndef IMG_NOANIM
	/* optional animation data */
	float cur_dur, frame_interval;
	int cur_frame, num_frames;
	float *uoffs, *voffs;
#endif
};

struct image_rgba {
	int width, height, scanlen;
	uint32_t *pixels;

	unsigned int xmask, ymask, xshift;
};

int load_image(struct image *img, const char *fname);
int save_image(struct image *img, const char *fname);
int dump_image(struct image *img, const char *fname);
void destroy_image(struct image *img);

int load_cubemap(struct image *cube, const char *fname_fmt);
void destroy_cubemap(struct image *cube);

/* scanlen == 0 means tightly packed (scanlen = width) */
void init_image(struct image *img, int x, int y, uint16_t *pixels, int scanlen);

void premul_alpha(struct image *img);

int conv_rle(struct image *img, uint16_t ckey);
void blitfb_rle(uint16_t *fb, int x, int y, struct image *img);

int conv_rle_alpha(struct image *img);
void blendfb_rle(uint16_t *fb, int x, int y, struct image *img);

#endif	/* IMAGE_H_ */
