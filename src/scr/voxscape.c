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
#include "opt_3d.h"
#include "imago2.h"

#include "dos/keyb.h"


#define FP_VIEWER 8
#define FP_BASE 12
#define FP_SCAPE 10
#define FP_REC 18

#define SIN_SHIFT 12
#define SIN_LENGTH (1 << SIN_SHIFT)
#define SIN_TO_COS (SIN_LENGTH / 4)

#define FOV 48
#define VIS_NEAR 16
#define VIS_CLOSE 48
#define VIS_MID 128
#define VIS_FAR 256
#define VIS_HAZE (VIS_FAR - 64)

#define PIXEL_SIZE 1
#define PIXEL_ABOVE (FB_WIDTH / PIXEL_SIZE)
#define VIS_VER_SKIP 1
#define PAL_SHADES 32

#define SKY_HEIGHT 8192
#define FP_SCALE 16

#define VIS_VER_STEPS ((VIS_FAR - VIS_NEAR) / VIS_VER_SKIP)
#define VIS_HOR_STEPS (FB_WIDTH / PIXEL_SIZE)

#define LOCK_PLAYER_TO_GROUND

#define FLY_HEIGHT 96
#define V_PLAYER_HEIGHT 32
#define V_HEIGHT_SCALER_SHIFT 7
#define V_HEIGHT_SCALER (1 << V_HEIGHT_SCALER_SHIFT)
#define HORIZON (FB_HEIGHT * 0.7)

#define HMAP_WIDTH 1024
#define HMAP_HEIGHT 1024
#define HMAP_SIZE (HMAP_WIDTH * HMAP_HEIGHT)

#define SKY_TEX_WIDTH 256
#define SKY_TEX_HEIGHT 256

#define REFLECT_SHADE 0.625

#define DIST_RADIUS 128
#define MAX_POINTS_PER_RADIUS (DIST_RADIUS * 8) /* hope it's enough */

#define PETRUB_SIZE 1024
#define PETRUB_RANGE 16

#define SEA_LEVEL 1

/* When getting different PNG maps, comment out the below and run on PC. Then comment in and use the produced BIN files (and probably will remove the PNGs at the end from data) */
/* #define CALCULATE_DIST_MAP
#define SAVE_DIST_MAP
#define SAVE_COLOR_MAP
#define SAVE_HEIGHT_MAP */


typedef struct Point2D
{
	int x,y;
} Point2D;

static unsigned char* distMap;

static unsigned char* cloudTex;
static unsigned char* skyTex;
static unsigned short* skyPal[PAL_SHADES];

static int *heightScaleTab = NULL;
static int *isin = NULL;
static Point2D *viewNearPosVec = NULL;
static Point2D *viewNearStepVec = NULL;
static int* yMaxHolder = NULL;

static unsigned char *hmap = NULL;
static unsigned char *cmap = NULL;
static uint16_t *pmap = NULL;

static int *shadeVoxOff;
static char* petrubTab;

static void(*drawColumnFunc)(int, uint16_t, uint16_t*);

static int haze;

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

static int writeMapFile(const char* path, int count, unsigned char* src)
{
	FILE* f = fopen(path, "wb");
	if (!f) {
		fprintf(stderr, "voxscape: failed to open %s: %s\n", path, strerror(errno));
		return -1;
	}

	fwrite(src, 1, count, f);

	fclose(f);
	return 0;
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

	uint16_t *shadedPmap = pmap;

	for (j=0; j<PAL_SHADES; ++j) {
		unsigned char* cmapSrc = &cmap[HMAP_SIZE];
		const int shade = PAL_SHADES - j;
		for (i=0; i<256; ++i) {
			const int r = ((*cmapSrc++ * shade) / PAL_SHADES) >> 3;
			const int g = ((*cmapSrc++ * shade) / PAL_SHADES) >> 2;
			const int b = ((*cmapSrc++ * shade) / PAL_SHADES) >> 3;

			*shadedPmap++ = (r << 11) | (g << 5) | b;
		}
	}

	shadeVoxOff = malloc(VIS_VER_STEPS * sizeof(int));

	for (i = 0; i < VIS_VER_STEPS; ++i) {
		float r = (float)i / (VIS_VER_STEPS - 1);
		/* r = -0.125f + pow(r, 1.5f); */
		r = r - 0.25f;
		CLAMP(r,0,1)
		shadeVoxOff[i] = (int)(r * (PAL_SHADES - 1)) * 256;
	}
}

static int loadAndConvertImgColormapPng()
{
	#ifdef SAVE_COLOR_MAP
		int i;
		struct img_pixmap mapPic;
		unsigned char* tempCmap;
		struct img_colormap* cmapSrc;
		unsigned char* cmapDst;

		img_init(&mapPic);
		if (img_load(&mapPic, "data/vxc1.png") == -1) {
			fprintf(stderr, "failed to load voxel colormap image\n");
			return -1;
		}

		tempCmap = malloc(HMAP_SIZE + 256 * 3);
		img_convert(&mapPic, IMG_FMT_IDX8);
		memcpy(tempCmap, mapPic.pixels, HMAP_SIZE);

		cmapSrc = img_colormap(&mapPic);
		cmapDst = &tempCmap[HMAP_SIZE];

		for (i = 0; i < 256; ++i) {
			*cmapDst++ = cmapSrc->color[i].r;
			*cmapDst++ = cmapSrc->color[i].g;
			*cmapDst++ = cmapSrc->color[i].b;
		}

		writeMapFile("data/cmap1.bin", HMAP_SIZE + 256 * 3, tempCmap);

		free(tempCmap);
		img_destroy(&mapPic);
	#else
		if (readMapFile("data/cmap1.bin", HMAP_SIZE + 256 * 3, cmap) == -1) {
			fprintf(stderr, "failed to load voxel colormap data\n");
			return -1;
		}

		initPalShades();
	#endif

	return 0;
}

static int loadAndConvertImgHeightmapPng()
{
	#ifdef SAVE_HEIGHT_MAP
		struct img_pixmap mapPic;

		int i;
		const uint32_t* src;
		int hMin = 100000;
		int hMax = 0;

		img_init(&mapPic);
		if (img_load(&mapPic, "data/vxh1.png") == -1) {
			fprintf(stderr, "failed to load voxel heightmap image\n");
			return -1;
		}

		src = (uint32_t*)mapPic.pixels;

		for (i = 0; i < HMAP_SIZE; ++i) {
			uint32_t c32 = src[i];
			int a = (c32 >> 24) & 255;
			int r = (c32 >> 16) & 255;
			int g = (c32 >> 8) & 255;
			int b = c32 & 255;
			int c = (a + r + g + b) / 4;

			if (c < hMin) hMin = c;
			if (c > hMax) hMax = c;
			hmap[i] = c;
		}
		/* printf("%d %d\n", hMin, hMax); */

		for (i = 0; i < HMAP_SIZE; ++i) {
			int c = (((int)hmap[i] - hMin) * 256) / (hMax - hMin) - 8;
			CLAMP(c, 0, 255);
			hmap[i] = c;
		}

		writeMapFile("data/hmap1.bin", HMAP_SIZE, hmap);

		img_destroy(&mapPic);
	#else
		if (readMapFile("data/hmap1.bin", HMAP_SIZE, hmap) == -1) {
			fprintf(stderr, "failed to load voxel heightmap data\n");
			return -1;
		}
	#endif

	return 0;
}

static int initHeightmapAndColormap()
{
	if (!hmap) hmap = malloc(HMAP_SIZE);
	if (!cmap) cmap = malloc(HMAP_SIZE + 256 * 3);
	if (!pmap) pmap = (uint16_t*)malloc(512 * PAL_SHADES);

	if (loadAndConvertImgHeightmapPng() == -1) return -1;
	if (loadAndConvertImgColormapPng() == -1) return -1;

	return 0;
}


static int initDistMap()
{
	int i;

	#ifdef CALCULATE_DIST_MAP
		int x, y, r, n;

		int *rCount = malloc(DIST_RADIUS * sizeof(int));
		Point2D* distOffsets = malloc(DIST_RADIUS * MAX_POINTS_PER_RADIUS * sizeof(Point2D));

		memset(rCount, 0, DIST_RADIUS * sizeof(int));

		for (y = -DIST_RADIUS; y <= DIST_RADIUS; ++y) {
			for (x = -DIST_RADIUS; x <= DIST_RADIUS; ++x) {
				const int radius = isqrt(x*x + y*y);
				if (radius > 0 && radius <= DIST_RADIUS) {
					const int rIndex = radius - 1;
					const int pIndex = rCount[rIndex]++;
					Point2D* p = &distOffsets[rIndex * MAX_POINTS_PER_RADIUS + pIndex];
					p->x = x;
					p->y = y;
				}
			}
		}
	#endif

	distMap = malloc(HMAP_SIZE);

	#ifdef CALCULATE_DIST_MAP
		i = 0;
		for (y = 0; y < HMAP_HEIGHT; ++y) {
			for (x = 0; x < HMAP_WIDTH; ++x) {
				const int h = hmap[i];
				int safeR = 0;	/* 0 will mean safe to reflect sky instantly */
				int hb = 16;
				for (r = 0; r < DIST_RADIUS; ++r) {
					const int count = rCount[r];
					Point2D* p = &distOffsets[r * MAX_POINTS_PER_RADIUS];
					for (n = 0; n < count; ++n) {
						const int xp = (x + p[n].x) & (HMAP_WIDTH - 1);
						const int yp = (y + p[n].y) & (HMAP_WIDTH - 1);
						const int hCheck = hmap[yp * HMAP_WIDTH + xp] - (hb >> 1);
						if (h < hCheck) {
							safeR = r + 1;
							r = DIST_RADIUS;	/* hack to break again outside */
							break;
						}
					}
					++hb;
				}
				distMap[i++] = safeR;
			}
		}

		#ifdef SAVE_DIST_MAP
			writeMapFile("data/dmap1.bin", HMAP_SIZE, distMap);
		#endif

		free(rCount);
		free(distOffsets);
	#else
		if (readMapFile("data/dmap1.bin", HMAP_SIZE, distMap) == -1) {
			/* memset(distMap, 0, HMAP_SIZE); */ /* 0 will still instantly reflect the clouds but not the mountains which need more thorough steps */ 
			return -1;
		}
	#endif

	petrubTab = malloc(PETRUB_SIZE);
	for (i = 0; i < PETRUB_SIZE; ++i) {
		const float reps = 32.0f;
		float f = 2.0f * pturbulence1((float)i * 1.0f/reps, PETRUB_SIZE / reps, 8);
		CLAMP(f, 0.0f, 1.0f)
		petrubTab[i] = (int)(f * PETRUB_RANGE);
	}

	return 0;
}

static void initSkyAndCloudTextures()
{
	int x, y, i, n;
	const float scale = 1.0f / 64.0f;
	const int perX = (int)(scale * SKY_TEX_WIDTH);
	const int perY = (int)(scale * SKY_TEX_HEIGHT);

	cloudTex = (unsigned char*)malloc(SKY_TEX_WIDTH * SKY_TEX_HEIGHT);

	i = 0;
	for (y = 0; y < SKY_TEX_HEIGHT; ++y) {
		for (x = 0; x < SKY_TEX_WIDTH; ++x) {
			/* float f = pfbm2((float)x * scale, (float)y * scale, perX, perY, 8) + 0.25f; */
			float f = pturbulence2((float)x * scale, (float)y * scale, perX, perY, 4);
			CLAMP(f, 0.0f, 0.99f)
			cloudTex[i++] = (int)(f * 127.0f);
		}
	}

	skyTex = (unsigned char*)malloc(SKY_TEX_WIDTH * SKY_TEX_HEIGHT);

	for (i = 0; i < PAL_SHADES; ++i) {
		unsigned short* pal;
		skyPal[i] = (unsigned short*)malloc(256 * sizeof(unsigned short));
		pal = skyPal[i];
		for (n = 0; n < 255; ++n) {
			int r = n - 48;
			int g = n - 32;
			int b = n + 32;

			r = (r * (PAL_SHADES - i - 1)) / (PAL_SHADES / 2) - 16;
			g = (g * (PAL_SHADES - i - 1)) / (PAL_SHADES / 2) - 32;
			b = (b * (PAL_SHADES - i - 1)) / (PAL_SHADES / 2) - 48;

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

	initSkyAndCloudTextures();

	hmap = malloc(HMAP_SIZE);
	viewNearPosVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));
	viewNearStepVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));
	yMaxHolder = malloc(VIS_HOR_STEPS * sizeof(int));


	if(initHeightmapAndColormap() == -1 || initDistMap() == -1) {
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
	free(hmap);
	free(cmap);
	free(pmap);
	free(shadeVoxOff);
	free(cloudTex);
	free(skyTex);
	free(distMap);
	free(petrubTab);

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

	setPoint2D(&dHor, (pr.x - pl.x) / (VIS_HOR_STEPS - 1), (pr.y - pl.y) / (VIS_HOR_STEPS - 1));
	for (i=0; i<VIS_HOR_STEPS; ++i) {
		setPoint2D(viewStepVec++, (VIS_VER_SKIP * pl.x) >> FP_BASE, (VIS_VER_SKIP * pl.y) >> FP_BASE);
		setPoint2D(viewPosVec++, (VIS_NEAR * pl.x) >> FP_BASE, (VIS_NEAR * pl.y) >> FP_BASE);

		pl.x += dHor.x;
		pl.y += dHor.y;
	}
}

static uint16_t reflectSky(int px, int py, int dvx, int dvy, int vh, int petrubation)
{
	int u = (((px + ((dvx * vh) >> FP_SCALE)) >> (FP_SCALE - 7)) + petrubation) & (SKY_TEX_WIDTH - 1);
	int v = (((py + ((dvy * vh) >> FP_SCALE)) >> (FP_SCALE - 7)) + petrubation) & (SKY_TEX_HEIGHT - 1);

	return skyPal[(int)((PAL_SHADES-1) * REFLECT_SHADE) + (petrubation>>1)][skyTex[v * SKY_TEX_WIDTH + u]];
}


static uint16_t reflectSample(int px, int py, int dvx, int dvy, int ph, int dh, int remainingSteps, uint16_t* pal, int petrubation)
{
	int i;

	int vx = px;
	int vy = py;

	int sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
	int mapOffset = sampleOffset & (HMAP_SIZE - 1);
	for (i = 0; i < remainingSteps;) {
		const int safeSteps = distMap[mapOffset];

		if (safeSteps == 0) {
			return reflectSky(px, py, dvx, dvy, dh, petrubation>>1);
		} else {
			vx += safeSteps * dvx;
			vy += safeSteps * dvy;
			ph += safeSteps * dh;

			sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
			mapOffset = sampleOffset & (HMAP_SIZE - 1);

			if ((ph >> FP_SCALE) < hmap[mapOffset]) {
				return pal[cmap[mapOffset] + (petrubation >> 2) * 256];
			}
			i += safeSteps;
		}
	}

	return reflectSky(px, py, dvx, dvy, dh, petrubation>>1);
}

static void drawColumn(int hCount, uint16_t cv, uint16_t* dst)
{
	do {
		*dst = cv;
		dst -= FB_WIDTH;
	} while (--hCount > 0);
}

static void drawColumnHaze(int hCount, uint16_t cv, uint16_t* dst)
{
	const int hi = haze;

	const int cvR = (cv >> 11) & 31;
	const int cvG = (cv >> 5) & 63;
	const int cvB = cv & 31;
	do {
		const uint16_t bg = *dst;
		const int bgR = (bg >> 11) & 31;
		const int bgG = (bg >> 5) & 63;
		const int bgB = bg & 31;
		const int r = (bgR * hi + cvR * ((VIS_FAR - VIS_HAZE) - hi)) / (VIS_FAR - VIS_HAZE);
		const int g = (bgG * hi + cvG * ((VIS_FAR - VIS_HAZE) - hi)) / (VIS_FAR - VIS_HAZE);
		const int b = (bgB * hi + cvB * ((VIS_FAR - VIS_HAZE) - hi)) / (VIS_FAR - VIS_HAZE);
		*dst = (r << 11) | (g << 5) | b;
		dst -= FB_WIDTH;
	} while (--hCount > 0);
}

static void drawColumn2X(int hCount, uint16_t cv, uint16_t* dst)
{
	const uint32_t cv32 = (cv << 16) | cv;
	uint32_t* dst32 = (uint32_t*)dst;

	do {
		*dst32 = cv32;
		dst32 -= FB_WIDTH / 2;
	} while (--hCount > 0);
}

static void drawColumn4X(int hCount, uint16_t cv, uint16_t* dst)
{
	const uint32_t cv32 = (cv << 16) | cv;
	uint32_t* dst32 = (uint32_t*)dst;

	do {
		*dst32 = cv32;
		*(dst32 + 1) = cv32;
		dst32 -= FB_WIDTH / 2;
	} while (--hCount > 0);
}

static int setupPixStepColumn(int i)
{
	int j;

	// In transitions from close to mid to far, clone the yMaxHolder for nearby columns 
	if (i == VIS_CLOSE) {
		for (j = 0; j < VIS_HOR_STEPS; j += 4) {
			const int yMax = yMaxHolder[j];
			yMaxHolder[j + 1] = yMax;
			yMaxHolder[j + 2] = yMax;
			yMaxHolder[j + 3] = yMax;
		}
	}
	else if (i == VIS_MID) {
		for (j = 0; j < VIS_HOR_STEPS; j += 2) {
			const int yMax = yMaxHolder[j];
			yMaxHolder[j + 1] = yMax;
		}
	}

	if (i < VIS_CLOSE) {
		drawColumnFunc = drawColumn4X;
		return 4;
	}
	else if (i < VIS_MID) {
		drawColumnFunc = drawColumn2X;
		return 2;
	}
	else if (i < VIS_HAZE) {
		drawColumnFunc = drawColumn;
	}
	else {
		drawColumnFunc = drawColumnHaze;
		haze = (3 * (i - VIS_HAZE)) / 2;
		CLAMP(haze, 0, VIS_FAR - VIS_HAZE - 1)
	}
	return 1;
}

static void renderScape(int petrT)
{
	int i, j;

	const int playerHeight = viewPos.y >> FP_VIEWER;

	const Point2D* posVecL = &viewNearPosVec[0];
	const Point2D* posVecR = &viewNearPosVec[VIS_HOR_STEPS - 1];
	const Point2D* stepVecL = &viewNearStepVec[0];
	const Point2D* stepVecR = &viewNearStepVec[VIS_HOR_STEPS - 1];
	const int vPosX = viewPos.x << (FP_SCAPE - FP_VIEWER);
	const int vPosY = viewPos.z << (FP_SCAPE - FP_VIEWER);
	int vxL = posVecL->x + vPosX;
	int vxR = posVecR->x + vPosX;
	int vyL = posVecL->y + vPosY;
	int vyR = posVecR->y + vPosY;
	const int dvxL = stepVecL->x;
	const int dvxR = stepVecR->x;
	const int dvyL = stepVecL->y;
	const int dvyR = stepVecR->y;

	uint16_t* dstBase = (uint16_t*)fb_pixels + (FB_HEIGHT - 1) * FB_WIDTH;

	memset(yMaxHolder, 0, VIS_HOR_STEPS * sizeof(int));

	for (i = 0; i < VIS_VER_STEPS; ++i) {
		const int pixStep = setupPixStepColumn(i);
		const int dvx = pixStep * ((vxR - vxL) / VIS_HOR_STEPS);
		const int dvy = pixStep * ((vyR - vyL) / VIS_HOR_STEPS);
		int vx = vxL;
		int vy = vyL;

		const uint16_t* pmapPtr = pmap +shadeVoxOff[i];
		uint16_t* pmapPtrShade;
		const int heightScale = heightScaleTab[i];

		int shadePalI = i + (int)((VIS_VER_STEPS - 1) * REFLECT_SHADE);
		CLAMP(shadePalI, 0, VIS_VER_STEPS-1)
		pmapPtrShade = pmap + shadeVoxOff[shadePalI];

		for (j = 0; j < VIS_HOR_STEPS; j+= pixStep) {
			const int yMax = yMaxHolder[j];
			if (yMax <= FB_HEIGHT) {
				const int sampleOffset = (vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE);
				const int mapOffset = sampleOffset & (HMAP_SIZE - 1);
				const int hm = hmap[mapOffset];
				int h = (((-playerHeight + hm) * heightScale) >> (FP_REC - V_HEIGHT_SCALER_SHIFT)) + HORIZON;
				if (h > FB_HEIGHT + 1) h = FB_HEIGHT + 1;

				if (yMax < h) {
					uint16_t cv;
					int yH, hCount;
					uint16_t* dst;
					if (hm > SEA_LEVEL) {
						cv = pmapPtr[cmap[mapOffset]];
					} else {
						const int petrubation = petrubTab[(((petrubTab[j >> 1] + i) >> 1) + petrT) & (PETRUB_SIZE - 1)];
						const int dh = ((playerHeight + petrubation - hm) << FP_SCALE) / (i + 1);
						const int dvxH = viewNearStepVec[j].x;
						const int dvyH = viewNearStepVec[j].y;
						cv = reflectSample(vx, vy, dvxH, dvyH, hm << FP_SCALE, dh, VIS_VER_STEPS - i, pmapPtrShade, petrubation);
					}

					yH = yMaxHolder[j];
					hCount = h - yH;
					yMaxHolder[j] = h;
					dst = dstBase - (yH -1) * FB_WIDTH + j;

					drawColumnFunc(hCount, cv, dst);
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
	setViewPos(780,V_PLAYER_HEIGHT,675);
	setViewAngle(0,4*SIN_LENGTH/5,0);

	prevTime = time_msec;
}

static unsigned char getVoxelHeightUnderPlayer()
{
	const int vx = viewPos.x >> FP_VIEWER;
	const int vy = viewPos.z >> FP_VIEWER;

	const int sampleOffset = vy * HMAP_WIDTH + vx;
	const int mapOffset = sampleOffset & (HMAP_SIZE - 1);

	return hmap[mapOffset];
}

static void move(int dt)
{
	const int speedX = (dt << FP_VIEWER) >> 8;
	const int speedZ = (dt << FP_VIEWER) >> 8;

	const int velX = (speedX * isin[(viewAngle.y + SIN_TO_COS) & (SIN_LENGTH-1)]) >> FP_BASE;
	const int velZ = (speedZ * isin[viewAngle.y & (SIN_LENGTH-1)]) >> FP_BASE;

	/* printf("%d %d %d\n", viewPos.x >> FP_VIEWER, viewPos.z >> FP_VIEWER, viewAngle.y); */

	if (kb_isdown('w')) {
		viewPos.x += velX;
		viewPos.z += velZ;
	}

	if (kb_isdown('s')) {
		viewPos.x -= velX;
		viewPos.z -= velZ;
	}

	if (kb_isdown('a')) {
		viewAngle.y -= speedX;
	}

	if (kb_isdown('d')) {
		viewAngle.y += speedX;
	}

	if(mouse_bmask & MOUSE_BN_LEFT) {
		viewAngle.y = (4*mouse_x) & (SIN_LENGTH-1);
	}
	if(mouse_bmask & MOUSE_BN_RIGHT) {
		viewPos.y = mouse_y << FP_VIEWER;
	}

	#ifdef LOCK_PLAYER_TO_GROUND
		viewPos.y = (getVoxelHeightUnderPlayer() + V_PLAYER_HEIGHT) << FP_VIEWER;
	#endif

	prevTime = time_msec;

	updateRaySamplePosAndStep();
}

static void renderBitmapLineSky(int u, int du, int v, int dv, const unsigned char* src, const unsigned short* pal, unsigned short* dst)
{
	int length = FB_WIDTH;
	while (length-- > 0) {
		int tu = (u >> FP_SCALE) & (SKY_TEX_WIDTH - 1);
		int tv = (v >> FP_SCALE) & (SKY_TEX_HEIGHT - 1);
		*dst++ = pal[src[tv * SKY_TEX_WIDTH + tu]];
		u += du;
		v += dv;
	};
}

static void rotatePoint2D(int *x, int *y, float angle)
{
	float rx = cos(angle);
	float ry = sin(angle);

	float xp = (int)*x;
	float yp = (int)*y;

	*x = (int)(xp * rx - yp * ry);
	*y = (int)(xp * ry + yp * rx);
}

static void renderSky()
{
	unsigned short* dst = (unsigned short*)fb_pixels;
	int y;

	static float angleTest = 0.0f;

	angleTest = ((float)(viewAngle.y - SIN_LENGTH / 4) / SIN_LENGTH) * -2.0f * 3.1415926536f;

	for (y = 0; y < FB_HEIGHT / 2; ++y) {
		int z, u, v, du, dv;
		int palNum = (PAL_SHADES * y) / (FB_HEIGHT / 2);

		int yp = FB_HEIGHT / 2 - y;
		if (yp == 0) yp = 1;
		z = (SKY_HEIGHT * PROJ_MUL) / yp;
		u = 0;
		v = z << (FP_SCALE - 7);
		du = 2 * z;
		dv = 0;

		rotatePoint2D(&u, &v, angleTest);
		rotatePoint2D(&du, &dv, angleTest);

		u += (-FB_WIDTH / 2) * du;
		v += (-FB_WIDTH / 2) * dv;

		CLAMP(palNum, 0, PAL_SHADES - 1)
			renderBitmapLineSky(u, du, v, dv, skyTex, skyPal[palNum], dst);

		dst += FB_WIDTH;
	}

	for (y = FB_HEIGHT / 2; y < FB_HEIGHT / 2 + FB_HEIGHT / 2; ++y) {
		uint32_t* dst32;
		uint32_t c;
		const unsigned short* farPal;
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

static void testRenderDistMap()
{
	int x, y;

	uint16_t* dst = fb_pixels;
	for (y=0; y<FB_HEIGHT; ++y) {
		for (x=0; x<FB_WIDTH; ++x) {
			int c = (distMap[(y & (HMAP_HEIGHT - 1)) * HMAP_WIDTH + (x & (HMAP_WIDTH - 1))] * 256) / DIST_RADIUS;
			CLAMP(c,0,255)
			if (c == 0) c = 255;
			*dst++ = PACK_RGB16(c, c, c);
		}
	}
}

static void moveClouds(int dt)
{
	static int skyMove1 = 0;
	static int skyMove2 = 0;

	unsigned int* dst = (unsigned int*)skyTex;

	const int skyMoveOff1 = skyMove1 >> FP_VIEWER;
	const int skyMoveOff2 = skyMove2 >> FP_VIEWER;

	int x,y;
	for (y = 0; y < SKY_TEX_HEIGHT; ++y) {
		const int y1 = (y + skyMoveOff1) & (SKY_TEX_HEIGHT - 1);
		const int y2 = (y + skyMoveOff2) & (SKY_TEX_HEIGHT - 1);
		unsigned int* src1 = (unsigned int*)&cloudTex[y1 * SKY_TEX_WIDTH];
		unsigned int* src2 = (unsigned int*)&cloudTex[y2 * SKY_TEX_WIDTH];

		for (x = 0; x < SKY_TEX_WIDTH / (4*4); ++x) {
			*dst = *src1 + *src2;
			*(dst + 1) = *(src1 + 1) + *(src2 + 1);
			*(dst + 2) = *(src1 + 2) + *(src2 + 2);
			*(dst + 3) = *(src1 + 3) + *(src2 + 3);
			dst += 4;
			src1 += 4;
			src2 += 4;
		}
	}

	skyMove1 += 4*dt;
	skyMove2 += 8*dt;
}

static void draw(void)
{
	const int dt = time_msec - prevTime;
	const int petrT = time_msec >> 6;

	move(dt);

	renderSky();

	renderScape(petrT);

	/* testRenderDistMap(); */

	moveClouds(dt);

	swap_buffers(0);
}
