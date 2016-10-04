#ifndef GFXUTIL_H_
#define GFXUTIL_H_

#define PACK_RGB16(r, g, b) \
	(((((r) >> 3) & 0x1f) << 11) | ((((g) >> 2) & 0x3f) << 5) | (((b) >> 3) & 0x1f))

#define UNPACK_R16(c)	(((c) >> 8) & 0x7c)
#define UNPACK_G16(c)	(((c) >> 3) & 0xfc)
#define UNPACK_B16(c)	(((c) << 3) & 0x7c)

#define PACK_RGB32(r, g, b) \
	((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax);
void draw_line(int x0, int y0, int x1, int y1, unsigned short color);

#endif	/* GFXUTIL_H_ */
