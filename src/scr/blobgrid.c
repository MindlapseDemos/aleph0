/* Blobgrid effect idea */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "noise.h"

#include "opt_rend.h"
#include "thunder.h"


typedef struct BlobGridParams
{
	int numPoints;
	int blobSizesNum;
	int connectionDist;
	int connectionBreaks;
} BlobGridParams;

#define FP_PT 16

/* #define STARS_NORMAL */

#define MAX_NUM_POINTS 256
#define NUM_STARS MAX_NUM_POINTS
#define STARS_CUBE_LENGTH 1024
#define STARS_CUBE_DEPTH 512

#define FAINT_BG_WIDTH 256
#define FAINT_BG_HEIGHT 256

static unsigned char* faintBgTex;

BlobGridParams bgParamsStars = { NUM_STARS, 5, 2*4096, 16};

static int currentVisibleStarsNum = 0;


typedef struct Pos3D
{
	int x,y,z;
} Pos3D;

static Pos3D *origPos;
static Pos3D *screenPos;

static unsigned long startingTime;

static unsigned short *blobsPal;
static unsigned int *blobsPal32;
static unsigned char *blobBuffer;

struct screen* thunderScreen = NULL;
static unsigned short* thunderPal;
static unsigned int* thunderPal32;

static dseq_event* ev_thingsIn;
static dseq_event* ev_thingsOut;


static int blobg_init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"blobgrid",
	blobg_init,
	destroy,
	start,
	0,
	draw
};

struct screen *blobgrid_screen(void)
{
	return &scr;
}

static void initFaintBackground()
{
	int x, y, i;
	const float scale = 1.0f / 32.0f;
	const int perX = (int)(scale * FAINT_BG_WIDTH);
	const int perY = (int)(scale * FAINT_BG_HEIGHT);

	faintBgTex = (unsigned char*)malloc(FAINT_BG_WIDTH * FAINT_BG_HEIGHT);

	i = 0;
	for (y = 0; y < FAINT_BG_HEIGHT; ++y) {
		for (x = 0; x < FAINT_BG_WIDTH; ++x) {
			float f = pfbm2((float)x * scale, (float)y * scale, perX, perY, 8) + 0.25f;
			/*float f = pturbulence2((float)x * scale, (float)y * scale, perX, perY, 4); */
			CLAMP(f, 0.0f, 0.99f);
			faintBgTex[i++] = (int)(f * 9.0f);
		}
	}
}

static void initStars(void)
{
	int i;
	Pos3D *dst = origPos;

	for (i=0; i<NUM_STARS; ++i) {
		dst->x = (rand() & (STARS_CUBE_LENGTH - 1)) - STARS_CUBE_LENGTH / 2;
		dst->y = (rand() & (STARS_CUBE_LENGTH - 1)) - STARS_CUBE_LENGTH / 2;
		dst->z = rand() & (STARS_CUBE_DEPTH - 1);
		++dst;
	}
}

static int blobg_init(void)
{
	origPos = (Pos3D*)malloc(sizeof(Pos3D) * MAX_NUM_POINTS);
	screenPos = (Pos3D*)malloc(sizeof(Pos3D) * MAX_NUM_POINTS);

	blobBuffer = (unsigned char*)malloc(POLKA_BUFFER_WIDTH * POLKA_BUFFER_HEIGHT);
	blobsPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);
	thunderPal = (unsigned short*)malloc(sizeof(unsigned short) * 256);

	setPalGradient(0,63, 0,0,0, 31,63,63, blobsPal);
	setPalGradient(64,255, 31,63,31, 23,15,7, blobsPal);

	setPalGradient(0, 127, 0, 0, 0, 7, 9, 15, thunderPal);
	setPalGradient(128, 255, 7, 9, 15, 31, 27, 7, thunderPal);

	blobsPal32 = createColMap16to32(blobsPal);
	thunderPal32 = createColMap16to32(thunderPal);

	initBlobGfx();

	initStars();

	initFaintBackground();

	ev_thingsIn = dseq_lookup("blobgrid.thingsIn");
	ev_thingsOut = dseq_lookup("blobgrid.thingsOut");

	return 0;
}

static void destroy(void)
{
	free(blobsPal);
	free(thunderPal);
	free(blobsPal32);
	free(thunderPal32);
	free(origPos);
	free(screenPos);
	free(faintBgTex);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void drawConnections(BlobGridParams *params)
{
	int i,j,k;
	int numPoints = currentVisibleStarsNum;
	const int connectionDist = params->connectionDist;
	const int connectionBreaks = params->connectionBreaks;
	const int blobSizesNum = params->blobSizesNum;

	const int bp = connectionDist / connectionBreaks;

	if (numPoints <= 0) return;
	if (numPoints > NUM_STARS) numPoints = NUM_STARS;

	for (j=0; j<numPoints; ++j) {
		const int xpj = screenPos[j].x;
		const int ypj = screenPos[j].y;
		for (i=j+1; i<numPoints; ++i) {
			const int xpi = screenPos[i].x;
			const int ypi = screenPos[i].y;

			if (i!=j) {
				const int lx = xpi - xpj;
				const int ly = ypi - ypj;
				const int dst = lx*lx + ly*ly;

				if (dst >= bp && dst < connectionDist) {
					const int steps = dst / bp;
					int px = xpi << FP_PT;
					int py = ypi << FP_PT;
					const int dx = ((xpj - xpi) << FP_PT) / steps;
					const int dy = ((ypj - ypi) << FP_PT) / steps;

					for (k=0; k<steps-1; ++k) {
						int iii = k >> 1;
						if (iii < 0) iii = 0; if (iii > blobSizesNum-1) iii = blobSizesNum-1;
						px += dx;
						py += dy;
						drawBlob(px>>FP_PT, py>>FP_PT,blobSizesNum - 1 - iii, blobBuffer);
					}
				}
			}
		}
	}
}

static void drawConnections3D(BlobGridParams *params)
{
	int i,j,k;
	int numPoints = currentVisibleStarsNum;
	const int connectionDist = params->connectionDist;
	const int connectionBreaks = params->connectionBreaks;
	const int blobSizesNum = params->blobSizesNum;

	const int bp = connectionDist / connectionBreaks;

	if (numPoints <= 0) return;
	if (numPoints > NUM_STARS) numPoints = NUM_STARS;

	for (j=0; j<numPoints; ++j) {
		const int xpj = origPos[j].x;
		const int ypj = origPos[j].y;
		const int zpj = origPos[j].z;
		if (zpj > 0) {
			for (i=j+1; i<numPoints; ++i) {
				const int xpi = origPos[i].x;
				const int ypi = origPos[i].y;
				const int zpi = origPos[i].z;

				if (i!=j && zpi > 0) {
					const int lx = xpi - xpj;
					const int ly = ypi - ypj;
					const int lz = zpi - zpj;
					const int dst = lx*lx + ly*ly + lz*lz;

					if (bp > 0 && dst < connectionDist) {
						const int xsi = screenPos[i].x;
						const int ysi = screenPos[i].y;
						const int xsj = screenPos[j].x;
						const int ysj = screenPos[j].y;

						int length = isqrt((xsi - xsj)*(xsi - xsj) + (ysi - ysj)*(ysi - ysj));

						int px = xsi << FP_PT;
						int py = ysi << FP_PT;

						int dx, dy, dl, ti, zi;

						if (length<=1) continue;	/* ==0 crashes, possibly overflow from sqrt giving a NaN? */
						dl = (1 << FP_PT) / length;
						dx = (xsj - xsi) * dl;
						dy = (ysj - ysi) * dl;

						ti = 0;
						zi = (zpi + zpj) >> 1;
						for (k=0; k<length-1; ++k) {
							int size = ((blobSizesNum-1) * 1 * ti) >> FP_PT;
							/* if (size > blobSizesNum-1) size = blobSizesNum-1 - size; */
							if (size < 3) size = 3;
							/* if (size > blobSizesNum-1) size = blobSizesNum-1; */

							size = (size * (STARS_CUBE_DEPTH + STARS_CUBE_DEPTH / 4 - zi)) / STARS_CUBE_DEPTH;
							if (size > blobSizesNum - 1) size = blobSizesNum - 1;

							px += dx;
							py += dy;
							ti += dl;
							drawBlob(px>>FP_PT, py>>FP_PT, size, blobBuffer);
						}
					}
				}
			}
		}
	}
}

static void drawStars(BlobGridParams *params)
{
	const int blobSizesNum = params->blobSizesNum;

	int count = currentVisibleStarsNum;
	Pos3D *src = origPos;
	Pos3D *dst = screenPos;

	if (count <= 0) return;
	if (count > NUM_STARS) count = NUM_STARS;

	do {
		const int x = src->x;
		const int y = src->y;
		const int z = src->z;

		dst->z = z;
		if (z > 0) {
			int xp = (POLKA_BUFFER_WIDTH / 2) + (x << 6) / z;
			int yp = (POLKA_BUFFER_HEIGHT / 2) + (y << 6) / z;

			int size = ((2*blobSizesNum - 1) * (STARS_CUBE_DEPTH - z)) / STARS_CUBE_DEPTH;

			if (xp < POLKA_BUFFER_PAD) xp = POLKA_BUFFER_PAD; if (xp > 2 * POLKA_BUFFER_PAD + FB_WIDTH-1) xp = 2 * POLKA_BUFFER_PAD + FB_WIDTH-1;
			if (yp < POLKA_BUFFER_PAD) yp = POLKA_BUFFER_PAD; if (yp > 2 * POLKA_BUFFER_PAD + FB_HEIGHT-1) yp = 2 * POLKA_BUFFER_PAD + FB_HEIGHT-1;

			dst->x = xp;
			dst->y = yp;

			drawBlob(xp,yp,size,blobBuffer);
		}
		++src;
		++dst;
	}while(--count != 0);
}

static void moveStars()
{
	int count = currentVisibleStarsNum;
	Pos3D *src = origPos;

	if (count <= 0) return;
	if (count > NUM_STARS) count = NUM_STARS;

	do {
		#ifndef STARS_NORMAL
			src->x += (((count) & 7) + 1);
			src->y += (((count) & 3) + 2);
			if (src->x >= STARS_CUBE_DEPTH / 1) src->x = - STARS_CUBE_DEPTH / 1;
			if (src->y >= STARS_CUBE_DEPTH / 1) src->y = - STARS_CUBE_DEPTH / 1;
		#endif			

		src->z = (src->z - ((count & 3) + 1)) & (STARS_CUBE_DEPTH - 1);
		++src;
	}while(--count != 0);
}

static void drawEffect(BlobGridParams *params, int t)
{
	static int prevT = 0;
	const int dt = t - prevT;
	if (dt < 0 || dt > 20) {
		moveStars();
	
		clearBlobBuffer(blobBuffer);
		drawStars(params);
		drawConnections3D(params);

		prevT = t;
	}
}


void blendThunder8bppWithVram(unsigned char* buffer, unsigned int* colMap16to32)
{
	int i;
	unsigned short* src = (unsigned short*)buffer;
	unsigned int* dst = (unsigned int*)fb_pixels;

	for (i = 0; i < (FB_WIDTH * FB_HEIGHT) / 2; ++i) {
		*dst++ |= colMap16to32[*src++];
	}
}


static void mergeThunderScreen()
{
	if (!thunderScreen) {
		thunderScreen = scr_lookup("thunder");
	}

	setThunderZoom(dseq_value(ev_thingsIn) - dseq_value(ev_thingsOut));

	thunderScreen->draw();

	blendThunder8bppWithVram(getThunderBlurBuffer(), thunderPal32);
}

static void renderFaintBackground(int t)
{
	static int flipflop = 0;

	int x, y;
	int tt = t >> 6;

	unsigned int* dst = (unsigned int*)(fb_pixels + flipflop * FB_WIDTH);
	for (y = 0; y < FB_HEIGHT; y+=2) {
		unsigned char* src = &faintBgTex[((y + flipflop) & (FAINT_BG_HEIGHT-1)) * FAINT_BG_WIDTH];
		for (x = 0; x < FB_WIDTH/2; ++x) {
			int c = src[(x+tt) & (FAINT_BG_WIDTH - 1)] + src[(x + 2*tt) & (FAINT_BG_WIDTH - 1)];
			*dst++ |= (c << 16) | c;
		}
		dst += FB_WIDTH/2;
	}
	flipflop = (flipflop + 1) & 1;
}

static void draw(void)
{
	int t = time_msec - startingTime;
	const float ft = dseq_value(ev_thingsIn) - dseq_value(ev_thingsOut);

	currentVisibleStarsNum = ft * NUM_STARS;

	drawEffect(&bgParamsStars, t);

	buffer8bppToVram(blobBuffer, blobsPal32);

	renderFaintBackground(t);

	mergeThunderScreen();

	fadeToBlack16bpp(ft, fb_pixels, FB_WIDTH, FB_HEIGHT, FB_WIDTH);

	swap_buffers(0);
}
