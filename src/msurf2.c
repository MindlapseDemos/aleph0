#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msurf2.h"
#include "mcubes.h"

#define NORMALIZE_GRAD
#undef NORMALIZE_NORMAL

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

void msurf_begin(struct msurf_volume *vol)
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
			abort();
		}
		if(!(vol->cells = malloc(vol->num_store * sizeof *vol->cells))) {
			fprintf(stderr, "failed to allocate volume cells\n");
			abort();
		}

		cell = vol->cells;
		for(z=0; z<vol->zres; z++) {
			for(y=0; y<vol->yres; y++) {
				for(x=0; x<vol->xres; x++) {
					cell->vol = vol;
					cell->x = x;
					cell->y = y;
					cell->z = z;
					for(i=0; i<8; i++) {
						vx = x + celloffs[i][0];
						vy = y + celloffs[i][1];
						vz = z + celloffs[i][2];
						cell->vox[i] = vol->voxels + msurf_addr(vol, vx, vy, vz);
					}
					cell->flags = 0;
					cell++;
				}
				cell += vol->xstore - vol->xres;
			}
			cell += vol->ystore - vol->yres;
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

	vol->cur++;
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

void msurf_proc_cell(struct msurf_volume *vol, struct msurf_cell *cell)
{
	int i, j, x, y, z, p0, p1;
	float t;
	unsigned int code;
	struct g3d_vertex vert[12];
	struct msurf_voxel *vox0, *vox1;
	cgm_vec3 norm;

	static const int pidx[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
		{6, 7},	{7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	/* calulcate the marching cubes bit code */
	code = 0;
	for(i=0; i<8; i++) {
		if(cell->vox[i]->val > vol->isoval) {
			code |= 1 << i;
		}
	}

	if((code | ~code) == 0) return;

	/* for each of the voxels, make sure we have valid gradients */
	for(i=0; i<8; i++) {
		if((cell->vox[i]->flags & 0xf) != vol->cur) {
			x = cell->x + celloffs[i][0];
			y = cell->y + celloffs[i][1];
			z = cell->z + celloffs[i][2];
			calc_grad(vol, x, y, z, &cell->vox[i]->grad);
			cell->vox[i]->flags = (cell->vox[i]->flags & ~0xf) | (vol->cur & 0xf);
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
}

#if 0
void msurf_polygonize(struct msurf_volume *vol)
{
	int i, cx, cy, cz;
	struct msurf_cell *cell;

	for(i=0; i<vol->num_mballs; i++) {
		/* start from the center of each metaball and go outwards until we hit
		 * the isosurface, then follow it
		 */
		msurf_pos_to_cell(vol, vol->mballs[i].pos, &cx, &cy, &cz);
		cell = vol->cells + msurf_addr(vol, cx, cy, cz);
	}
}
#endif
