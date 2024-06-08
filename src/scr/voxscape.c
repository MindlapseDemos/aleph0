/* Voxel landscape attempt */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "noise.h"
#include "gfxutil.h"
#include "opt_rend.h"


#define HOR_THE_VER_STEP

#define FP_VIEWER 8
#define FP_BASE 12
#define FP_SCAPE 10
#define FP_REC 18

#define SIN_SHIFT 12
#define SIN_LENGTH (1 << SIN_SHIFT)
#define SIN_TO_COS (SIN_LENGTH / 4)

#define FOV 48
#define VIS_NEAR 32
#define VIS_FAR 320

#define PIXEL_SIZE 2
#define PIXEL_ABOVE (FB_WIDTH / PIXEL_SIZE)
#define VIS_VER_SKIP 1
#define PAL_SHADES 32

#define SKY_HEIGHT 8192
#define PROJ_MUL 256
#define FP_SCALE 16

#define VIS_VER_STEPS ((VIS_FAR - VIS_NEAR) / VIS_VER_SKIP)
#define VIS_HOR_STEPS (FB_WIDTH / PIXEL_SIZE)

#define V_PLAYER_HEIGHT 160
#define V_HEIGHT_SCALER_SHIFT 7
#define V_HEIGHT_SCALER (1 << V_HEIGHT_SCALER_SHIFT)
#define HORIZON (FB_HEIGHT * 0.7)

#define HMAP_WIDTH 512
#define HMAP_HEIGHT 512
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)

#define SKY_TEX_WIDTH 256
#define SKY_TEX_HEIGHT 256

#define REFLECT_SHADE 0.75

static unsigned char* skyTex;
static unsigned short* skyPal[PAL_SHADES];


typedef struct Point2D
{
	int x,y;
} Point2D;


static int *heightScaleTab = NULL;
static int *isin = NULL;
static Point2D *viewNearPosVec = NULL;
static Point2D *viewNearStepVec = NULL;
static int* yMaxHolder = NULL;
static uint16_t** dstX;

static unsigned char *hmap = NULL;
static unsigned char *cmap = NULL;

static int *shadeVoxOff;

static Vector3D viewPos;
static Vector3D viewAngle;

static int prevTime;


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"voxelscape",
	init,
	destroy,
	start,
	0,
	draw
};

struct screen *voxscape_screen(void)
{
	return &scr;
}

static void initSinTab(const int numSines, const int repeats, const int amplitude)
{
	int i;
	const int numSinesRepeat = numSines / repeats;
	float sMul = (2.0f * 3.1415926536f) / (float)(numSinesRepeat-1);

	if (!isin) isin = malloc(numSines * sizeof(int));

	for (i=0; i<numSines; ++i) {
		isin[i] = (int)(sin((float)i * sMul) * amplitude);
	}
}

static void createHeightScaleTab()
{
	int i;
	float z = 0;
	const float dz = 1.0f / (VIS_VER_STEPS - 1);

	if (!heightScaleTab) heightScaleTab = malloc(VIS_VER_STEPS * sizeof(int));

	for (i = 0; i < VIS_VER_STEPS; ++i) {
		const int stepZ = (int)(z * (1 << FP_SCAPE));
		heightScaleTab[i] = (1 << FP_REC) /  (VIS_NEAR + ((stepZ * VIS_FAR) >> FP_SCAPE));
		z += dz;
	}
}

static int readMapFile(const char *path, int count, void *dst)
{
	FILE *f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "voxscape: failed to open %s: %s\n", path, strerror(errno));
		return -1;
	}

	fread(dst, 1, count, f);

	fclose(f);
	return 0;
}

static void initPalShades()
{
	int i,j;

	uint16_t *shadedPmap = (uint16_t*)&cmap[HMAP_SIZE + 512];

	for (j=1; j<PAL_SHADES; ++j) {
		uint16_t *origPmap = (uint16_t*)&cmap[HMAP_SIZE];
		const int shade = PAL_SHADES - j;
		for (i=0; i<256; ++i) {
			const uint16_t c = origPmap[i];
			const int r = (((c >> 11) & 31) * shade) / PAL_SHADES;
			const int g = (((c >> 5) & 63) * shade) / PAL_SHADES;
			const int b = ((c & 31) * shade) / PAL_SHADES;
			*shadedPmap++ = (r << 11) | (g << 5) | b;
		}
	}

	shadeVoxOff = malloc(VIS_VER_STEPS * sizeof(int));

	for (i = 0; i < VIS_VER_STEPS; ++i) {
		float r = (float)i / (VIS_VER_STEPS - 1);
		r = -0.125f + pow(r, 1.5f);
		CLAMP(r,0,1)
		shadeVoxOff[i] = (int)(r * (PAL_SHADES - 1)) * 256;
	}
}

static int initHeightmapAndColormap()
{
	const int cmapSize = HMAP_SIZE + 512 * PAL_SHADES;

	if (!hmap) hmap = malloc(HMAP_SIZE);
	if (!cmap) cmap = malloc(cmapSize);

	if(readMapFile("data/hmap1.bin", HMAP_SIZE, hmap) == -1) return -1;
	if(readMapFile("data/cmap1.bin", cmapSize, cmap) == -1) return -1;

	initPalShades();
	return 0;
}

static void initSkyTexture()
{
	int x, y, i, n;
	const float scale = 1.0f / 64.0f;
	const int perX = (int)(scale * SKY_TEX_WIDTH);
	const int perY = (int)(scale * SKY_TEX_HEIGHT);

	skyTex = (unsigned char*)malloc(SKY_TEX_WIDTH * SKY_TEX_HEIGHT);

	i = 0;
	for (y = 0; y < SKY_TEX_HEIGHT; ++y) {
		for (x = 0; x < SKY_TEX_WIDTH; ++x) {
			/* float f = pfbm2((float)x * scale, (float)y * scale, perX, perY, 8) + 0.25f; */
			float f = pturbulence2((float)x * scale, (float)y * scale, perX, perY, 4);
			CLAMP(f, 0.0f, 0.99f)
			skyTex[i++] = (int)(f * 255.0f);
		}
	}

	for (i = 0; i < PAL_SHADES; ++i) {
		unsigned short* pal;
		skyPal[i] = (unsigned short*)malloc(256 * sizeof(unsigned short));
		pal = skyPal[i];
		for (n = 0; n < 255; ++n) {
			int r = n - 64;
			int g = n - 32;
			int b = n + 64;

			r = (r * (PAL_SHADES - i - 1)) / (PAL_SHADES / 2) + 48;
			g = (g * (PAL_SHADES - i - 1)) / (PAL_SHADES / 2) + 16;
			b = (b * (PAL_SHADES - i - 1)) / (PAL_SHADES / 2) + 32;

			CLAMP(r, 0, 63);
			CLAMP(g, 0, 160);
			CLAMP(b, 0, 255);

			*pal++ = PACK_RGB16(r, g, b);
		}
	}
}


static int init(void)
{
	memset(fb_pixels, 0, (FB_WIDTH * FB_HEIGHT * FB_BPP) / 8);

	initSinTab(SIN_LENGTH, 1, 65536);
	createHeightScaleTab();

	initSkyTexture();

	hmap = malloc(HMAP_SIZE);
	viewNearPosVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));
	viewNearStepVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));
	yMaxHolder = malloc(VIS_HOR_STEPS * sizeof(int));
	dstX = malloc(VIS_HOR_STEPS * sizeof(uint16_t*));

	if(initHeightmapAndColormap() == -1) {
		return -1;
	}

	return 0;
}

static void destroy(void)
{
	int i;

	free(isin);
	free(heightScaleTab);
	free(viewNearPosVec);
	free(viewNearStepVec);
	free(yMaxHolder);
	free(dstX);
	free(hmap);
	free(cmap);
	free(shadeVoxOff);
	free(skyTex);

	for (i = 0; i < PAL_SHADES; ++i) {
		free(skyPal[i]);
	}
}


static void setPoint2D(Point2D *pt, int x, int y)
{
	pt->x = x;
	pt->y = y;
}

static void updateRaySamplePosAndStep()
{
	int i;
	Point2D pl, pr, dHor;
	
	const int halfFov = (FOV / 2) << (SIN_SHIFT-8);
	const int yaw = viewAngle.y & (SIN_LENGTH-1);
	const int yawL = (yaw - halfFov) & (SIN_LENGTH-1);
	const int yawR = (yaw + halfFov) & (SIN_LENGTH-1);
	const int length = (1 << (FP_BASE + FP_SCAPE)) / isin[halfFov + SIN_TO_COS];

	Point2D *viewPosVec = viewNearPosVec;
	Point2D *viewStepVec = viewNearStepVec;

	setPoint2D(&pl, isin[(yawL+SIN_TO_COS)&(SIN_LENGTH-1)]*length, isin[yawL]*length);
	setPoint2D(&pr, isin[(yawR+SIN_TO_COS)&(SIN_LENGTH-1)]*length, isin[yawR]*length);

#ifdef HOR_THE_VER_STEP
	setPoint2D(&dHor, (pr.x - pl.x) / (VIS_HOR_STEPS - 1), (pr.y - pl.y) / (VIS_HOR_STEPS - 1));
	for (i=0; i<VIS_HOR_STEPS; ++i) {
		setPoint2D(viewStepVec++, (VIS_VER_SKIP * pl.x) >> FP_BASE, (VIS_VER_SKIP * pl.y) >> FP_BASE);
		setPoint2D(viewPosVec++, (VIS_NEAR * pl.x) >> FP_BASE, (VIS_NEAR * pl.y) >> FP_BASE);

		pl.x += dHor.x;
		pl.y += dHor.y;
	}
#else
	setPoint2D(&viewNearStepVec[0], (VIS_VER_SKIP * pl.x) >> FP_BASE, (VIS_VER_SKIP * pl.y) >> FP_BASE);
	setPoint2D(&viewNearPosVec[0], (VIS_NEAR * pl.x) >> FP_BASE, (VIS_NEAR * pl.y) >> FP_BASE);
	setPoint2D(&viewNearStepVec[VIS_HOR_STEPS - 1], (VIS_VER_SKIP * pr.x) >> FP_BASE, (VIS_VER_SKIP * pr.y) >> FP_BASE);
	setPoint2D(&viewNearPosVec[VIS_HOR_STEPS - 1], (VIS_NEAR * pr.x) >> FP_BASE, (VIS_NEAR * pr.y) >> FP_BASE);
#endif
}

static uint16_t reflectSky(int px, int py, int dvx, int dvy, int vh)
{
	int u = ((px + ((dvx * vh) >> FP_SCALE)) >> (FP_SCALE - 6)) & (SKY_TEX_WIDTH - 1);
	int v = ((py + ((dvy * vh) >> FP_SCALE)) >> (FP_SCALE - 6)) & (SKY_TEX_HEIGHT - 1);

	return skyPal[(int)((PAL_SHADES-1) * REFLECT_SHADE)][skyTex[v * SKY_TEX_WIDTH + u]];
}


static uint16_t reflectSample(int px, int py, int dvx, int dvy, int ph, int dh, int viewerOffset, int remainingSteps, uint16_t* pal)
{
	int i;

	int vx = px;
	int vy = py;
	for (i = 0; i < remainingSteps; i+=2) {
		const int sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
		const int mapOffset = (viewerOffset + sampleOffset) & (HMAP_SIZE - 1);
		const int hm = hmap[mapOffset];

		const int h = ph >> FP_SCALE;

		if (h < hm) {
			return pal[cmap[mapOffset]];
		}

		vx += dvx;
		vy += dvy;
		ph += dh;
	}

	return reflectSky(px, py, dvx, dvy, dh);
}

static void renderScape()
{
	int i,j;

	const int playerHeight = viewPos.y >> FP_VIEWER;
	const int viewerOffset = (viewPos.z >> FP_VIEWER) * HMAP_WIDTH + (viewPos.x >> FP_VIEWER);

	uint16_t *dstBase = (uint16_t*)fb_pixels + (FB_HEIGHT-1) * FB_WIDTH;
	uint16_t* pmapPtr = (uint16_t*)&cmap[HMAP_SIZE];

	for (j=0; j<VIS_HOR_STEPS; ++j) {
		int yMax = 0;

		int vx = viewNearPosVec[j].x;
		int vy = viewNearPosVec[j].y;
		const int dvx = viewNearStepVec[j].x;
		const int dvy = viewNearStepVec[j].y;

		#if PIXEL_SIZE == 2
			uint32_t *dst = (uint32_t*)dstBase;
		#elif PIXEL_SIZE == 1
			uint16_t *dst = dstBase;
		#endif


		for (i=0; i<VIS_VER_STEPS; i++) {
			const int sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
			const int mapOffset = (viewerOffset + sampleOffset) & (HMAP_SIZE - 1);
			const int hm = hmap[mapOffset];
			int h = (((-playerHeight + hm) * heightScaleTab[i]) >> (FP_REC - V_HEIGHT_SCALER_SHIFT)) + HORIZON;
			if (h > FB_HEIGHT-1) h = FB_HEIGHT-1;

			if (yMax < h) {
				int hCount = h - yMax;

				#if PIXEL_SIZE == 2
					uint32_t cv;
				#elif PIXEL_SIZE == 1
					uint16_t cv;
				#endif

				if (hm > 1) {
					unsigned char c = cmap[mapOffset];
					uint16_t* pal = pmapPtr + shadeVoxOff[i];
					#if PIXEL_SIZE == 2
						uint16_t c16 = pal[c];
					cv = (c16 << 16) | c16;
					#elif PIXEL_SIZE == 1
						cv = pal[c];
					#endif
				} else {
					const int dh = ((playerHeight - hm) << FP_SCALE) / (i + 1);
					uint16_t c16 = reflectSample(vx, vy, dvx, dvy, hm << FP_SCALE, dh, viewerOffset, VIS_VER_STEPS - i, pmapPtr + shadeVoxOff[(int)((VIS_VER_STEPS - 1) * REFLECT_SHADE)]);
					#if PIXEL_SIZE == 2
						cv = (c16 << 16) | c16;
					#elif PIXEL_SIZE == 1
						cv = c16;
					#endif
				}

				do {
					*dst = cv;
					dst -= PIXEL_ABOVE;
				}while(--hCount > 0);

				yMax = h;
				if (yMax == FB_HEIGHT - 1) break;
			}
			vx += dvx;
			vy += dvy;
		}

		dstBase += PIXEL_SIZE;
	}
}

static void renderScapeX()
{
	int i, j;

	const int playerHeight = viewPos.y >> FP_VIEWER;
	const int viewerOffset = (viewPos.z >> FP_VIEWER) * HMAP_WIDTH + (viewPos.x >> FP_VIEWER);

	Point2D* posVecL = &viewNearPosVec[0];
	Point2D* posVecR = &viewNearPosVec[VIS_HOR_STEPS - 1];
	Point2D* stepVecL = &viewNearStepVec[0];
	Point2D* stepVecR = &viewNearStepVec[VIS_HOR_STEPS - 1];

	int vxL = posVecL->x;
	int vxR = posVecR->x;
	int vyL = posVecL->y;
	int vyR = posVecR->y;
	const int dvxL = stepVecL->x;
	const int dvxR = stepVecR->x;
	const int dvyL = stepVecL->y;
	const int dvyR = stepVecR->y;

	uint16_t* dstBase = (uint16_t*)fb_pixels + (FB_HEIGHT - 1) * FB_WIDTH;
	for (i = 0; i < VIS_HOR_STEPS; ++i) {
		dstX[i] = dstBase + i;
	}

	memset(yMaxHolder, 0, VIS_HOR_STEPS * sizeof(int));

	for (j = 0; j < VIS_VER_STEPS; ++j) {
		int vx = vxL;
		int vy = vyL;
		const int dvx = (vxR - vxL) / VIS_HOR_STEPS;
		const int dvy = (vyR - vyL) / VIS_HOR_STEPS;

		uint16_t* pmapPtr = (uint16_t*)&cmap[HMAP_SIZE] + shadeVoxOff[j];
		const int heightScale = heightScaleTab[j];

		for (i = 0; i < VIS_HOR_STEPS; ++i) {
			const int yMax = yMaxHolder[i];
			if (yMax < FB_HEIGHT - 1) {
				const int sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
				const int mapOffset = (viewerOffset + sampleOffset) & (HMAP_SIZE - 1);
				const int hm = hmap[mapOffset];
				int h = (((-playerHeight + hm) * heightScale) >> (FP_REC - V_HEIGHT_SCALER_SHIFT)) + HORIZON;
				if (h > FB_HEIGHT - 1) h = FB_HEIGHT - 1;

				if (yMax < h) {
					const uint16_t cv = pmapPtr[cmap[mapOffset]];

					int hCount = h - yMaxHolder[i];
					uint16_t* dst = dstX[i];
					do {
						*dst = cv;
						dst -= FB_WIDTH;
					} while (--hCount > 0);

					dstX[i] = dst;
					yMaxHolder[i] = h;
				}
			}
			vx += dvx; vy += dvy;
		}
		vxL += dvxL; vyL += dvyL;
		vxR += dvxR; vyR += dvyR;
	}
}

static void setViewPos(int px, int py, int pz)
{
	viewPos.x = px << FP_VIEWER;
	viewPos.y = py << FP_VIEWER;
	viewPos.z = pz << FP_VIEWER;
}

static void setViewAngle(int rx, int ry, int rz)
{
	viewAngle.x = rx & (SIN_LENGTH-1);
	viewAngle.y = ry & (SIN_LENGTH-1);
	viewAngle.z = rz & (SIN_LENGTH-1);
}

static void start(long trans_time)
{
	setViewPos(3*HMAP_WIDTH/4, V_PLAYER_HEIGHT, HMAP_HEIGHT/6);
	setViewAngle(0,128,0);

	prevTime = time_msec;
}

static void move()
{
	const int dt = time_msec - prevTime;
 
	const int speedX = (dt << FP_VIEWER) >> 8;
	const int speedZ = (dt << FP_VIEWER) >> 8;

	const int velX = (speedX * isin[(viewAngle.y + SIN_TO_COS) & (SIN_LENGTH-1)]) >> FP_BASE;
	const int velZ = (speedZ * isin[viewAngle.y]) >> FP_BASE;

	viewPos.x += velX;
	viewPos.z += velZ;

	if(mouse_bmask & MOUSE_BN_LEFT) {
		viewAngle.y = (4*mouse_x) & (SIN_LENGTH-1);
	}
	if(mouse_bmask & MOUSE_BN_RIGHT) {
		viewPos.y = mouse_y << FP_VIEWER;
	}

	prevTime = time_msec;

	updateRaySamplePosAndStep();
}

static void renderBitmapLineSky(int u, int du, unsigned char* src, unsigned short* pal, unsigned short* dst)
{
	int length = FB_WIDTH;
	while (length-- > 0) {
		int tu = (u >> FP_SCALE) & (SKY_TEX_WIDTH - 1);
		*dst++ = pal[src[tu]];
		u += du;
	};
}

static void renderSky()
{
	unsigned short* dst = (unsigned short*)fb_pixels;
	int y;

	int tv = viewPos.x >> FP_VIEWER;

	for (y = 0; y < FB_HEIGHT / 2; ++y) {
		unsigned char* src;
		int z, u, v, du;
		int palNum = (PAL_SHADES * y) / (FB_HEIGHT / 2);

		int yp = FB_HEIGHT / 2 - y;
		if (yp == 0) yp = 1;
		z = (SKY_HEIGHT * PROJ_MUL) / yp;

		du = z * 3;
		u = (-FB_WIDTH / 2) * du;
		v = ((z >> 8) + tv) & (SKY_TEX_HEIGHT - 1);
		src = &skyTex[v * SKY_TEX_WIDTH];

		CLAMP(palNum, 0, PAL_SHADES - 1)
			renderBitmapLineSky(u, du, src, skyPal[palNum], dst);

		dst += FB_WIDTH;
	}

	for (y = FB_HEIGHT / 2; y < FB_HEIGHT / 2 + FB_HEIGHT / 4; ++y) {
		uint32_t* dst32;
		uint32_t c;
		unsigned short* farPal;
		int x;

		int palNum = (PAL_SHADES * (FB_HEIGHT - y)) / (FB_HEIGHT / 2);
		CLAMP(palNum, 0, PAL_SHADES - 1)
		farPal = skyPal[palNum];
		dst32 = (uint32_t*)dst;
		c = farPal[63];
		c = (c << 16) | c;

		for (x = 0; x < FB_WIDTH; ++x) {
			*dst32++ = c;
		}
		dst += FB_WIDTH;
	}
}

static void draw(void)
{
	move();

	/* some view with water */
	setViewPos(2 * HMAP_WIDTH / 4, V_PLAYER_HEIGHT/2, HMAP_HEIGHT / 6-64);

	renderSky();

#ifdef HOR_THE_VER_STEP
	renderScape();
#else
	renderScapeX();
#endif

	swap_buffers(0);
}
