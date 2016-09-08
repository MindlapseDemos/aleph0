#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "screen.h"
#include "demo.h"
#include "3dgfx.h"

struct mesh {
	int prim;
	struct g3d_vertex *varr;
	unsigned int *iarr;
	int vcount, icount;
};

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);
static void draw_mesh(struct mesh *mesh);
static int gen_cube(struct mesh *mesh, float sz);

static struct screen scr = {
	"polytest",
	init,
	destroy,
	start, 0,
	draw
};

static struct mesh cube;

struct screen *polytest_screen(void)
{
	return &scr;
}

static int init(void)
{
	gen_cube(&cube, 1.0);
	return 0;
}

static void destroy(void)
{
	free(cube.varr);
}

static void start(long trans_time)
{
	g3d_matrix_mode(G3D_PROJECTION);
	g3d_load_identity();
	g3d_perspective(50.0, 1.3333333, 0.5, 100.0);

	g3d_enable(G3D_CULL_FACE);
}

static void draw(void)
{
	static int prev_mx, prev_my;
	static float theta, phi = 25;

	int dx = mouse_x - prev_mx;
	int dy = mouse_y - prev_my;
	prev_mx = mouse_x;
	prev_my = mouse_y;

	if(dx || dy) {
		theta += dx * 2.0;
		phi += dy * 2.0;

		if(phi < -90) phi = -90;
		if(phi > 90) phi = 90;
	}

	/*float angle = (float)time_msec / 50.0;*/

	memset(fb_pixels, 0, fb_width * fb_height * 2);

	g3d_matrix_mode(G3D_MODELVIEW);
	g3d_load_identity();
	g3d_translate(0, 0, -5);
	g3d_rotate(phi, 1, 0, 0);
	g3d_rotate(theta, 0, 1, 0);

	draw_mesh(&cube);
}

static void draw_mesh(struct mesh *mesh)
{
	if(mesh->iarr) {
		/*g3d_draw_indexed(mesh->prim, mesh->iarr, mesh->icount, mesh->varr);*/
	} else {
		g3d_draw(mesh->prim, mesh->varr, mesh->vcount);
	}
}

#define NORMAL(vp, x, y, z) do { vp->nx = x; vp->ny = y; vp->nz = z; } while(0)
#define COLOR(vp, cr, cg, cb) do { vp->r = cr; vp->g = cg; vp->b = cb; } while(0)
#define TEXCOORD(vp, tu, tv) do { vp->u = tu; vp->v = tv; } while(0)
#define VERTEX(vp, vx, vy, vz) \
	do { \
		vp->x = vx; vp->y = vy; vp->z = vz; vp->w = 1.0f; \
		++vp; \
	} while(0)

static int gen_cube(struct mesh *mesh, float sz)
{
	struct g3d_vertex *vptr;
	float hsz = sz / 2.0;

	mesh->prim = G3D_QUADS;
	mesh->iarr = 0;
	mesh->icount = 0;

	mesh->vcount = 24;
	if(!(mesh->varr = malloc(mesh->vcount * sizeof *mesh->varr))) {
		return -1;
	}
	vptr = mesh->varr;

	/* -Z */
	NORMAL(vptr, 0, 0, -1);
	COLOR(vptr, 255, 0, 255);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	/* -Y */
	NORMAL(vptr, 0, -1, 0);
	COLOR(vptr, 0, 255, 255);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, -hsz, -hsz, hsz);
	/* -X */
	NORMAL(vptr, -1, 0, 0);
	COLOR(vptr, 255, 255, 0);
	VERTEX(vptr, -hsz, -hsz, -hsz);
	VERTEX(vptr, -hsz, -hsz, hsz);
	VERTEX(vptr, -hsz, hsz, hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	/* +X */
	NORMAL(vptr, 1, 0, 0);
	COLOR(vptr, 255, 0, 0);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, hsz, -hsz, -hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	/* +Y */
	NORMAL(vptr, 0, 1, 0);
	COLOR(vptr, 0, 255, 0);
	VERTEX(vptr, -hsz, hsz, hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	VERTEX(vptr, hsz, hsz, -hsz);
	VERTEX(vptr, -hsz, hsz, -hsz);
	/* +Z */
	NORMAL(vptr, 0, 0, 1);
	COLOR(vptr, 0, 0, 255);
	VERTEX(vptr, -hsz, -hsz, hsz);
	VERTEX(vptr, hsz, -hsz, hsz);
	VERTEX(vptr, hsz, hsz, hsz);
	VERTEX(vptr, -hsz, hsz, hsz);

	return 0;
}
