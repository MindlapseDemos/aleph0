#ifndef SMOKE_TEXT_H_
#define SMOKE_TEXT_H_

#include "3dgfx.h"
#include "cgmath/cgmath.h"


struct particle {
	float x, y, z;
	float vx, vy, vz;	/* velocity */
	int r, g, b;
	float life;
};

struct vfield {
	cgm_vec2 pos, size;

	int width, height;
	int xshift;
	cgm_vec2 *v;
};

struct emitter {
	struct particle *plist;
	int pcount;

	cgm_vec3 wind;
	float drag;
	float max_life;

	struct g3d_vertex *varr;
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

void set_smktxt_wind(struct smktxt *stx, float x, float y, float z);
void set_smktxt_plife(struct smktxt *stx, float life);
void set_smktxt_pcount(struct smktxt *stx, int count);
void set_smktxt_drag(struct smktxt *stx, float drag);

/* assume constant 30fps update dt */
void update_smktxt(struct smktxt *stx);
void draw_smktxt(struct smktxt *stx);


#endif	/* SMOKE_TEXT_ */
