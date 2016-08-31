#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "imago2.h"
#include "demo.h"
#include "screen.h"

#ifndef M_PI
#define M_PI	3.1415926535
#endif

#define VSCALE	1.5

#define TEX_FNAME	"data/grid.png"
#define TEX_USCALE	4
#define TEX_VSCALE	2

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void stop(long trans_time);
static void draw(void);

static void draw_tunnel_range(unsigned short *pixels, int xoffs, int yoffs, int starty, int num_lines, long tm);
static int count_bits(unsigned int x);
static int count_zeros(unsigned int x);

static unsigned int *gen_test_image(int *wptr, int *hptr);

static struct screen scr = {
	"tunnel",
	init,
	destroy,
	start,
	stop,
	draw
};

static int xsz, ysz, vxsz, vysz;
static int pan_width, pan_height;
static unsigned int *tunnel_map;
static unsigned char *tunnel_fog;

static int tex_xsz, tex_ysz;
static unsigned int *tex_pixels;
static int tex_xshift, tex_yshift;
static unsigned int tex_xmask, tex_ymask;

static long trans_start, trans_dur;
static int trans_dir;


struct screen *tunnel_screen(void)
{
	return &scr;
}


static int init(void)
{
	int i, j, n;
	unsigned int *tmap;
	unsigned char *fog;
	float aspect = (float)fb_width / (float)fb_height;

	xsz = fb_width;
	ysz = fb_height;
	vxsz = xsz * VSCALE;
	vysz = ysz * VSCALE;

	pan_width = vxsz - xsz;
	pan_height = vysz - ysz;

	if(!(tunnel_map = malloc(vxsz * vysz * sizeof *tunnel_map))) {
		fprintf(stderr, "failed to allocate tunnel map\n");
		return -1;
	}
	if(!(tunnel_fog = malloc(vxsz * vysz))) {
		fprintf(stderr, "failed to allocate tunnel fog map\n");
		return -1;
	}

	tmap = tunnel_map;
	fog = tunnel_fog;

	for(i=0; i<vysz; i++) {
		float y = 2.0 * (float)i / (float)vysz - 1.0;
		for(j=0; j<vxsz; j++) {
			float x = aspect * (2.0 * (float)j / (float)vxsz - 1.0);
			float tu = atan2(y, x) / M_PI * 0.5 + 0.5;
			float d = sqrt(x * x + y * y);
			float tv = d == 0.0 ? 0.0 : 1.0 / d;

			int tx = (int)(tu * 65535.0 * TEX_USCALE) & 0xffff;
			int ty = (int)(tv * 65535.0 * TEX_VSCALE) & 0xffff;

			int f = (int)(d * 192.0);

			*tmap++ = (tx << 16) | ty;
			*fog++ = f > 255 ? 255 : f;
		}
	}

	if(!(tex_pixels = img_load_pixels(TEX_FNAME, &tex_xsz, &tex_ysz, IMG_FMT_RGBA32))) {
		fprintf(stderr, "failed to load image " TEX_FNAME "\n");
		return -1;
	}
	if((count_bits(tex_xsz) | count_bits(tex_ysz)) != 1) {
		fprintf(stderr, "non-pow2 image (%dx%d)\n", tex_xsz, tex_ysz);
		return -1;
	}

	/*tex_pixels = gen_test_image(&tex_xsz, &tex_ysz);*/

	n = count_zeros(tex_xsz);
	for(i=0; i<n; i++) {
		tex_xmask |= 1 << i;
	}
	tex_xshift = n;

	n = count_zeros(tex_ysz);
	for(i=0; i<n; i++) {
		tex_ymask |= 1 << i;
	}
	tex_yshift = n;

	return 0;
}

static void destroy(void)
{
	free(tunnel_map);
	free(tunnel_fog);
	img_free_pixels(tex_pixels);
}

static void start(long trans_time)
{
	if(trans_time) {
		trans_start = time_msec;
		trans_dur = trans_time;
		trans_dir = 1;
	}
}

static void stop(long trans_time)
{
	if(trans_time) {
		trans_start = time_msec;
		trans_dur = trans_time;
		trans_dir = -1;
	}
}

#define NUM_WORK_ITEMS	8

static void draw(void)
{
	int i, num_lines = ysz / NUM_WORK_ITEMS;
	int draw_lines = num_lines;
	float t;
	int xoffs, yoffs;

	if(trans_dir) {
		long interval = time_msec - trans_start;
		int progr = num_lines * interval / trans_dur;
		if(trans_dir < 0) {
			draw_lines = num_lines - progr - 1;
		} else {
			draw_lines = progr;
		}
		if(progr >= num_lines) {
			trans_dir = 0;
		}
	}

	t = time_msec / 10000.0;
	xoffs = (int)(cos(t * 3.0) * pan_width / 2) + pan_width / 2;
	yoffs = (int)(sin(t * 4.0) * pan_height / 2) + pan_height / 2;

	for(i=0; i<NUM_WORK_ITEMS; i++) {
		int starty = i * num_lines;
		int resty = starty + draw_lines;
		int rest_lines = num_lines - draw_lines;
		draw_tunnel_range((unsigned short*)fb_pixels, xoffs, yoffs, starty, draw_lines, time_msec);
		if(rest_lines) {
			memset((unsigned short*)fb_pixels + resty * fb_width, 0, rest_lines * fb_width * 2);
		}
	}
}

static void tunnel_color(int *rp, int *gp, int *bp, long toffs, unsigned int tpacked, int fog)
{
	int r, g, b;
	unsigned int col;
	unsigned int tx = (((tpacked >> 16) & 0xffff) << tex_xshift) >> 16;
	unsigned int ty = ((tpacked & 0xffff) << tex_yshift) >> 16;
	tx += toffs;
	ty += toffs << 1;

	tx &= tex_xmask;
	ty &= tex_ymask;

	col = tex_pixels[(ty << tex_xshift) + tx];
	r = col & 0xff;
	g = (col >> 8) & 0xff;
	b = (col >> 16) & 0xff;

	*rp = (r * fog) >> 8;
	*gp = (g * fog) >> 8;
	*bp = (b * fog) >> 8;
}

#define PACK_RGB16(r, g, b) \
	(((((r) >> 3) & 0x1f) << 11) | ((((g) >> 2) & 0x3f) << 5) | (((b) >> 3) & 0x1f))
#define PACK_RGB32(r, g, b) \
	((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))

static void draw_tunnel_range(unsigned short *pix, int xoffs, int yoffs, int starty, int num_lines, long tm)
{
	int i, j;
	unsigned int *tmap = tunnel_map + (starty + yoffs) * vxsz + xoffs;
	unsigned char *fog = tunnel_fog + (starty + yoffs) * vxsz + xoffs;

	long toffs = tm / 4;
	unsigned int *pixels = (unsigned int*)pix + starty * (fb_width >> 1);

	for(i=0; i<num_lines; i++) {
		for(j=0; j<(xsz>>1); j++) {
			unsigned int col;
			int r, g, b, idx = j << 1;

			tunnel_color(&r, &g, &b, toffs, tmap[idx], fog[idx]);
			col = PACK_RGB16(r, g, b);
			tunnel_color(&r, &g, &b, toffs, tmap[idx + 1], fog[idx + 1]);
			col |= PACK_RGB16(r, g, b) << 16;
			*pixels++ = col;
		}
		tmap += vxsz;
		fog += vxsz;
	}
}

static int count_bits(unsigned int x)
{
	int i, nbits = 0;
	for(i=0; i<32; i++) {
		if(x & 1) ++nbits;
		x >>= 1;
	}
	return nbits;
}

static int count_zeros(unsigned int x)
{
	int i, num = 0;
	for(i=0; i<32; i++) {
		if(x & 1) break;
		++num;
		x >>= 1;
	}
	return num;
}

static unsigned int *gen_test_image(int *wptr, int *hptr)
{
	int i, j;
	int xsz = 256, ysz = 256;
	unsigned int *pixels, *pix;

	if(!(pixels = malloc(xsz * ysz * sizeof *pix))) {
		return 0;
	}
	pix = pixels;

	for(i=0; i<ysz; i++) {
		for(j=0; j<xsz; j++) {
			int val = i ^ j;

			*pix++ = PACK_RGB32(val, val / 2, val / 4);
		}
	}

	*wptr = xsz;
	*hptr = ysz;
	return pixels;
}
