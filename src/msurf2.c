#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cgmath/cgmath.h"
#include "msurf2.h"
#include "mcubes.h"

#define NORMALIZE_GRAD
#undef NORMALIZE_NORMAL

#define CELL_FRAME_BITS		0x00ff
#define CELL_CODE_BITS		0xff00

int dbg_visited;
static unsigned int frmid;

int msurf_init(struct msurf_volume *vol)
{
	memset(vol, 0, sizeof *vol);
	return 0;
}

void msurf_destroy(struct msurf_volume *vol)
{
	free(vol->voxels);
	free(vol->cells);
	free(vol->varr);
	free(vol->mballs);
}

void msurf_resolution(struct msurf_volume *vol, int x, int y, int z)
{
	if(x == vol->xres && y == vol->yres && z == vol->zres) {
		return;
	}
	vol->xres = x;
	vol->yres = y;
	vol->zres = z;
	vol->flags &= ~(MSURF_VALID | MSURF_POSVALID);
}

void msurf_size(struct msurf_volume *vol, float x, float y, float z)
{
	vol->size.x = x;
	vol->size.y = y;
	vol->size.z = z;
	vol->rad.x = x * 0.5f;
	vol->rad.y = y * 0.5f;
	vol->rad.z = z * 0.5f;
	vol->flags &= ~MSURF_POSVALID;
}

int msurf_metaballs(struct msurf_volume *vol, int count)
{
	struct metaball *mballs;
	if(!(mballs = malloc(count * sizeof *mballs))) {
		fprintf(stderr, "failed to allocate %d metaballs\n", count);
		return -1;
	}
	free(vol->mballs);
	vol->mballs = mballs;
	vol->num_mballs = count;
	return 0;
}

static const int celloffs[][3] = {
	{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
	{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
};

int msurf_begin(struct msurf_volume *vol)
{
	int i, x, y, z, vx, vy, vz;
	struct msurf_cell *cell;
	struct msurf_voxel *vox;

	if(!(vol->flags & MSURF_VALID)) {
		vol->xstore = next_pow2(vol->xres);
		vol->ystore = next_pow2(vol->yres);
		vol->xystore = vol->xstore * vol->ystore;
		vol->xshift = calc_shift(vol->xstore);
		vol->yshift = calc_shift(vol->ystore);
		vol->xyshift = vol->xshift + vol->yshift;
		vol->num_store = vol->xstore * vol->ystore * vol->zres;

		free(vol->voxels);
		free(vol->cells);

		if(!(vol->voxels = calloc(vol->num_store, sizeof *vol->voxels))) {
			fprintf(stderr, "failed to allocate voxels\n");
			return -1;
		}
		if(!(vol->cells = malloc(vol->num_store * sizeof *vol->cells))) {
			fprintf(stderr, "failed to allocate volume cells\n");
			free(vol->voxels);
			return -1;
		}

		cell = vol->cells;
		for(z=0; z<vol->zres; z++) {
			for(y=0; y<vol->ystore; y++) {
				for(x=0; x<vol->xstore; x++) {
					/* vol==0 for padding cells not part of the volume */
					if(x >= vol->xres - 1 || y >= vol->yres - 1 || z >= vol->zres - 1) {
						memset(cell, 0, sizeof *cell);
						cell->x = x;
						cell->y = y;
						cell->z = z;
					} else {
						cell->vol = vol;
						cell->x = x;
						cell->y = y;
						cell->z = z;
						for(i=0; i<8; i++) {
							vx = x + celloffs[i][0];
							vy = y + celloffs[i][1];
							vz = z + celloffs[i][2];
							cell->vox[i] = vol->voxels + msurf_addr(vol, vx, vy, vz);
							assert(cell->vox[i] >= vol->voxels);
							assert(cell->vox[i] < vol->voxels + vol->num_store);
						}
						cell->flags = 0;
					}
					cell++;
				}
			}
		}
	}

	if(!(vol->flags & MSURF_POSVALID)) {
		vox = vol->voxels;
		for(z=0; z<vol->zres; z++) {
			for(y=0; y<vol->yres; y++) {
				for(x=0; x<vol->xres; x++) {
					msurf_cell_to_pos(vol, x, y, z, &vox->pos);
					vox++;
				}
				vox += vol->xstore - vol->xres;
			}
			vox += vol->ystore - vol->yres;
		}
	}

	if(!(vol->flags & MSURF_VALID) || !(vol->flags & MSURF_POSVALID)) {
		vol->dx = vol->size.x / vol->xres;
		vol->dy = vol->size.y / vol->yres;
		vol->dz = vol->size.z / vol->zres;

		vol->flags |= MSURF_VALID | MSURF_POSVALID;
	}

	vol->num_verts = 0;
	vol->cur++;
	frmid = vol->cur & 0xff;
	dbg_visited = 0;
	return 0;
}

static void calc_grad(struct msurf_volume *vol, int x, int y, int z, cgm_vec3 *grad)
{
	struct msurf_voxel *ptr = vol->voxels + msurf_addr(vol, x, y, z);
	if(x < vol->xres - 1) {
		grad->x = ptr[1].val - ptr->val;
	} else {
		grad->x = ptr->val - ptr[-1].val;
	}
	if(y < vol->yres - 1) {
		grad->y = ptr[vol->xstore].val - ptr->val;
	} else {
		grad->y = ptr->val - ptr[-vol->xstore].val;
	}
	if(z < vol->zres - 1) {
		grad->z = ptr[vol->xystore].val - ptr->val;
	} else {
		grad->z = ptr->val - ptr[-vol->xystore].val;
	}
#ifdef NORMALIZE_GRAD
	fast_normalize(&grad->x);
#endif
}

int msurf_proc_cell(struct msurf_volume *vol, struct msurf_cell *cell)
{
	int i, j, x, y, z, p0, p1;
	float t, lensq;
	unsigned int code;
	struct g3d_vertex vert[12];
	struct msurf_voxel *vox, *vox0, *vox1;
	cgm_vec3 dir, norm;

	static const int pidx[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
		{6, 7},	{7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	/* update the metaball field if necessary */
	for(i=0; i<8; i++) {
		vox = cell->vox[i];
		if((vox->flags & 0xff00) != (frmid << 8)) {
			vox->val = 0;
			for(j=0; j<vol->num_mballs; j++) {
				dir = vol->mballs[j].pos;
				cgm_vsub(&dir, &vox->pos);
				lensq = cgm_vlength_sq(&dir);
				vox->val += lensq == 0.0f ? 1024.0f : vol->mballs[j].energy / lensq;
			}
			vox->flags = (vox->flags & ~0xff00) | (frmid << 8);
		}
	}

	/* calculcate the marching cubes bit code */
	code = 0;
	for(i=0; i<8; i++) {
		if(cell->vox[i]->val > vol->isoval) {
			code |= 1 << i;
		}
	}
	cell->flags = (code << 8) | frmid;

	if((code | ~code) == 0) return 0;

	/* for each of the voxels, make sure we have valid gradients */
	for(i=0; i<8; i++) {
		if((cell->vox[i]->flags & 0xff) != frmid) {
			x = cell->x + celloffs[i][0];
			y = cell->y + celloffs[i][1];
			z = cell->z + celloffs[i][2];
			calc_grad(vol, x, y, z, &cell->vox[i]->grad);
			cell->vox[i]->flags = (cell->vox[i]->flags & ~0xff) | frmid;
		}
	}

	/* generate up to max 12 verts per cube. interpolate positions and normals for each one */
	for(i=0; i<12; i++) {
		if(mc_edge_table[code] & (1 << i)) {
			p0 = pidx[i][0];
			p1 = pidx[i][1];
			vox0 = cell->vox[p0];
			vox1 = cell->vox[p1];

			t = (vol->isoval - vox0->val) / (vox1->val - vox0->val);
			vert[i].x = vox0->pos.x + (vox1->pos.x - vox0->pos.x) * t;
			vert[i].y = vox0->pos.y + (vox1->pos.y - vox0->pos.y) * t;
			vert[i].z = vox0->pos.z + (vox1->pos.z - vox0->pos.z) * t;
			vert[i].nx = vox0->grad.x + (vox1->grad.x - vox0->grad.x) * t;
			vert[i].ny = vox0->grad.y + (vox1->grad.y - vox0->grad.y) * t;
			vert[i].nz = vox0->grad.z + (vox1->grad.z - vox0->grad.z) * t;
			vert[i].r = vert[i].g = vert[i].b = vert[i].a = 0xff;
#ifdef NORMALIZE_NORMAL
			fast_normalize(&vert[i].nx);
#endif
		}
	}

	/* for each generated triangle, add its vertices to the vertex buffer */
	for(i=0; mc_tri_table[code][i] != -1; i+=3) {
		for(j=0; j<3; j++) {
			int idx = mc_tri_table[code][i + j];
			struct g3d_vertex *newv;

			if(vol->num_verts >= vol->max_verts) {
				int newsz = vol->max_verts ? vol->max_verts * 2 : 32;
				if(!(newv = realloc(vol->varr, newsz * sizeof *vol->varr))) {
					fprintf(stderr, "msurf2: failed to resize vertex array\n");
					abort();
				}
				vol->varr = newv;
				vol->max_verts = newsz;
			}

			vol->varr[vol->num_verts++] = vert[idx];
		}
	}

	return 1;
}

#define ADDOPEN(dir, c) \
	do { \
		struct msurf_cell *cp = (c); \
		if((dirvalid & dir) == dir && ((cp->flags & 0xff) != frmid)) { \
			cp->next = openlist; \
			openlist = cp; \
			cp->flags = (cp->flags & ~0xff) | frmid; \
		} \
	} while(0)

/* start from the center of each metaball and go outwards until we hit the
 * isosurface, then follow it
 */
void msurf_genmesh(struct msurf_volume *vol)
{
	int i, cx, cy, cz, foundsurf;
	unsigned int dirvalid;
	struct msurf_cell *cell, *cellptr, *openlist = 0;

	for(i=0; i<vol->num_mballs; i++) {
		msurf_pos_to_cell(vol, vol->mballs[i].pos, &cx, &cy, &cz);
		cell = vol->cells + msurf_addr(vol, cx, cy, cz);
		ADDOPEN(0, cell);

		foundsurf = 0;
		while(openlist) {
			cell = openlist;
			openlist = openlist->next;

			dbg_visited++;

			/* each bit is 1 if it has cells to the corresponding side */
			dirvalid = 0;
			if(cell->x > 0) dirvalid |= 001;
			if(cell->y > 0) dirvalid |= 002;
			if(cell->z > 0) dirvalid |= 004;
			if(cell->x < vol->xres - 2) dirvalid |= 010;
			if(cell->y < vol->yres - 2) dirvalid |= 020;
			if(cell->z < vol->zres - 2) dirvalid |= 040;

			/* examine the current cell, if it's on the surface expand the search to
			 * its neighbors, otherwise keep going towards the same direction if we
			 * haven't hit the surface, ignore it if we have.
			 */
			if(msurf_proc_cell(vol, cell)) {
				/* this is part of the surface, add all neighbors */
				foundsurf = 1;
				ADDOPEN(001, cell - 1);
				ADDOPEN(010, cell + 1);
				ADDOPEN(003, cell - vol->xstore - 1);
				ADDOPEN(002, cell - vol->xstore);
				ADDOPEN(012, cell - vol->xstore + 1);
				ADDOPEN(021, cell + vol->xstore - 1);
				ADDOPEN(020, cell + vol->xstore);
				ADDOPEN(030, cell + vol->xstore + 1);
				cellptr = cell - vol->xystore;
				ADDOPEN(005, cellptr - 1);
				ADDOPEN(004, cellptr);
				ADDOPEN(014, cellptr + 1);
				ADDOPEN(007, cellptr - vol->xstore - 1);
				ADDOPEN(006, cellptr - vol->xstore);
				ADDOPEN(016, cellptr - vol->xstore + 1);
				ADDOPEN(025, cellptr + vol->xstore - 1);
				ADDOPEN(024, cellptr + vol->xstore);
				ADDOPEN(034, cellptr + vol->xstore + 1);
				cellptr = cell + vol->xystore;
				ADDOPEN(041, cellptr - 1);
				ADDOPEN(040, cellptr);
				ADDOPEN(050, cellptr + 1);
				ADDOPEN(043, cellptr - vol->xstore - 1);
				ADDOPEN(042, cellptr - vol->xstore);
				ADDOPEN(052, cellptr - vol->xstore + 1);
				ADDOPEN(061, cellptr + vol->xstore - 1);
				ADDOPEN(060, cellptr + vol->xstore);
				ADDOPEN(070, cellptr + vol->xstore + 1);
			} else {
				/* not part of the surface, if we haven't found the surface yet
				 * expand along the Z axis, otherwise ignore
				 */
				if(!foundsurf) {
					ADDOPEN(004, cell - vol->xystore);
					ADDOPEN(040, cell + vol->xystore);
				}
			}
		}
	}

}
