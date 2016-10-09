#ifndef POLYCLIP_H_
#define POLYCLIP_H_

#include "3dgfx.h"

struct cplane {
	float x, y, z;
	float nx, ny, nz;
};

/* Polygon clipper
 * returns:
 *  1 -> fully inside, not clipped
 *  0 -> straddling the plane and clipped
 * -1 -> fully outside, not clipped
 * in all cases, vertices are copied to vout, and the vertex count is written
 * to wherever voutnum is pointing
 */
int clip_poly(struct g3d_vertex *vout, int *voutnum,
		const struct g3d_vertex *vin, int vnum, struct cplane *plane);

#endif	/* POLYCLIP_H_ */
