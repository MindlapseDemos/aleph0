#ifndef POLYCLIP_H_
#define POLYCLIP_H_

#include "3dgfx.h"

struct cplane {
	float x, y, z;
	float nx, ny, nz;
};

enum {
	CLIP_LEFT, CLIP_RIGHT,
	CLIP_BOTTOM, CLIP_TOP,
	CLIP_NEAR, CLIP_FAR
};

/* Generic polygon clipper
 * returns:
 *  1 -> fully inside, not clipped
 *  0 -> straddling the plane and clipped
 * -1 -> fully outside, not clipped
 * in all cases, vertices are copied to vout, and the vertex count is written
 * to wherever voutnum is pointing
 */
int clip_poly(struct g3d_vertex *vout, int *voutnum,
		const struct g3d_vertex *vin, int vnum, struct cplane *plane);

/* Special-case frustum clipper (might be slightly faster) */
int clip_frustum(struct g3d_vertex *vout, int *voutnum,
		const struct g3d_vertex *vin, int vnum, int fplane);

#endif	/* POLYCLIP_H_ */
