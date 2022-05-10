#include <stdio.h>
#include <math.h>
#include "demo.h"
#include "screen.h"
#include "gfxutil.h"
#include "util.h"
#include "cgmath/cgmath.h"
#include "rt.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"raytrace",
	init,
	destroy,
	start,
	0,
	draw
};

enum {LASTX = 1, LASTY = 2};

struct tile {
	int sz;
	unsigned int flags;
	struct { int x, y; } cpos[4];	/* corner coordinates */
	uint16_t *cptr[4];	/* corner pixels */
};

#define TILESZ		8
#define NUM_TILES	((320 / TILESZ) * (240 / TILESZ))

static cgm_vec3 raydir[240][320];
static struct tile tiles[NUM_TILES];
static struct rtscene scn;

struct screen *raytrace_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, k;
	float z = 1.0f / tan(cgm_deg_to_rad(25.0f));
	struct tile *tptr = tiles;

	for(i=0; i<240; i++) {
		cgm_vec3 *vptr = raydir[i];
		float y = 1.0f - (float)i / 120.0f;
		for(j=0; j<320; j++) {
			vptr->x = ((float)j / 160.0f - 1.0f) * 1.333333f;
			vptr->y = y;
			vptr->z = z;
			vptr++;

			if(((j & (TILESZ-1)) | (i & (TILESZ-1))) == 0) {
				tptr->sz = TILESZ;
				tptr->flags = 0;
				if(j + TILESZ >= 320) tptr->flags |= LASTX;
				if(i + TILESZ >= 240) tptr->flags |= LASTY;

				tptr->cpos[0].x = j;
				tptr->cpos[0].y = i;
				tptr->cpos[1].x = j + (tptr->flags & LASTX ? TILESZ - 1 : TILESZ);
				tptr->cpos[1].y = i;
				tptr->cpos[2].x = j;
				tptr->cpos[2].y = i + (tptr->flags & LASTY ? TILESZ - 1 : TILESZ);
				tptr->cpos[3].x = tptr->cpos[1].x;
				tptr->cpos[3].y = tptr->cpos[2].y;

				for(k=0; k<4; k++) {
					tptr->cptr[k] = fb_pixels + tptr->cpos[k].y * 320 + tptr->cpos[k].x;
				}
				tptr++;
			}
		}
	}

	rt_init(&scn);

	rt_color(1, 0, 0);
	rt_specular(0.8f, 0.8f, 0.8f);
	rt_shininess(30.0f);
	rt_add_sphere(&scn, 0, 0, 0, 1);	/* x,y,z, rad */

	rt_color(0.4, 0.4, 0.4);
	rt_specular(0, 0, 0);
	rt_shininess(1);
	rt_add_plane(&scn, 0, 1, 0, -1);		/* nx,ny,nz, dist */

	rt_color(1, 1, 1);
	rt_add_light(&scn, -8, 15, -10);
	return 0;
}

static void destroy(void)
{
	rt_destroy(&scn);
}

static void start(long start_time)
{
}

static uint16_t INLINE rend_pixel(int x, int y)
{
	int r, g, b;
	cgm_ray ray;
	cgm_vec3 col;

	ray.dir = raydir[y][x];
	cgm_vcons(&ray.origin, 0, 0, -5);

	if(ray_trace(&ray, &scn, 0, &col)) {
		r = cround64(col.x * 255.0f);
		g = cround64(col.y * 255.0f);
		b = cround64(col.z * 255.0f);
		if(r > 255) r = 255;
		if(g > 255) g = 255;
		if(b > 255) b = 255;
		return PACK_RGB16(r, g, b);
	}
	return 0;
}

#define FBPTR(x, y)	(fb_pixels + ((y) << 8) + ((y) << 6) + (x))

static void draw(void)
{
	int i, j, xbound, ybound;
	uint16_t *fbptr;
	struct tile *tptr;

	tptr = tiles;
	for(i=0; i<NUM_TILES; i++) {
		*tptr->cptr[0] = rend_pixel(tptr->cpos[0].x, tptr->cpos[0].y);
		if(tptr->flags & LASTX) {
			*tptr->cptr[1] = rend_pixel(tptr->cpos[1].x, tptr->cpos[1].y);
			if(tptr->flags & LASTY) {
				*tptr->cptr[3] = rend_pixel(tptr->cpos[3].x, tptr->cpos[3].y);
			}
		}
		if(tptr->flags & LASTY) {
			*tptr->cptr[2] = rend_pixel(tptr->cpos[2].x, tptr->cpos[2].y);
		}
		tptr++;
	}

	swap_buffers(0);
}
