#ifndef GFXUTIL_H_
#define GFXUTIL_H_

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax);
void draw_line(int x0, int y0, int x1, int y1, unsigned short color);

#endif	/* GFXUTIL_H_ */
