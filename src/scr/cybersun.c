#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "demo.h"
#include "3dgfx.h"
#include "screen.h"
#include "gfxutil.h"
#include "mesh.h"
#include "image.h"
#include "util.h"
#include "cgmath/cgmath.h"

#define TM_RIPPLE_START		1.0f
#define TM_RIPPLE_TRANS_LEN	3.0f

#define VFOV	50.0f
#define HFOV	(VFOV * 1.333333f)

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_mountains(void);

static struct screen scr = {
	"cybersun",
	init,
	destroy,
	start,
	0,
	draw
};

static float cam_theta = 0, cam_phi = 0;
static float cam_dist = 0;

static struct g3d_mesh gmesh;
#define GMESH_GRIDSZ	25
#define GMESH_SIZE		128
static struct image gtex;

#define MOUNTIMG_WIDTH	512
#define MOUNTIMG_HEIGHT	64
static struct image mountimg;
static int mountimg_skip[MOUNTIMG_WIDTH];

static long part_start;


struct screen *cybersun_screen(void)
{
	return &scr;
}

static int init(void)
{
	int i, j;

	if(gen_plane_mesh(&gmesh, GMESH_SIZE, GMESH_SIZE, GMESH_GRIDSZ, GMESH_GRIDSZ) == -1) {
		return -1;
	}
	for(i=0; i<gmesh.vcount; i++) {
		gmesh.varr[i].u *= GMESH_GRIDSZ;
		gmesh.varr[i].v *= GMESH_GRIDSZ;
	}
	if(load_image(&gtex, "data/pgrid.png") == -1) {
		return -1;
	}
	if(load_image(&mountimg, "data/cybmount.png") == -1) {
		return -1;
	}
	assert(mountimg.width == MOUNTIMG_WIDTH);
	assert(mountimg.height == MOUNTIMG_HEIGHT);

	for(i=0; i<MOUNTIMG_WIDTH; i++) {
		uint16_t *pptr = mountimg.pixels + i;
		for(j=0; j<MOUNTIMG_HEIGHT; j++) {
			if(*pptr != 0x7e0) {
				mountimg_skip[i] = j;
				break;
			}
			pptr += MOUNTIMG_WIDTH;
		}
	}
	destroy_image(&mountimg);
	mountimg.pixels = 0;

	return 0;
}

static void destroy(void)
{
	destroy_mesh(&gmesh);
	destroy_image(&gtex);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(VFOV, 1.3333333, 0.5, 500.0);

	g3d_enable(G3D_CULL_FACE);

	g3d_clear_color(85, 70, 136);

	part_start = time_msec;
}

static void update(void)
{
	int i, j;
	float t = (time_msec - part_start) / 1000.0f;
	struct g3d_vertex *vptr;

	float ampl = cgm_smoothstep(TM_RIPPLE_START, TM_RIPPLE_START + TM_RIPPLE_TRANS_LEN, t);

	mouse_orbit_update(&cam_theta, &cam_phi, &cam_dist);

	/* update mesh */
	vptr = gmesh.varr;
	for(i=0; i<GMESH_GRIDSZ + 1; i++) {
		for(j=0; j<GMESH_GRIDSZ + 1; j++) {
			float u = (float)j / GMESH_GRIDSZ - 0.5f;
			float v = (float)i / GMESH_GRIDSZ - 0.5f;
			float x = u * 32.0f;
			float y = v * 32.0f;
			float r = sqrt(x * x + y * y);

			vptr->z = sin(x * 0.5 + t) + cos(x * 0.8f) * 0.5f;
			vptr->z += cos(y * 0.5 + t);
			vptr->z += sin(r + t) * 0.5f;
			vptr->z *= r * 0.1f > 1.0f ? 1.0f : r * 0.1f;
			vptr->z *= ampl;
			vptr++;
		}
	}
}

static void draw(void)
{
	update();

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, -2, -cam_dist);
	g3d_rotate(cam_phi, 1, 0, 0);
	g3d_rotate(cam_theta, 0, 1, 0);
	if(opt.sball) {
		g3d_mult_matrix(sball_matrix);
	}

	g3d_clear(G3D_COLOR_BUFFER_BIT | G3D_DEPTH_BUFFER_BIT);
	draw_mountains();

	g3d_set_texture(gtex.width, gtex.height, gtex.pixels);
	g3d_enable(G3D_TEXTURE_2D);
	g3d_enable(G3D_DEPTH_TEST);

	g3d_push_matrix();
	g3d_rotate(-90, 1, 0, 0);
	draw_mesh(&gmesh);
	g3d_pop_matrix();

	g3d_disable(G3D_DEPTH_TEST);
	g3d_disable(G3D_TEXTURE_2D);

	swap_buffers(fb_pixels);
}

/* XXX all the sptr calculations assume mountimg.width == 512 */
static void draw_mountains(void)
{
	int i, j, horizon_y, y;
	int32_t x, xstart, xend, dx;
	uint16_t *dptr;

	/* 24.8 fixed point, 512 width, 90deg arc */
	xstart = cround64(cam_theta * (256.0 * MOUNTIMG_WIDTH / 90.0));
	xend = cround64((cam_theta + HFOV) * (256.0 * MOUNTIMG_WIDTH / 90.0));
	dx = (xend - xstart) / FB_WIDTH;
	x = xstart;

	horizon_y = cround64(-cam_phi * (FB_HEIGHT / 45.0)) + FB_HEIGHT / 2;
	y = horizon_y - MOUNTIMG_HEIGHT;

	if(y >= FB_HEIGHT) {
		/* TODO draw gradient for the sky */
		return;
	}
	if(horizon_y < 0) {
		memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);
		return;
	}

	for(i=0; i<FB_WIDTH; i++) {
		int skip = mountimg_skip[(x >> 8) & 0x1ff];
		int vspan = MOUNTIMG_HEIGHT - skip;

		dptr = fb_pixels + (y + skip) * FB_WIDTH + i;

		for(j=0; j<vspan; j++) {
			*dptr = 0;	/* black mountains */
			dptr += FB_WIDTH;
		}

		x += dx;
	}

	if(horizon_y < FB_HEIGHT) {
		memset(fb_pixels + horizon_y * FB_WIDTH, 0, (FB_HEIGHT - horizon_y) * FB_WIDTH * 2);
	}
}
