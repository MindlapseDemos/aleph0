#ifndef GFXUTIL_H_
#define GFXUTIL_H_

#include "inttypes.h"
#include "image.h"

#define PACK_RGB16(r, g, b) \
	((((uint16_t)(r) << 8) & 0xf800) | \
	 (((uint16_t)(g) << 3) & 0x7e0) | \
	 (((uint16_t)(b) >> 3) & 0x1f))

#define UNPACK_R16(c)	(((c) >> 8) & 0xf8)
#define UNPACK_G16(c)	(((c) >> 3) & 0xfc)
#define UNPACK_B16(c)	(((c) << 3) & 0xf8)


#ifdef BUILD_BIGENDIAN

#define PACK_RGB32(r, g, b) \
	((((b) & 0xff) << 16) | (((g) & 0xff) << 8) | ((r) & 0xff) | 0xff000000)

#define UNPACK_B32(c)	(((c) >> 16) & 0xff)
#define UNPACK_G32(c)	(((c) >> 8) & 0xff)
#define UNPACK_R32(c)	((c) & 0xff)

#else	/* LITTLE_ENDIAN */

#define PACK_RGB32(r, g, b) \
	((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff) | 0xff000000)

#define UNPACK_R32(c)	(((c) >> 16) & 0xff)
#define UNPACK_G32(c)	(((c) >> 8) & 0xff)
#define UNPACK_B32(c)	((c) & 0xff)
#endif

void init_gfxutil(void);

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax);
void draw_line(int x0, int y0, int x1, int y1, unsigned short color);

/* scale in 24.8 fixed point */
void blur_horiz(uint16_t *dest, uint16_t *src, int xsz, int ysz, int rad, int scale);
void blur_vert(uint16_t *dest, uint16_t *src, int xsz, int ysz, int rad, int scale);
void blur_xyzzy_horiz8(uint8_t *dest, uint8_t *src);
void blur_xyzzy_vert8(uint8_t *dest, uint8_t *src);

void convimg_rgb24_rgb16(uint16_t *dest, unsigned char *src, int xsz, int ysz);

void blitfb(uint16_t *dest, uint16_t *src, int width, int height, int pitch_pix);
void blit(uint16_t *dest, int destwidth, uint16_t *src, int xsz, int ysz, int pitch_pix);
void blit_key(uint16_t *dest, int destwidth, uint16_t *src, int xsz, int ysz, int pitch_pix, uint16_t key);

void overlay_add_full(uint16_t *dest, uint16_t *src);

/* the palette is 256 * 4 ints to avoid unpacking and conversions */
void overlay_add_pal(uint16_t *dest, uint8_t *src, int xsz, int ysz, int pitch_pix, unsigned int *pal);
void overlay_full_add_pal(uint16_t *dest, uint8_t *src, unsigned int *pal);

extern void (*overlay_alpha)(struct image *dest, int x, int y, const struct image *src,
		int width, int height);

void draw_billboard(float x, float y, float z, float size, int r, int g, int b, int a);

#endif	/* GFXUTIL_H_ */
