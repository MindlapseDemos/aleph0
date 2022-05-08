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

static cgm_vec3 raydir[120][160];
static struct rtscene scn;

struct screen *raytrace_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j;
	float z = 1.0f / tan(cgm_deg_to_rad(25.0f));

	for(i=0; i<120; i++) {
		cgm_vec3 *vptr = raydir[i];
		float y = 1.0f - (float)i / 60.0f;
		for(j=0; j<160; j++) {
			vptr->x = ((float)j / 80.0f - 1.0f) * 1.333333f;
			vptr->y = y;
			vptr->z = z;
			vptr++;
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

static void draw(void)
{
	int i, j, r, g, b;
	uint16_t pix, *fbptr = fb_pixels;

	for(i=0; i<120; i++) {
		for(j=0; j<160; j++) {
			cgm_ray ray;
			cgm_vec3 col;
			ray.dir = raydir[i][j];
			cgm_vcons(&ray.origin, 0, 0, -5);

			if(ray_trace(&ray, &scn, 0, &col)) {
				r = cround64(col.x * 255.0f) & 0xff;
				g = cround64(col.y * 255.0f) & 0xff;
				b = cround64(col.z * 255.0f) & 0xff;
				if(r > 255) r = 255;
				if(g > 255) g = 255;
				if(b > 255) b = 255;
				pix = PACK_RGB16(r, g, b);
			} else {
				pix = 0;
			}
			fbptr[0] = fbptr[1] = fbptr[320] = fbptr[321] = pix;
			fbptr += 2;
		}
		fbptr += 320;
	}

	swap_buffers(0);
}
