#include <stdio.h>
#include <math.h>
#include "demo.h"
#include "screen.h"
#include "gfxutil.h"
#include "util.h"
#include "cgmath/cgmath.h"
#include "rt.h"

#define HALFRES
/* define to see a visualization of the sub-sampling resolution */
#undef SUBDBG

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
	int sz;
	unsigned int valid;
	uint16_t *fbptr;
};

#define TILESZ		16
#define NUM_TILES	((320 / TILESZ) * (240 / TILESZ))
/* 4 8x8 + 4^2 4x4 + 4^3 2x2 = 4 + 16 + 64 = 84 */
#define MAX_TILESTACK_SIZE	84

static cgm_vec3 raydir[240][320];
static struct tile tiles[NUM_TILES];
static struct rtscene scn;

static float cam_theta = 10, cam_phi = 10;
static float cam_dist = 5;
static float cam_xform[16];

static struct rtsphere *subsph;
static struct rtbox *box;
static struct rtcylinder *pillar[4];
static union rtobject *obj;

struct screen *raytrace_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j, k;
	float z = 1.0f / tan(cgm_deg_to_rad(25.0f));
	struct tile *tptr = tiles;
	char namebuf[64];

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
				tptr->sz = TILESZ;
				tptr->fbptr = fb_pixels + i * 320 + j;
				tptr->valid = 0;
				tptr++;
			}
		}
	}

	rt_init(&scn);
	if(rt_load(&scn, "data/rayscn.rt") == -1) {
		return -1;
	}

	subsph = (struct rtsphere*)rt_find_object(&scn, "subsph");
	box = (struct rtbox*)rt_find_object(&scn, "box");
	obj = rt_find_object(&scn, "rootdiff");

	for(i=0; i<4; i++) {
		sprintf(namebuf, "pillar%d", i);
		pillar[i] = (struct rtcylinder*)rt_find_object(&scn, namebuf);
	}

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

static void fillsq(uint16_t *ptr, int sz, uint16_t col)
{
	int i;
	uint32_t *ptr32 = (uint32_t*)ptr;
	uint32_t colcol = col | ((uint32_t)col << 16);

	switch(sz) {
	case 2:
#ifdef SUBDBG
		colcol = 0x18ff18ff;
#endif
		ptr32[0] = colcol;
		ptr32[160] = colcol;
		break;
	case 4:
#ifdef SUBDBG
		colcol = 0x03800380;
#endif
		ptr32[0] = ptr32[1] = colcol;
		ptr32[160] = ptr32[161] = colcol;
		ptr32[320] = ptr32[321] = colcol;
		ptr32[480] = ptr32[481] = colcol;
		break;
	case 8:
#ifdef SUBDBG
		colcol = 0xe00fe00f;
#endif
		ptr32[0] = ptr32[1] = colcol; ptr32[2] = ptr32[3] = colcol;
		ptr32[160] = ptr32[161] = colcol; ptr32[162] = ptr32[163] = colcol;
		ptr32[320] = ptr32[321] = colcol; ptr32[322] = ptr32[323] = colcol;
		ptr32[480] = ptr32[481] = colcol; ptr32[482] = ptr32[483] = colcol;
		ptr32[640] = ptr32[641] = colcol; ptr32[642] = ptr32[643] = colcol;
		ptr32[800] = ptr32[801] = colcol; ptr32[802] = ptr32[803] = colcol;
		ptr32[960] = ptr32[961] = colcol; ptr32[962] = ptr32[963] = colcol;
		ptr32[1120] = ptr32[1121] = colcol; ptr32[1122] = ptr32[1123] = colcol;
		break;
	case 16:
#ifdef SUBDBG
		col = 0xff00;
#endif
		for(i=0; i<4; i++) {
			memset16(ptr, col, 16); ptr += 320;
			memset16(ptr, col, 16); ptr += 320;
			memset16(ptr, col, 16); ptr += 320;
			memset16(ptr, col, 16); ptr += 320;
		}
		break;
	default:
		break;
	}
}


/* keep only some significant bits from R G and B for color comparisons */
/* 11100 111100 11100 (1110 0111 1001 1100) */
#define CMPMASK		0xe79c
/* 11000 111000 11000 (1100 0111 0001 1000) */
/*#define CMPMASK		0xc718*/

static void rend_tile(struct tile *tile)
{
	int x0, y0, x1, y1, tsz, hsz, offs;
	unsigned int valid;
	uint16_t c0, c1, c2, c3, tmp, *fbptr;
	struct tile tiles[MAX_TILESTACK_SIZE];
	int tiles_top;

	tiles[0] = *tile;
	tiles_top = 1;
	tile = tiles;

	while(tiles_top > 0) {
		tile = tiles + --tiles_top;

		x0 = tile->x;
		y0 = tile->y;
		tsz = tile->sz;
		hsz = tsz >> 1;
		offs = tsz - 1;
		x1 = x0 + offs;
		y1 = y0 + offs;
		fbptr = tile->fbptr;
		valid = tile->valid;

		switch(valid) {
		case 1:
			c0 = fbptr[0];
			c1 = fbptr[offs] = rend_pixel(x1, y0);
			c2 = fbptr[offs * 320] = rend_pixel(x0, y1);
			c3 = fbptr[offs * 320 + offs] = rend_pixel(x1, y1);
			break;
		case 2:
			c0 = fbptr[0] = rend_pixel(x0, y0);
			c1 = fbptr[offs];
			c2 = fbptr[offs * 320] = rend_pixel(x0, y1);
			c3 = fbptr[offs * 320 + offs] = rend_pixel(x1, y1);
			break;
		case 3:
			c0 = fbptr[0] = rend_pixel(x0, y0);
			c1 = fbptr[offs] = rend_pixel(x1, y0);
			c2 = fbptr[offs * 320];
			c3 = fbptr[offs * 320 + offs] = rend_pixel(x1, y1);
			break;
		case 4:
			c0 = fbptr[0] = rend_pixel(x0, y0);
			c1 = fbptr[offs] = rend_pixel(x1, y0);
			c2 = fbptr[offs * 320] = rend_pixel(x0, y1);
			c3 = fbptr[offs * 320 + offs];
			break;
		default:
			c0 = fbptr[0] = rend_pixel(x0, y0);
			c1 = fbptr[offs] = rend_pixel(x1, y0);
			c2 = fbptr[offs * 320] = rend_pixel(x0, y1);
			c3 = fbptr[offs * 320 + offs] = rend_pixel(x1, y1);
		}

#ifdef HALFRES
		if(tsz <= 4) {
			/* we're doing half-res and we're at 4x4, write the 4 quads and continue */
			goto fillquads;
		}
#else
		if(tsz <= 2) {
			/* we're at 2x2, write the 4 pixels and continue */
			fbptr[0] = c0;
			fbptr[1] = c1;
			fbptr[320] = c2;
			fbptr[321] = c3;
			continue;
		}
#endif

		tmp = c0 & CMPMASK;
		if((c1 & CMPMASK) == tmp && (c2 & CMPMASK) == tmp && (c3 & CMPMASK) == tmp) {
fillquads:
			/* colors are the same, draw 4 quads and continue */
			fillsq(fbptr, hsz, c0);
			fillsq(fbptr + hsz, hsz, c1);
			fillsq(fbptr + 320 * hsz, hsz, c2);
			fillsq(fbptr + 320 * hsz + hsz, hsz, c3);
			continue;
		}

		/* colors are not the same, push onto the stack to subdivide further */
		tile = tiles + tiles_top++;
		tile->x = x0;
		tile->y = y0;
		tile->sz = hsz;
		tile->fbptr = fbptr;
		tile->valid = 1;

		tile = tiles + tiles_top++;
		tile->x = x0 + hsz;
		tile->y = y0;
		tile->sz = hsz;
		tile->fbptr = fbptr + hsz;
		tile->valid = 2;

		tile = tiles + tiles_top++;
		tile->x = x0;
		tile->y = y0 + hsz;
		tile->sz = hsz;
		tile->fbptr = fbptr + 320 * hsz;
		tile->valid = 3;

		tile = tiles + tiles_top++;
		tile->x = x0 + hsz;
		tile->y = y0 + hsz;
		tile->sz = hsz;
		tile->fbptr = fbptr + 320 * hsz + hsz;
		tile->valid = 4;
	}
}

static void update(void)
{
	int i;
	float t, px;

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	cgm_midentity(cam_xform);
	cgm_mtranslate(cam_xform, 0, 0, -cam_dist);
	cgm_mrotate_x(cam_xform, cgm_deg_to_rad(cam_phi));
	cgm_mrotate_y(cam_xform, cgm_deg_to_rad(cam_theta));

	t = (float)time_msec / 1000.0f;
	if(subsph) {
		subsph->p.x = (float)cos(t) * 0.5f;
		subsph->p.y = (float)sin(t) * 0.5f;
		subsph->p.z = -0.5f;
	}

	if(box) {
		px = (float)sin(t) * 1.5;
		box->min.x = px - 0.3f;
		box->max.x = px + 0.3f;
	}

	for(i=0; i<4; i++) {
		if(pillar[i]) {
			pillar[i]->p.x = (float)cos(t + (CGM_PI * 2.0f / 3.0f) * i) * 6.0f;
			pillar[i]->p.z = (float)sin(t + (CGM_PI * 2.0f / 3.0f) * i) * 6.0f;
		}
	}
}

static void draw(void)
{
	int i, j, xbound, ybound;
	uint16_t *fbptr;
	struct tile *tile;

	update();

	tile = tiles;
	for(i=0; i<NUM_TILES; i++) {
		rend_tile(tile);
		tile++;
	}

	swap_buffers(0);
}
