#ifndef SMOKE_TEXT_H_
#define SMOKE_TEXT_H_

#include "szint.h"

struct ivec2 { int32_t x, y; };


struct particle {
	int32_t x, y;
	int32_t vx, vy;	/* velocity */
	int r, g, b;
	int32_t life;
};

struct vfield {
	struct ivec2 pos, size;

	int width, height;
	int xshift;
	struct ivec2 *v;
};

struct emitter {
	struct particle *plist;
	int pcount;

	struct ivec2 wind;
	int32_t drag;
	int32_t max_life;

	struct ivec2 *varr;
};

struct smktxt {
	struct emitter em;
	struct vfield vfield;

	unsigned char *img_pixels;
	int img_xsz, img_ysz;
};


struct smktxt *create_smktxt(const char *imgname, const char *vfieldname);
void destroy_smktxt(struct smktxt *stx);

int gen_smktxt_vfield(struct smktxt *stx, int xres, int yres, float xfreq, float yfreq);
int dump_smktxt_vfield(struct smktxt *stx, const char *fname);

void set_smktxt_wind(struct smktxt *stx, int32_t x, int32_t y);
void set_smktxt_plife(struct smktxt *stx, int32_t life);
void set_smktxt_pcount(struct smktxt *stx, int count);
void set_smktxt_drag(struct smktxt *stx, int32_t drag);

/* assume constant 30fps update dt */
void update_smktxt(struct smktxt *stx);
void draw_smktxt(struct smktxt *stx);


#endif	/* SMOKE_TEXT_ */
