#ifndef MSURF2_H_
#define MSURF2_H_

#include "cgmath/cgmath.h"
#include "3dgfx.h"
#include "util.h"

enum {
	MSURF_VALID		= 0x100,
	MSURF_POSVALID	= 0x200,
	MSURF_GRADVALID	= 0x400,
	MSURF_FLOOR		= 0x800
};

struct msurf_volume;

struct msurf_voxel {
	float val;
	cgm_vec3 pos;
	cgm_vec3 grad;
	unsigned int flags;
};

struct msurf_cell {
	struct msurf_volume *vol;
	unsigned int x, y, z;
	struct msurf_voxel *vox[8];
	unsigned int flags;
	struct msurf_cell *next;
};

struct metaball {
	float energy;
	cgm_vec3 pos;
};

struct msurf_volume {
	unsigned int xres, yres, zres;	/* useful X,Y,Z volume resolution */
	unsigned int xstore, ystore, xystore;	/* actual storage size (always pow2) */
	unsigned int xshift, yshift, xyshift;	/* shifts to access rows/slices */
	unsigned int num_store;		/* total storage count (xstore*ystore*zres) */

	struct msurf_voxel *voxels;		/* voxels array */
	cgm_vec3 size, rad;				/* size and half-size (radius) of volume */
	float dx, dy, dz;				/* step between voxels (cell size) */

	struct msurf_cell *cells;		/* cells array (space between 8 voxels) */

	float isoval;					/* isosurface value */
	unsigned int flags;

	struct g3d_vertex *varr;		/* isosurface mesh */
	unsigned int num_verts, max_verts;

	struct metaball *mballs;		/* metaballs */
	unsigned int num_mballs;

	float floor_z, floor_energy;

	int cur;
};

#define msurf_addr(ms, x, y, z) \
	(((z) << (ms)->xyshift) + ((y) << (ms)->xshift) + (x))


int msurf_init(struct msurf_volume *vol);
void msurf_destroy(struct msurf_volume *vol);

void msurf_resolution(struct msurf_volume *vol, int x, int y, int z);
void msurf_size(struct msurf_volume *vol, float x, float y, float z);
int msurf_metaballs(struct msurf_volume *vol, int count);

int msurf_begin(struct msurf_volume *vol);
int msurf_proc_cell(struct msurf_volume *vol, struct msurf_cell *cell);
void msurf_genmesh(struct msurf_volume *vol);

static INLINE void msurf_pos_to_cell(struct msurf_volume *vol, cgm_vec3 pos,
		int *cx, int *cy, int *cz)
{
	int x = cround64(pos.x * vol->xres / vol->size.x);
	int y = cround64(pos.y * vol->yres / vol->size.y);
	int z = cround64(pos.z * vol->zres / vol->size.z);
	*cx = x < 0 ? 0 : (x >= vol->xres ? vol->xres - 1 : x);
	*cy = y < 0 ? 0 : (y >= vol->yres ? vol->yres - 1 : y);
	*cz = z < 0 ? 0 : (z >= vol->zres ? vol->zres - 1 : z);
}

static INLINE void msurf_cell_to_pos(struct msurf_volume *vol, int cx, int cy,
		int cz, cgm_vec3 *pos)
{
	pos->x = (float)cx * vol->size.x / (float)vol->xres;
	pos->y = (float)cy * vol->size.y / (float)vol->yres;
	pos->z = (float)cz * vol->size.z / (float)vol->zres;
}

#endif	/* MSURF2_H_ */
