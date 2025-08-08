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

#include "assfile/assfile.h"

#include "dos/keyb.h"

/* Comment out if you wanted to move by input instead of animated path for the demo */
#define MOVE_DEMO_PATH_ON

#define FP_VIEWER 8
#define FP_BASE 12
#define FP_SCAPE 10
#define FP_REC 18

#define SIN_SHIFT 12
#define SIN_LENGTH (1 << SIN_SHIFT)
#define SIN_TO_COS (SIN_LENGTH / 4)

#define FOV 48
#define VIS_NEAR 16
#define VIS_CLOSE 32
#define VIS_MID 128
#define VIS_FAR 240
#define VIS_HAZE (VIS_FAR - 32)

#define HAZE_BLEND_SHIFT 6
#define HAZE_BLEND_STEPS (1 << HAZE_BLEND_SHIFT)

#define PIXEL_SIZE 1
#define PIXEL_ABOVE (FB_WIDTH / PIXEL_SIZE)
#define VIS_VER_SKIP 1
#define PAL_SHADES 32

#define SKY_HEIGHT 8192
#define FP_SCALE 16

#define VIS_VER_STEPS ((VIS_FAR - VIS_NEAR) / VIS_VER_SKIP)
#define VIS_HOR_STEPS (FB_WIDTH / PIXEL_SIZE)

/*#define LOCK_PLAYER_TO_GROUND */

#define FLY_HEIGHT 96
#define V_PLAYER_HEIGHT 64
#define V_CLAMP_LOW 32
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

/* Let's minify the distmap */
#define DMAP_SHIFT 2
#define DMAP_WIDTH (HMAP_WIDTH >> DMAP_SHIFT)
#define DMAP_HEIGHT (HMAP_HEIGHT >> DMAP_SHIFT)
#define DMAP_SIZE (DMAP_WIDTH * DMAP_HEIGHT)

typedef struct Point2D
{
	int x,y;
} Point2D;

#define VOX_PATH_POINTS 44
#define LOW_TEST (6144 + 1024)

static int posAnglePath[4 * VOX_PATH_POINTS] = {
	355840, 86784, 31744, 2179, 333548, 64416, 27131, 2179, 310209, 40688, 22290, 2179, 293348, 31344, 18784, 2179,
	268876, 31344, 13729, 2146, 245521, 30272, 13300, 2053, 225825, 10976, 13038, 2053, 204737, 10976, 12759, 2103,
	187953, 21104, 12537, 2153, 167988, 32784, 8320, 2266, 145491, 37232, 15, 2341, 130586, 40368, -12663, 2557,
	115498, 42864, -27454, 2151, 96955, 42864, -32543, 1757, 77793, 42864, -25594, 1364, 72368, 42864, -9793, 1156,
	73833, 42864, 15375, 1012, 78150, 52352, 30051, 950, 84070, 58736, 46567, 847, 91438, 62800, 68693, 847,
	98483, 52784, 94243, 847, 106377, 37328, 116963, 608, 129557, 30864, 135794, 434, 147895, 23840, 147170, 284,
	168229, 18928, 156617, 284, 183247, 17056, 165098, 375,
	209765, LOW_TEST, 184370, 406, 209765, LOW_TEST, 184370, 637, 209765, LOW_TEST, 184370, 1049, 209765, LOW_TEST, 184370, 1627,
	209765, LOW_TEST, 184370, 1871, 209765, LOW_TEST, 184370, 2169, 209765, LOW_TEST, 184370, 2441,
	209765, LOW_TEST, 184370, 2701, 209765, LOW_TEST, 184370, 2924, 209765, LOW_TEST, 184370, 3015, 209765, LOW_TEST, 184370, 3015,
	209749, LOW_TEST, 184194, 3015, 208623, LOW_TEST, 171858, 3015, 207446, 9888, 158914, 3015, 205652, 29024, 139778, 3015,
	204253, 43904, 124898, 3015, 204110, 54352, 100308, 3015, 204110, 69352, 78813, 3015 };

typedef struct PathVals
{
	int x, y, z, a;
} PathVals;

typedef struct Voxel
{
	unsigned char height;
	unsigned char color;
} Voxel;

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

static Voxel* vmap = NULL;

static int *shadeVoxOff;
static char* petrubTab;

static void(*drawColumnFunc)(int, uint16_t, uint16_t*);

static int haze;

static Vector3D viewPos;
static Vector3D viewAngle;

static int prevTime;

static unsigned char hazeMap[VIS_FAR - VIS_HAZE];

static dseq_event* ev_fade;
static dseq_event* ev_flypath;


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

static void initHazeMap()
{
	int i;
	for (i = VIS_HAZE; i < VIS_FAR; ++i) {
		float shade = ((float)(i - VIS_HAZE) / (float)(VIS_FAR - VIS_HAZE - 1)) * 1.5f;
		CLAMP(shade, 0, 1);
		hazeMap[i - VIS_HAZE] = (HAZE_BLEND_STEPS-1) - (unsigned char)(shade * (HAZE_BLEND_STEPS - 1));
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
	ass_file *f = ass_fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "voxscape: failed to open %s: %s\n", path, strerror(errno));
		return -1;
	}

	ass_fread(dst, 1, count, f);

	ass_fclose(f);
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
		if (imgass_load(&mapPic, "data/src/voxel/vxc1.png") == -1) {
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

static void copyHmapAndCmapToVmap()
{
	int i;

	unsigned char* srcH = hmap;
	unsigned char* srcC = cmap;
	Voxel* dst = vmap;

	for (i=0; i<HMAP_SIZE; ++i) {
		const unsigned char h = *srcH++;
		const unsigned char c = *srcC++;
		dst->height = h;
		dst->color = c;
		++dst;
	}
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
		if (imgass_load(&mapPic, "data/src/voxel/vxh1.png") == -1) {
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
	if (!vmap) vmap = malloc(HMAP_SIZE * sizeof(Voxel));
	if (!hmap) hmap = malloc(HMAP_SIZE);
	if (!cmap) cmap = malloc(HMAP_SIZE + 256 * 3);
	if (!pmap) pmap = (uint16_t*)malloc(512 * PAL_SHADES);

	if (loadAndConvertImgHeightmapPng() == -1) return -1;
	if (loadAndConvertImgColormapPng() == -1) return -1;

	copyHmapAndCmapToVmap();

	/* No need for these two anymore */
	free(hmap);
	free(cmap);

	return 0;
}

/* Ugly hack and can get artifact */
/* Also points out there could be a more optimal distance field, mine is just tweaked to something but not precise */
static void distmapHack()
{
	int count = DMAP_SIZE;
	unsigned char* dm = distMap;
	do {
		unsigned char c = *dm;

		if (c < 8) {
			c *= 1.5f;
		} else if (c < 16) {
			c *= 1.25f;
		} else {
			c *= 1.125f;
		}

		*dm++ = c;
	} while (--count != 0);
}

static int initDistMap()
{
	int i;

	#ifdef CALCULATE_DIST_MAP
		int x, y, r, n;
		int pixStep = 1 << DMAP_SHIFT;

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

	distMap = malloc(DMAP_SIZE);

	#ifdef CALCULATE_DIST_MAP
		i = 0;
		for (y = 0; y < HMAP_HEIGHT; y+=pixStep) {
			printf("%d ", y);
			for (x = 0; x < HMAP_WIDTH; x+=pixStep) {
				int xi, yi;
				int rMin = 255;
				for (yi = 0; yi < pixStep; ++yi) {
					for (xi = 0; xi < pixStep; ++xi) {
						const int h = hmap[(y+yi) * HMAP_WIDTH + x+xi];
						int hb = 16;
						int safeR = 0;
						for (r = 0; r < DIST_RADIUS; ++r) {
							const int count = rCount[r];
							Point2D* p = &distOffsets[r * MAX_POINTS_PER_RADIUS];
							for (n = 0; n < count; ++n) {
								const int xp = (x + xi + p[n].x) & (HMAP_WIDTH - 1);
								const int yp = (y + yi + p[n].y) & (HMAP_HEIGHT - 1);
								const int hCheck = hmap[yp * HMAP_WIDTH + xp] - (hb >> 1);
								if (h < hCheck) {
									safeR = r + 1;
									r = DIST_RADIUS;	/* hack to break again outside */
									break;
								}
							}
							++hb;
						}
						if (safeR > 0 && safeR < rMin) rMin = safeR;
					}
				}
				if (rMin == 255) rMin = 0;
				distMap[i++] = rMin;
			}
		}

		#ifdef SAVE_DIST_MAP
			writeMapFile("data/dmap1.bin", DMAP_SIZE, distMap);
		#endif

		free(rCount);
		free(distOffsets);
	#else
		if (readMapFile("data/dmap1.bin", DMAP_SIZE, distMap) == -1) {
			/* memset(distMap, 0, DMAP_SIZE); */ /* 0 will still instantly reflect the clouds but not the mountains which need more thorough steps */ 
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

	/*distmapHack(); */

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
	initHazeMap();
	createHeightScaleTab();

	initSkyAndCloudTextures();

	hmap = malloc(HMAP_SIZE);
	viewNearPosVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));
	viewNearStepVec = malloc(VIS_HOR_STEPS * sizeof(Point2D));
	yMaxHolder = malloc(VIS_HOR_STEPS * sizeof(int));


	if(initHeightmapAndColormap() == -1 || initDistMap() == -1) {
		return -1;
	}

	ev_fade = dseq_lookup("voxelscape.fade");
	ev_flypath = dseq_lookup("voxelscape.flypath");

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
	free(vmap);
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

	for (i = 0; i < remainingSteps;) {
		int distOffset = (vy >> (FP_SCAPE + DMAP_SHIFT)) * DMAP_WIDTH + (vx >> (FP_SCAPE + DMAP_SHIFT));
		int distMapOffset = distOffset & (DMAP_SIZE - 1);
		const int safeSteps = distMap[distMapOffset];

		if (safeSteps == 0) {
			return reflectSky(px, py, dvx, dvy, dh, petrubation>>1);
		} else {
			int h;
			Voxel* v;

			vx += safeSteps * dvx;
			vy += safeSteps * dvy;

			ph += safeSteps * dh;
			h = ph >> FP_SCALE;
			if (h > 96) break;	/* hack value */
			v = &vmap[((vy >> FP_SCAPE) * HMAP_WIDTH + (vx >> FP_SCAPE)) & (HMAP_SIZE - 1)];
			if (h < v->height) {
				return pal[v->color + (petrubation >> 2) * 256];
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
	const int hi1 = haze;
	const int hi0 = (HAZE_BLEND_STEPS - 1) - hi1;

	const uint32_t cvRB = cv & 0xf81f;
	const uint32_t cvG = cv & 0x7e0;
	do {
		const uint32_t bg = (uint32_t)*dst;
		const uint32_t bgRB = bg & 0xf81f;
		const uint32_t bgG = bg & 0x7e0;
		const uint32_t cRB = ((bgRB * hi0 + cvRB * hi1) >> HAZE_BLEND_SHIFT) & 0xf81f;
		const uint32_t cG = ((bgG * hi0 + cvG * hi1) >> HAZE_BLEND_SHIFT) & 0x7e0;
		*dst = cRB | cG;
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

	/* In transitions from close to mid to far, clone the yMaxHolder for nearby columns */
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
		haze = hazeMap[i - VIS_HAZE];
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
				Voxel* v = &vmap[sampleOffset & (HMAP_SIZE - 1)];
				const int hm = v->height;
				int h = (((-playerHeight + hm) * heightScale) >> (FP_REC - V_HEIGHT_SCALER_SHIFT)) + HORIZON;
				if (h > FB_HEIGHT + 1) h = FB_HEIGHT + 1;

				if (yMax < h) {
					uint16_t cv;
					int yH;
					if (hm > SEA_LEVEL) {
						cv = pmapPtr[v->color];
					} else {
						const int petrubation = petrubTab[(((petrubTab[j >> 1] + i) >> 1) + petrT) & (PETRUB_SIZE - 1)];
						const int dh = ((playerHeight + petrubation - hm) << FP_SCALE) / (i + 1);
						const int dvxH = viewNearStepVec[j].x;
						const int dvyH = viewNearStepVec[j].y;
						cv = reflectSample(vx, vy, dvxH, dvyH, hm << FP_SCALE, dh, VIS_VER_STEPS - i, pmapPtrShade, petrubation);
					}

					yH = yMaxHolder[j];
					yMaxHolder[j] = h;

					drawColumnFunc(h - yH, cv, dstBase - (yH - 1) * FB_WIDTH + j);
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

	/*setViewPos(355870 >> FP_VIEWER, 86880 >> FP_VIEWER, 31745 >> FP_VIEWER); */
	/*setViewAngle(0, 6275, 0); */

	/*setViewPos(217680 >> FP_VIEWER, 8192 >> FP_VIEWER, 19635 >> FP_VIEWER); */
	/*setViewAngle(0, 2352, 0); */

	prevTime = time_msec;
}

static unsigned char getVoxelHeightUnderPlayer()
{
	const int vx = viewPos.x >> FP_VIEWER;
	const int vy = viewPos.z >> FP_VIEWER;

	const int sampleOffset = vy * HMAP_WIDTH + vx;
	const int mapOffset = sampleOffset & (HMAP_SIZE - 1);

	return vmap[mapOffset].height;
}

/*static int interpolate(int v0, int v1, float t)
{
	return (int)((1.0f - t) * (float)v0 + t * (float)v1);
}

static int interpolate2(int v0, int v1, int v2, float t)
{
	float a = (1.0f - t) * (float)v0 + t * (float)v1;
	float b = (1.0f - t) * (float)v1 + t * (float)v2;
	return (int)((1.0f - t) * a + t * b);
}

static int interpolate3(int v0, int v1, int v2, int v3, float t)
{
	float a = (1.0f - t) * (float)v0 + t * (float)v1;
	float b = (1.0f - t) * (float)v1 + t * (float)v2;
	float c = (1.0f - t) * (float)v2 + t * (float)v3;
	float d = (1.0f - t) * a + t * b;
	float e = (1.0f - t) * b + t * c;
	return (int)((1.0f - t) * d + t * e);
}


static void moveThroughPath()
{
	PathVals* pVals = (PathVals*)posAnglePath;
	PathVals* pv0;
	PathVals* pv1;
	PathVals* pv2;
	PathVals* pv3;

	float ft = dseq_value(ev_flypath);
	float tp = ft * 18;

	float t = (float)fmod(tp, 1.0f);

	int i0 = 3 * (int)tp;
	int i1 = i0 + 1;
	int i2 = i0 + 2;
	int i3 = i0 + 3;
	CLAMP(i0, 0, VOX_PATH_POINTS - 1);
	CLAMP(i1, 0, VOX_PATH_POINTS - 1);
	CLAMP(i2, 0, VOX_PATH_POINTS - 1);
	CLAMP(i3, 0, VOX_PATH_POINTS - 1);

	pv0 = &pVals[i0];
	pv1 = &pVals[i1];
	pv2 = &pVals[i2];
	pv3 = &pVals[i3];

	viewPos.x = interpolate3(pv0->x, pv1->x, pv2->x, pv3->x, t);
	viewPos.y = interpolate3(pv0->y, pv1->y, pv2->y, pv3->y, t);
	viewPos.z = interpolate3(pv0->z, pv1->z, pv2->z, pv3->z, t);
	viewAngle.y = interpolate3(pv0->a, pv1->a, pv2->a, pv3->a, t) & (SIN_LENGTH - 1);

	prevTime = time_msec;
}*/

static void moveThroughPath2()
{
	PathVals* pVals = (PathVals*)posAnglePath;
	PathVals* pv0;
	PathVals* pv1;
	PathVals* pv2;
	PathVals* pv3;

	float ft = dseq_value(ev_flypath);
	float tp = ft * 42;

	float t = (float)fmod(tp, 1.0f);

	int i0 = (int)tp;
	int i1 = i0 + 1;
	int i2 = i0 + 2;
	int i3 = i0 + 3;
	CLAMP(i0, 0, VOX_PATH_POINTS - 1);
	CLAMP(i1, 0, VOX_PATH_POINTS - 1);
	CLAMP(i2, 0, VOX_PATH_POINTS - 1);
	CLAMP(i3, 0, VOX_PATH_POINTS - 1);

	pv0 = &pVals[i0];
	pv1 = &pVals[i1];
	pv2 = &pVals[i2];
	pv3 = &pVals[i3];

	viewPos.x = cgm_bspline(pv0->x, pv1->x, pv2->x, pv3->x, t);
	viewPos.y = cgm_bspline(pv0->y, pv1->y, pv2->y, pv3->y, t);
	viewPos.z = cgm_bspline(pv0->z, pv1->z, pv2->z, pv3->z, t);
	viewAngle.y = (int)cgm_bspline(pv0->a, pv1->a, pv2->a, pv3->a, t) & (SIN_LENGTH - 1);

	prevTime = time_msec;
}

static void move(int dt)
{
	static int pI = 0;

	const int speedX = (dt << FP_VIEWER) >> 8;
	const int speedZ = (dt << FP_VIEWER) >> 8;

	const int velX = (speedX * isin[(viewAngle.y + SIN_TO_COS) & (SIN_LENGTH-1)]) >> FP_BASE;
	const int velZ = (speedZ * isin[viewAngle.y & (SIN_LENGTH-1)]) >> FP_BASE;

	static int pJustPressed = 0;

	if (kb_isdown('w')) {
		viewPos.x += velX;
		viewPos.z += velZ;
	}

	if (kb_isdown('s') || kb_isdown('x')) {
		viewPos.x -= velX;
		viewPos.z -= velZ;
	}

	if (kb_isdown('a')) {
		viewAngle.y -= speedX;
	}

	if (kb_isdown('d')) {
		viewAngle.y += speedX;
	}

	if (kb_isdown('q')) {
		viewPos.y += (speedX << 4);
	}
	if (kb_isdown('c')) {
		viewPos.y -= (speedX << 4);
		if (viewPos.y < (V_CLAMP_LOW << FP_VIEWER)) viewPos.y = (V_CLAMP_LOW << FP_VIEWER);
	}

	if (kb_isdown('p')) {
		if (pJustPressed == 0) {
			printf("%d, %d, %d, %d, ", viewPos.x, viewPos.y, viewPos.z, viewAngle.y);
			++pI;
			if ((pI & 3)==0) {
				printf("\n");
			}
		}
		pJustPressed = 1;
	} else {
		pJustPressed = 0;
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
			int c = (distMap[(y & (DMAP_HEIGHT - 1)) * DMAP_WIDTH + (x & (DMAP_WIDTH - 1))] * 256) / DIST_RADIUS;
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
	float fade;

#ifdef MOVE_DEMO_PATH_ON
	moveThroughPath2();
	fade = dseq_value(ev_fade);
#else
	move(dt);
	fade = 1.0f;
#endif

	updateRaySamplePosAndStep();

	renderSky();

	renderScape(petrT);

	/* testRenderDistMap(); */

	moveClouds(dt);

	fadeToBlack16bpp(fade, fb_pixels, FB_WIDTH, FB_HEIGHT, FB_WIDTH);

	swap_buffers(0);
}
