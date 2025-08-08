#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "imago2.h"
#include "demo.h"
#include "screen.h"
#include "gfxutil.h"
#include "util.h"
#include "smoketxt.h"

#ifndef M_PI
#define M_PI	3.1415926535
#endif

#define UPD_RATE	33
#define UPD_DT		(1.0 / 30.0)

#define VSCALE	1.5

#define TEX_USCALE	3
#define TEX_VSCALE	1
#undef ROTATE

#define BLUR_RAD	5
#define SINGLE_BLUR


static int tun_init(void);
static void tun_destroy(void);
static void tun_start(long trans_time);
static void tun_stop(long trans_time);
static void tun_update(float tsec, float dt);
static void tun_draw(void);
static void tun_keypress(int key);

static void draw_tunnel_range(unsigned short *pixels, int xoffs, int yoffs, int starty, int num_lines);
static int gen_tables(void);
static int gen_colormaps(struct img_pixmap *pixmap);
static int count_bits(unsigned int x);
static int count_zeros(unsigned int x);

static struct screen scr = {
	"tunnel",
	tun_init,
	tun_destroy,
	tun_start,
	tun_stop,
	tun_draw,
	tun_keypress
};

#define NUM_TUNPAL		16

static int vxsz, vysz;
static int pan_width, pan_height;
static unsigned long start_time;
static unsigned int *tunnel_map;
static unsigned char *tunnel_fog;

static int tex_xsz, tex_ysz;
static unsigned char *tex_pixels, *tex_pixels_curblur;
static uint16_t tunnel_cmap[NUM_TUNPAL][256];
static int tex_xshift, tex_yshift;
static unsigned int tex_xmask, tex_ymask;

static long trans_start, trans_dur;
static int trans_dir;

static unsigned long prev_upd;

static int blurlevel, nextblur;
static float tunpos, tunspeed, nextspeed;
static long accel_start;

static const float speedtab[] = {100.0f * UPD_DT, 300.0f * UPD_DT, 500.0f * UPD_DT, 800.0f * UPD_DT};

/* smoketext stuff */
static struct smktxt *stx[2], *curstx;
static unsigned char *smokebuf, *cur_smokebuf, *prev_smokebuf;
#define SMOKEBUF_SIZE	(320 * 240)
static unsigned int smoke_cmap[256 * 4], fadepal[256 * 4];
static struct img_pixmap textimg[2];

static dseq_event *ev_text, *ev_accel;
static int text_state;


struct screen *tunnel_screen(void)
{
	return &scr;
}

static int tun_init(void)
{
	int i, n;
	struct img_pixmap pixmap;
	unsigned char *pal;

	vxsz = FB_WIDTH * VSCALE;
	vysz = FB_HEIGHT * VSCALE;

	pan_width = vxsz - FB_WIDTH;
	pan_height = vysz - FB_HEIGHT;

	if(gen_tables() == -1) {
		return -1;
	}

	img_init(&pixmap);
	if(imgass_load(&pixmap, "data/tunnel2.png") == -1) {
		fprintf(stderr, "failed to load tunnel image\n");
		return -1;
	}
	tex_pixels = tex_pixels_curblur = pixmap.pixels;
	tex_xsz = pixmap.width;
	tex_ysz = pixmap.height;

	if(pixmap.fmt != IMG_FMT_IDX8) {
		fprintf(stderr, "tunnel texture is not palettized\n");
		return -1;
	}
	if((count_bits(tex_xsz) | count_bits(tex_ysz)) != 1) {
		fprintf(stderr, "non-pow2 image (%dx%d)\n", tex_xsz, tex_ysz);
		return -1;
	}

	n = count_zeros(tex_xsz);
	for(i=0; i<n; i++) {
		tex_xmask |= 1 << i;
	}
	tex_xshift = n;

	/*
	n = count_zeros(tex_ysz);
	for(i=0; i<n; i++) {
		tex_ymask |= 1 << i;
	}
	tex_yshift = n;
	*/

	tex_ymask = tex_xmask;
	tex_yshift = tex_xshift;

	if(gen_colormaps(&pixmap) == -1) {
		return -1;
	}

	img_init(textimg);
	img_init(textimg + 1);

	if(imgass_load(textimg, "data/ml_dsr8.png") == -1) {
		fprintf(stderr, "failed to load image: data/ml_dsr8.png\n");
		return -1;
	}
	img_convert(textimg, IMG_FMT_GREY8);
	if(imgass_load(textimg + 1, "data/presents.png") == -1) {
		fprintf(stderr, "failed to load image: data/presents.png\n");
		return -1;
	}
	img_convert(textimg + 1, IMG_FMT_GREY8);

	if(!(stx[0] = create_smktxt("data/ml_dsr.png", "data/vfield1"))) {
		return -1;
	}
	if(!(stx[1] = create_smktxt("data/presents.png", "data/vfield1"))) {
		return -1;
	}
	if(!(smokebuf = malloc(SMOKEBUF_SIZE * 2))) {
		perror("failed to allocate smoke ping-pong buffers");
		return -1;
	}
	cur_smokebuf = smokebuf;
	prev_smokebuf = smokebuf + SMOKEBUF_SIZE;

	if(!(pal = imgass_load_pixels("data/smokepal.ppm", &n, &n, IMG_FMT_RGB24))) {
		fprintf(stderr, "failed to load smoke colormap: smokepal.png\n");
		return -1;
	}
	for(i=0; i<256; i++) {
		smoke_cmap[i * 4] = pal[i * 3];
		smoke_cmap[i * 4 + 1] = pal[i * 3 + 1];
		smoke_cmap[i * 4 + 2] = pal[i * 3 + 2];
	}
	img_free_pixels(pal);

	ev_text = dseq_lookup("tunnel.text");
	ev_accel = dseq_lookup("tunnel.accel");

	dseq_set_trigger(ev_text, DSEQ_TRIG_KEY, 0, 0);
	dseq_set_trigger(ev_accel, DSEQ_TRIG_KEY, 0, 0);
	return 0;
}

static void tun_destroy(void)
{
	free(tunnel_map);
	free(tunnel_fog);
	img_free_pixels(tex_pixels);
	free(smokebuf);
}

static void tun_start(long trans_time)
{
	if(trans_time) {
		trans_start = time_msec;
		trans_dur = trans_time;
		trans_dir = 1;
	}

	tunpos = 0;
	blurlevel = nextblur = 0;
	tunspeed = nextspeed = speedtab[0];

	curstx = 0;
	text_state = 0;

	memset(smokebuf, 0, SMOKEBUF_SIZE * 2);
	start_time = time_msec;
	prev_upd = 0;

	tun_update(0, 0);
}

static void tun_stop(long trans_time)
{
	if(trans_time) {
		trans_start = time_msec;
		trans_dur = trans_time;
		trans_dir = -1;
	}
}

#define NUM_WORK_ITEMS	8
#define NUM_LINES		(FB_HEIGHT / NUM_WORK_ITEMS)
#define ACCEL_DUR	1000
static int draw_lines;
static int xoffs, yoffs;
static long toffs;

static void tun_update(float tsec, float dt)
{
	int i, progr;
	long interval;
	float t, curspeed, maxpan;

	draw_lines = NUM_LINES;

	if(trans_dir) {
		interval = time_msec - trans_start;
		progr = NUM_LINES * interval / trans_dur;
		if(progr >= NUM_LINES) {
			trans_dir = 0;
			draw_lines = NUM_LINES;
		} else {
			if(trans_dir < 0) {
				draw_lines = NUM_LINES - progr - 1;
			} else {
				draw_lines = progr;
			}
		}
	}

	if(tunspeed != nextspeed) {
		if(time_msec < accel_start + ACCEL_DUR) {
			t = (float)(time_msec - accel_start) / (float)ACCEL_DUR;
			curspeed = tunspeed + (nextspeed - tunspeed) * t;
			if(blurlevel != nextblur && t > 0.5f) {
				blurlevel = nextblur;
				tex_pixels_curblur = tex_pixels + tex_xsz * tex_xsz * blurlevel;
			}
		} else {
			tunspeed = nextspeed;
			goto samespeed;
		}
	} else {
samespeed:
		curspeed = tunspeed;
	}

	maxpan = (curspeed - speedtab[0]) / (speedtab[3] - speedtab[0]);
	xoffs = (int)(cos(tsec * 3) * maxpan * pan_width / 2) + pan_width / 2;
	yoffs = (int)(sin(tsec * 4) * maxpan * pan_height / 2) + pan_height / 2;

	tunpos += curspeed * 60.0f * dt;
	toffs = cround64(tunpos);

	if(dseq_triggered(ev_text)) {
		text_state = cround64(dseq_value(ev_text));
		if(text_state == 0 || text_state == 2 || text_state == 4) {
			curstx = text_state ? stx[(text_state >> 1) - 1] : 0;
			/* clear smoke buffer */
			memset(smokebuf, 0, SMOKEBUF_SIZE * 2);
		} else {
			curstx = 0;
		}

	} else if(dseq_triggered(ev_accel)) {
		/* accelerate */
		nextblur = (blurlevel + 1) & 3;
		nextspeed = speedtab[nextblur];
		accel_start = time_msec - start_time;
	}

	if(text_state == 2 || text_state == 4) {
		int fade;
		update_smktxt(curstx);

		fade = cround64(dseq_value(ev_text) * (text_state == 2 ? 256.0f : 128.0f));
		if(fade > 256) fade = 256;

		for(i=0; i<256; i++) {
			fadepal[i * 4] = (smoke_cmap[i * 4] * fade) >> 8;
			fadepal[i * 4 + 1] = (smoke_cmap[i * 4 + 1] * fade) >> 8;
			fadepal[i * 4 + 2] = (smoke_cmap[i * 4 + 2] * fade) >> 8;
			fadepal[i * 4 + 3] = (smoke_cmap[i * 4 + 3] * fade) >> 8;
		}
	}
}

static void tun_draw(void)
{
	unsigned long tm, upd_interv;
	int i, x, y;
	struct ivec2 *vptr;
	void *tmpptr;

	tm = time_msec - start_time;

	upd_interv = tm - prev_upd;
	if(upd_interv >= UPD_RATE) {
		float dt = (float)upd_interv / 1000.0f;
		prev_upd = tm;
		tun_update((float)tm / 1000.0f, dt);
	}

	for(i=0; i<NUM_WORK_ITEMS; i++) {
		int starty = i * NUM_LINES;
		int resty = starty + draw_lines;
		int rest_lines = NUM_LINES - draw_lines;
		draw_tunnel_range(fb_pixels, xoffs, yoffs, starty, draw_lines);
		if(rest_lines) {
			memset(fb_pixels + resty * FB_WIDTH, 0, rest_lines * FB_WIDTH * 2);
		}
	}

	if(curstx) {
		/* blur the previous smoke buffer */
		blur_xyzzy_horiz8(prev_smokebuf, cur_smokebuf);
#ifndef SINGLE_BLUR
		blur_xyzzy_vert8(cur_smokebuf, prev_smokebuf);
#else
		/* swap the smoke buffer pointers */
		tmpptr = cur_smokebuf;
		cur_smokebuf = prev_smokebuf;
		prev_smokebuf = tmpptr;
#endif

		vptr = curstx->em.varr;
		for(i=0; i<curstx->em.pcount; i++) {
			x = ((vptr->x * 228) >> 16) + 160;
			y = 120 - ((vptr->y * 228) >> 16);

			if(x < 0 || x >= 320 || y < 0 || y >= 240) {
				vptr++;
				continue;
			}

			cur_smokebuf[y * 320 + x] = 200;
			vptr++;
		}

		/* overlay the current smoke buffer over the image */
		overlay_full_add_pal(fb_pixels, cur_smokebuf, fadepal);

	} else if(text_state > 0) {
		/* we end up here only when text_state is 1 (ml&dsr solid) or 3 (presents solid) */
		struct img_pixmap *img = textimg + (text_state >> 1);
		uint16_t *dest = fb_pixels + (120 - img->height / 2) * 320 + (160 - img->width / 2);
		overlay_add_pal(dest, img->pixels, img->width, img->height, img->width, smoke_cmap);
	}

	swap_buffers(0);
}

static void tun_keypress(int key)
{
	switch(key) {
	case ' ':
		nextblur = (blurlevel + 1) & 3;
		nextspeed = speedtab[nextblur];
		accel_start = time_msec - start_time;
		break;

	default:
		break;
	}
}

static uint16_t tunnel_color(long toffs, long roffs, unsigned int tpacked, int fog)
{
	unsigned char texel;
	unsigned int tx = (((tpacked >> 16) & 0xffff) << tex_xshift) >> 16;
	unsigned int ty = ((tpacked & 0xffff) << tex_yshift) >> 16;
#ifdef ROTATE
	tx += roffs;
#endif
	ty += toffs;

	tx &= tex_xmask;
	ty &= tex_ymask;

	texel = tex_pixels_curblur[(ty << tex_xshift) + tx];
	return tunnel_cmap[fog >> 4][texel];	/* assumes NUM_TUNPAL == 16 */
}

static void draw_tunnel_range(unsigned short *pix, int xoffs, int yoffs, int starty, int num_lines)
{
	int i, j;
	unsigned int *tmap = tunnel_map + (starty + yoffs) * vxsz + xoffs;
	unsigned char *fog = tunnel_fog + (starty + yoffs) * vxsz + xoffs;

	unsigned int *pixels = (unsigned int*)pix + starty * (FB_WIDTH >> 1);

	for(i=0; i<num_lines; i++) {
		for(j=0; j<(FB_WIDTH>>1); j++) {
			unsigned int col;
			int idx = j << 1;

			col = tunnel_color(toffs, 0, tmap[idx], fog[idx]);
			col |= (unsigned int)tunnel_color(toffs, 0, tmap[idx + 1], fog[idx + 1]) << 16;
			*pixels++ = col;
		}
		tmap += vxsz;
		fog += vxsz;
	}
}

static int gen_tables(void)
{
	int i, j;
	unsigned int *tmap;
	unsigned char *fog;
	float aspect = (float)FB_WIDTH / (float)FB_HEIGHT;

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
			float tv = d == 0.0 ? 0.0 : 0.5 / d;

			int tx = (int)(tu * 65535.0 * TEX_USCALE) & 0xffff;
			int ty = (int)(tv * 65535.0 * TEX_VSCALE) & 0xffff;

			/*int f = (int)(20.0 / d) - 35 + (rand() & 0xf);*/
			int f = (int)(30.0 / d) - 35 + (rand() & 0xf);

			*tmap++ = (tx << 16) | ty;
			*fog++ = f > 255 ? 255 : (f < 0 ? 0 : f);
		}
	}

	return 0;
}

#define R_END	50
#define G_END	0
#define B_END	50

static int gen_colormaps(struct img_pixmap *pixmap)
{
	int i, j;
	int r, g, b;
	struct img_colormap *imgpal;
	uint16_t *cmap;
	int32_t tfix;

	/* populate the first colormap */
	imgpal = img_colormap(pixmap);

	for(i=0; i<NUM_TUNPAL; i++) {
		tfix = (i << 16) / (NUM_TUNPAL - 1);
		cmap = tunnel_cmap[i];
		for(j=0; j<256; j++) {
			r = imgpal->color[j].r;
			g = imgpal->color[j].g;
			b = imgpal->color[j].b;
			r = r + ((R_END - r) * tfix >> 16);
			g = g + ((G_END - g) * tfix >> 16);
			b = b + ((B_END - b) * tfix >> 16);
			*cmap++ = PACK_RGB16(r, g, b);
		}
	}
	return 0;
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
