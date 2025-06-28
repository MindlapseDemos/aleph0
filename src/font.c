#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "darray.h"
#include "font.h"
#include "gfxutil.h"
#include "image.h"

static int mkglyph(struct image *dest, struct image *src, int x, int y, int w, int h, uint16_t color)
{
	unsigned int i, j, offs;
	uint16_t *rgb, *sptr;
	unsigned char *alpha;

	if(!(rgb = malloc(w * h * 2)) || !(alpha = malloc(w * h))) {
		fprintf(stderr, "failed to allocate glyph image (%dx%d)\n", w, h);
		free(rgb);
		return -1;
	}

	init_image(dest, w, h, rgb, w);
	dest->alpha = alpha;

	sptr = src->pixels + y * src->scanlen + x;
	for(i=0; i<h; i++) {
		for(j=0; j<w; j++) {
			alpha[j] = UNPACK_R16(sptr[j]);
			rgb[j] = color;
		}
		alpha += w;
		rgb += w;
		sptr += src->scanlen;
	}

	premul_alpha(dest);
	conv_rle_alpha(dest);
	return 0;
}

int load_font(struct font *fnt, const char *fname, uint16_t color)
{
	int hdrline = 0, val, gx, gy, gwidth, gheight;
	FILE *fp;
	char buf[128];
	char *line;
	struct image img;
	struct glyph g;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to load font: %s\n", fname);
		return -1;
	}
	if(load_image(&img, fname) == -1) {
		fprintf(stderr, "failed to load font image: %s\n", fname);
		fclose(fp);
		return -1;
	}
	fnt->glyphs = darr_alloc(0, sizeof *fnt->glyphs);

	fnt->gmin = INT_MAX;
	fnt->gmax = 0;

	while(hdrline < 3 && fgets(buf, sizeof buf, fp)) {
		line = buf;
		while(*line && isspace(*line)) line++;

		if(*line != '#') {
			hdrline++;
			continue;
		}
		line++;
		while(*line && isspace(*line)) line++;
		if(!*line) continue;

		if(sscanf(line, "size: %d", &val) == 1) {
			fnt->size = val;
		} else if(sscanf(line, "advance: %d", &val) == 1) {
			fnt->advance = val;
		} else if(sscanf(line, "baseline: %d", &val) == 1) {
			fnt->baseline = val;
		} else if(sscanf(line, "%d: %dx%d%d%d o:%d,%d adv:%d", &g.cidx, &gwidth,
					&gheight, &gx, &gy, &g.orgx, &g.orgy, &g.adv) == 8) {
			mkglyph(&g.img, &img, gx, gy, gwidth, gheight, color);
			darr_push(fnt->glyphs, &g);

			if(g.cidx < fnt->gmin) fnt->gmin = g.cidx;
			if(g.cidx > fnt->gmax) fnt->gmax = g.cidx;
		}
	}

	fnt->num_glyphs = darr_size(fnt->glyphs);
	fclose(fp);
	destroy_image(&img);
	return 0;

}

void destroy_font(struct font *fnt)
{
	int i;

	if(!fnt) return;

	for(i=0; i<fnt->num_glyphs; i++) {
		destroy_image(&fnt->glyphs[i].img);
	}
	darr_free(fnt->glyphs);
}

void draw_text(struct font *fnt, uint16_t *fb, int x, int y, const char *str)
{
	int c;
	struct glyph *g;

	while((c = *str++)) {
		if(c < fnt->gmin || c > fnt->gmax) {
			if(c == ' ') x += fnt->glyphs['I' - fnt->gmin].adv << 1;
			continue;
		}
		g = fnt->glyphs + c - fnt->gmin;
		blendfb_rle(fb, x, y, &g->img);
		x += g->adv;
	}
}


void draw_text_img(struct font *fnt, struct image *img, int x, int y, const char *str)
{
	int c;
	struct glyph *g;

	while((c = *str++)) {
		if(c < fnt->gmin || c > fnt->gmax) {
			if(c == ' ') x += fnt->glyphs['I' - fnt->gmin].adv << 1;
			continue;
		}
		g = fnt->glyphs + c - fnt->gmin;
		blend_rle(img, x, y, &g->img);
		x += g->adv;
	}
}

int calc_text_len(struct font *fnt, const char *str)
{
	int c, x = 0;
	struct glyph *g;

	while((c = *str++)) {
		if(c < fnt->gmin || c > fnt->gmax) {
			if(c == ' ') x += fnt->glyphs['I' - fnt->gmin].adv << 1;
			continue;
		}
		g = fnt->glyphs + c - fnt->gmin;
		x += g->adv;
	}

	return x;
}
