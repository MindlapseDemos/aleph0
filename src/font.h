#ifndef FONT_H_
#define FONT_H_

#include "image.h"

struct glyph {
	struct image img;
	int cidx;
	int orgx, orgy;
	int adv;
};

struct font {
	struct glyph *glyphs;
	int size, advance, baseline;
	int num_glyphs;
	int gmin, gmax;
};

int load_font(struct font *fnt, const char *fname, uint16_t color);
void destroy_font(struct font *fnt);

void draw_text(struct font *fnt, uint16_t *fb, int x, int y, const char *str);
void draw_text_img(struct font *fnt, struct image *img, int x, int y, const char *str);

int calc_text_len(struct font *fnt, const char *str);

#endif	/* FONT_H_ */
