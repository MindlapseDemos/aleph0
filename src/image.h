#ifndef IMAGE_H_
#define IMAGE_H_

#include "inttypes.h"

struct image {
	int width, height, scanlen;
	uint16_t *pixels;
	unsigned char *alpha;

	unsigned int xmask, ymask, xshift;

	/* optional animation data */
	float cur_dur, frame_interval;
	int cur_frame, num_frames;
	float *uoffs, *voffs;
};

int load_image(struct image *img, const char *fname);
int load_image_alpha(struct image *img, const char *fname);
int dump_image(struct image *img, const char *fname);
void destroy_image(struct image *img);

int load_cubemap(struct image *cube, const char *fname_fmt);
void destroy_cubemap(struct image *cube);

/* scanlen == 0 means tightly packed (scanlen = width) */
void init_image(struct image *img, int x, int y, uint16_t *pixels, int scanlen);

#endif	/* IMAGE_H_ */
