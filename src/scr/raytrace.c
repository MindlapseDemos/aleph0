#include <stdio.h>
#include <math.h>
#include "demo.h"
#include "screen.h"
#include "gfxutil.h"
#include "util.h"
#include "cgmath/cgmath.h"
#include "rt.h"

#undef FULLRES

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

struct tile {
	int x, y;
	uint16_t *fbptr;
};

#define TILESZ		16
#define NUM_TILES	((320 / TILESZ) * (240 / TILESZ))

static cgm_vec3 raydir[240][320];
static struct tile tiles[NUM_TILES];
static struct rtscene scn;

static float cam_theta = 0, cam_phi = 0;
static float cam_dist = 5;
static float cam_xform[16];

static struct rtsphere *subsph;

struct screen *raytrace_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, k;
	float z = 1.0f / tan(cgm_deg_to_rad(25.0f));
	struct tile *tptr = tiles;
	union rtobject *obja, *objb;

	for(i=0; i<240; i++) {
		cgm_vec3 *vptr = raydir[i];
		float y = 1.0f - (float)i / 120.0f;
		for(j=0; j<320; j++) {
			vptr->x = ((float)j / 160.0f - 1.0f) * 1.333333f;
			vptr->y = y;
			vptr->z = z;
			vptr++;

			if(((j & (TILESZ-1)) | (i & (TILESZ-1))) == 0) {
				tptr->x = j;
				tptr->y = i;
				tptr->fbptr = fb_pixels + i * 320 + j;
				tptr++;
			}
		}
	}

	rt_init(&scn);
	rt_ambient(0.15, 0.15, 0.15);

	rt_color(1, 0, 0);
	rt_specular(0.8f, 0.8f, 0.8f);
	rt_shininess(30.0f);
	obja = rt_add_sphere(&scn, 0, 0, 0, 1);	/* x,y,z, rad */

	rt_color(0.2, 0.4, 1);
	objb = rt_add_sphere(&scn, 0, 0, 0, 0.7);
	subsph = &objb->s;

	rt_add_csg(&scn, RT_DIFF, obja, objb);

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
	ray.origin.x = ray.origin.y = ray.origin.z = 0.0f;

	cgm_rmul_mr(&ray, cam_xform);

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

#define CMPMASK		0xe79c
static void rend_tile(uint16_t *fbptr, int x0, int y0, int tsz, int valid)
{
	uint16_t *cptr[4];
	uint16_t cpix[4], tmp;
	uint32_t pp0, pp1, pp2, pp3, *fb32;
	int i, x1, y1, offs;

	fb32 = (uint32_t*)fbptr;

#ifdef FULLRES
	if(tsz <= 1) return;
#else
	if(tsz <= 2) {
		switch(valid) {
		case 0:
			fbptr[1] = fbptr[320] = fbptr[321] = *fbptr;
			break;
		case 1:
			fbptr[0] = fbptr[320] = fbptr[321] = fbptr[1];
			break;
		case 2:
			fbptr[0] = fbptr[1] = fbptr[321] = fbptr[320];
			break;
		case 3:
			fbptr[0] = fbptr[1] = fbptr[320] = fbptr[321];
			break;
		default:
			printf("valid = %d\n", valid);
			fbptr[0] = fbptr[1] = fbptr[320] = fbptr[321] = 0xff00;
		}
		return;
	}
#endif

	offs = tsz - 1;
	x1 = x0 + offs;
	y1 = y0 + offs;

	cptr[0] = fbptr;
	cptr[1] = fbptr + tsz - 1;
	cptr[2] = fbptr + (offs << 8) + (offs << 6);
	cptr[3] = cptr[2] + tsz - 1;

	cpix[0] = valid == 0 ? *cptr[0] : rend_pixel(x0, y0);
	cpix[1] = valid == 1 ? *cptr[1] : rend_pixel(x1, y0);
	cpix[2] = valid == 2 ? *cptr[2] : rend_pixel(x0, y1);
	cpix[3] = valid == 3 ? *cptr[3] : rend_pixel(x1, y1);

	tmp = cpix[0] & CMPMASK;
	if((cpix[1] & CMPMASK) != tmp) goto subdiv;
	if((cpix[2] & CMPMASK) != tmp) goto subdiv;
	if((cpix[3] & CMPMASK) != tmp) goto subdiv;

	pp0 = cpix[0] | ((uint32_t)cpix[0] << 16);
	pp1 = cpix[1] | ((uint32_t)cpix[1] << 16);
	pp2 = cpix[2] | ((uint32_t)cpix[2] << 16);
	pp3 = cpix[3] | ((uint32_t)cpix[3] << 16);

	switch(tsz) {
	case 2:
#ifdef SUBDBG
		pp0 = 0x18ff;
#endif
		fb32[0] = fb32[160] = pp0;
		break;
	case 4:
#ifdef SUBDBG
		pp0 = pp1 = pp2 = pp3 = 0x03800380;
#endif
		fb32[0] = fb32[160] = pp0;
		fb32[1] = fb32[161] = pp1;
		fb32[320] = fb32[480] = pp2;
		fb32[321] = fb32[481] = pp3;
		break;
	case 8:
#ifdef SUBDBG
		pp1 = pp0 = pp2 = pp3 = 0xe00fe00f;
#endif
		fb32[0] = fb32[1] = pp0; fb32[2] = fb32[3] = pp1;
		fb32[160] = fb32[161] = pp0; fb32[162] = fb32[163] = pp1;
		fb32[320] = fb32[321] = pp0; fb32[322] = fb32[323] = pp1;
		fb32[480] = fb32[481] = pp0; fb32[482] = fb32[483] = pp1;
		fb32[640] = fb32[641] = pp2; fb32[642] = fb32[643] = pp3;
		fb32[800] = fb32[801] = pp2; fb32[802] = fb32[803] = pp3;
		fb32[960] = fb32[961] = pp2; fb32[962] = fb32[963] = pp3;
		fb32[1120] = fb32[1121] = pp2; fb32[1122] = fb32[1123] = pp3;
		break;

	case 16:
#ifdef SUBDBG
		pp0 = 0xff00ff00;
#endif
		for(i=0; i<4; i++) {
			memset16(fbptr, pp0, 16); fbptr += 320;
			memset16(fbptr, pp0, 16); fbptr += 320;
			memset16(fbptr, pp0, 16); fbptr += 320;
			memset16(fbptr, pp0, 16); fbptr += 320;
		}
		break;
	}
	return;

subdiv:
	*cptr[0] = cpix[0];
	*cptr[1] = cpix[1];
	*cptr[2] = cpix[2];
	*cptr[3] = cpix[3];

	tsz >>= 1;
	rend_tile(fbptr, x0, y0, tsz, 0);
	rend_tile(fbptr + tsz, x0 + tsz, y0, tsz, 1);
	fbptr += (tsz << 8) + (tsz << 6);
	y0 += tsz;
	rend_tile(fbptr, x0, y0, tsz, 2);
	rend_tile(fbptr + tsz, x0 + tsz, y0, tsz, 3);
}

static void update(void)
{
	float t;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	cgm_midentity(cam_xform);
	cgm_mtranslate(cam_xform, 0, 0, -cam_dist);
	cgm_mrotate_x(cam_xform, cgm_deg_to_rad(cam_phi));
	cgm_mrotate_y(cam_xform, cgm_deg_to_rad(cam_theta));

	t = (float)time_msec / 1000.0f;
	subsph->p.x = (float)cos(t) * 0.5f;
	subsph->p.y = (float)sin(t) * 0.5f;
	subsph->p.z = -0.5f;
}

static void draw(void)
{
	int i, j, xbound, ybound;
	uint16_t *fbptr;
	struct tile *tile;

	update();

	tile = tiles;
	for(i=0; i<NUM_TILES; i++) {
		rend_tile(tile->fbptr, tile->x, tile->y, TILESZ, -1);
		tile++;
	}

	swap_buffers(0);
}
