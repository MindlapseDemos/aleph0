#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "noise.h"
#include "gfxutil.h"
#include "image.h"

#include "opt_rend.h"
#include "util.h"

static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"bump",
	init,
	destroy,
	start,
	0,
	draw
};


#define NUM_BIG_LIGHTS 3
#define BIG_LIGHT_WIDTH 256
#define BIG_LIGHT_HEIGHT BIG_LIGHT_WIDTH

#define BIGGER_LIGHT_WIDTH 384
#define BIGGER_LIGHT_HEIGHT BIGGER_LIGHT_WIDTH

#define NUM_PARTICLES 48
#define PARTICLE_LIGHT_WIDTH 16
#define PARTICLE_LIGHT_HEIGHT 16

#define HMAP_WIDTH 256
#define HMAP_HEIGHT 256

#define LMAP_WIDTH 512
#define LMAP_HEIGHT 512
#define LMAP_OFFSET_X ((LMAP_WIDTH - FB_WIDTH) / 2)
#define LMAP_OFFSET_Y ((LMAP_HEIGHT - FB_HEIGHT) / 2)

#define FAINT_LMAP_WIDTH 256
#define FAINT_LMAP_HEIGHT 256

static unsigned char* faintLmapTex;


static unsigned char electroPathData[] = {NUM_PARTICLES,
        6, 117,5,4,3, 114,5,3,4, 110,1,2,6, 110,251,1,3, 113,248,0,7, 120,248,255,7,
        27, 250,5,0,19, 13,5,7,13, 26,18,6,6, 26,24,7,5, 31,29,0,15, 46,29,7,2, 48,31,0,32, 80,31,7,10, 90,41,6,8, 90,49,7,8, 98,57,6,4, 98,61,7,4, 102,65,0,14, 116,65,1,2, 118,63,2,27, 118,36,3,5, 113,31,4,3, 110,31,3,12, 98,19,2,30, 98,245,1,11, 109,234,2,16, 109,218,1,8, 117,210,0,26, 143,210,1,12, 155,198,2,38, 155,160,1,4, 159,156,255,4,
        2, 130,9,6,48, 130,57,255,48,
        8, 84,5,3,4, 80,1,2,6, 80,251,1,7, 87,244,2,16, 87,228,1,52, 139,176,2,4, 139,172,1,6, 145,166,255,6,
        2, 46,5,2,13, 46,248,255,13,
        7, 39,13,0,3, 42,13,7,10, 52,23,0,32, 84,23,7,14, 98,37,6,8, 98,45,7,8, 106,53,255,8,
        12, 121,14,4,12, 109,14,3,3, 106,11,2,18, 106,249,1,11, 117,238,2,16, 117,222,1,4, 121,218,0,48, 169,218,1,6, 175,212,2,40, 175,172,3,2, 173,170,2,14, 173,156,255,14,
        9, 55,14,2,2, 55,12,1,5, 60,7,2,16, 60,247,1,11, 71,236,2,16, 71,220,1,38, 109,182,2,18, 109,164,3,8, 101,156,255,8,
        7, 65,14,2,22, 65,248,1,10, 75,238,2,16, 75,222,1,40, 115,182,2,16, 115,166,1,4, 119,162,255,4,
        9, 249,19,7,6, 255,25,0,17, 16,25,7,2, 18,27,6,14, 18,41,7,8, 26,49,0,40, 66,49,7,4, 70,53,0,3, 73,53,255,3,
        9, 228,23,2,30, 228,249,1,7, 235,242,2,48, 235,194,3,8, 227,186,4,13, 214,186,3,6, 208,180,2,23, 208,157,3,1, 207,156,255,1,
        16, 249,29,0,19, 12,29,7,2, 14,31,6,12, 14,43,7,10, 24,53,0,40, 64,53,7,6, 70,59,0,12, 82,59,7,4, 86,63,6,4, 86,67,7,10, 96,77,0,34, 130,77,1,12, 142,65,2,70, 142,251,1,5, 147,246,0,19, 166,246,255,19,
        22, 30,33,0,14, 44,33,7,2, 46,35,0,32, 78,35,7,8, 86,43,6,8, 86,51,7,8, 94,59,6,4, 94,63,7,6, 100,69,0,18, 118,69,1,4, 122,65,2,44, 122,21,3,2, 120,19,4,15, 105,19,3,3, 102,16,2,25, 102,247,1,11, 113,236,2,16, 113,220,1,6, 119,214,0,46, 165,214,1,2, 167,212,2,33, 167,179,255,33,
        11, 39,39,0,37, 76,39,7,6, 82,45,6,8, 82,53,7,8, 90,61,6,4, 90,65,7,8, 98,73,0,30, 128,73,1,10, 138,63,2,75, 138,244,1,6, 144,238,255,6,
        6, 69,45,4,41, 28,45,3,6, 22,39,2,16, 22,23,3,14, 8,9,4,8, 0,9,255,8,
        16, 186,49,6,2, 186,51,7,4, 190,55,0,24, 214,55,7,7, 221,62,6,20, 221,82,7,3, 224,85,0,10, 234,85,1,3, 237,82,2,11, 237,71,1,7, 244,64,0,23, 11,64,7,45, 56,109,0,105, 161,109,1,8, 169,101,2,3, 169,98,255,3,
        5, 196,49,7,2, 198,51,0,22, 220,51,7,7, 227,58,6,3, 227,61,255,3,
        6, 217,46,0,13, 230,46,7,5, 235,51,6,15, 235,66,5,6, 229,72,6,2, 229,74,255,2,
        4, 27,62,6,2, 27,64,7,33, 60,97,0,7, 67,97,255,7,
        10, 86,82,6,10, 86,92,7,5, 91,97,0,30, 121,97,1,2, 123,95,2,4, 123,91,1,6, 129,85,0,14, 143,85,7,14, 157,99,6,1, 157,100,255,1,
        23, 140,80,0,7, 147,80,7,7, 154,87,0,48, 202,87,7,9, 211,96,0,2, 213,96,1,3, 216,93,2,28, 216,65,3,6, 210,59,4,26, 184,59,3,8, 176,51,2,35, 176,16,1,4, 180,12,0,27, 207,12,1,4, 211,8,2,16, 211,248,1,12, 223,236,2,34, 223,202,3,4, 219,198,4,36, 183,198,3,4, 179,194,2,27, 179,167,1,1, 180,166,255,1,
        6, 81,88,6,6, 81,94,7,7, 88,101,0,40, 128,101,1,5, 133,96,2,1, 133,95,255,1,
        4, 189,98,6,3, 189,101,5,12, 177,113,4,76, 101,113,255,76,
        9, 145,101,5,4, 141,105,4,81, 60,105,3,46, 14,59,4,22, 248,59,3,8, 240,51,2,55, 240,252,1,7, 247,245,0,10, 1,245,255,10,
        3, 213,111,5,10, 203,121,4,92, 111,121,255,92,
        5, 197,113,5,4, 193,117,4,175, 18,117,5,10, 8,127,4,12, 252,127,255,12,
        4, 2,119,4,13, 245,119,5,6, 239,125,4,136, 103,125,255,136,
        7, 95,129,0,144, 239,129,7,8, 247,137,0,19, 10,137,1,16, 26,121,0,47, 73,121,7,5, 78,126,255,5,
        7, 44,132,4,15, 29,132,5,18, 11,150,4,19, 248,150,3,13, 235,137,4,171, 64,137,5,4, 60,141,255,4,
        6, 38,141,0,9, 47,141,7,9, 56,150,0,26, 82,150,1,9, 91,141,0,52, 143,141,255,52,
        4, 2,143,4,11, 247,143,3,10, 237,133,4,102, 135,133,255,102,
        11, 223,146,0,5, 228,146,7,3, 231,149,6,15, 231,164,7,7, 238,171,6,5, 238,176,7,14, 252,190,0,9, 5,190,1,6, 11,184,2,11, 11,173,3,5, 6,168,255,5,
        12, 32,149,0,16, 48,149,7,6, 54,155,0,29, 83,155,1,10, 93,145,0,113, 206,145,7,27, 233,172,6,5, 233,177,7,17, 250,194,0,15, 9,194,1,6, 15,188,2,19, 15,169,255,19,
        8, 141,156,5,6, 135,162,6,12, 135,174,5,52, 83,226,6,16, 83,242,5,8, 75,250,6,11, 75,5,7,9, 84,14,255,9,
        14, 189,157,6,11, 189,168,7,6, 195,174,6,18, 195,192,7,2, 197,194,0,26, 223,194,7,4, 227,198,6,40, 227,238,5,11, 216,249,6,17, 216,10,5,7, 209,17,4,26, 183,17,5,2, 181,19,6,3, 181,22,255,3,
        6, 45,160,6,5, 45,165,7,12, 57,177,6,30, 57,207,5,4, 53,211,4,2, 51,211,255,2,
        8, 217,166,7,9, 226,175,6,4, 226,179,7,13, 239,192,6,52, 239,244,5,5, 234,249,6,40, 234,33,5,3, 231,36,255,3,
        6, 24,168,5,5, 19,173,6,19, 19,192,5,6, 13,198,4,21, 248,198,3,1, 247,197,255,1,
        8, 58,166,0,15, 73,166,1,4, 77,162,0,24, 101,162,7,2, 103,164,6,14, 103,178,5,4, 99,182,4,1, 98,182,255,1,
        6, 247,169,4,3, 244,169,3,8, 236,161,2,15, 236,146,3,5, 231,141,4,81, 150,141,255,81,
        6, 39,173,6,29, 39,202,5,4, 35,206,4,41, 250,206,5,4, 246,210,6,10, 246,220,255,10,
        12, 92,170,4,15, 77,170,5,10, 67,180,6,28, 67,208,5,10, 57,218,4,61, 252,218,3,2, 250,216,2,4, 250,212,1,2, 252,210,0,41, 37,210,1,10, 47,200,2,18, 47,182,255,18,
        7, 127,173,6,3, 127,176,5,48, 79,224,6,16, 79,240,5,9, 70,249,6,17, 70,10,7,4, 74,14,255,4,
        5, 31,178,7,3, 34,181,6,19, 34,200,5,2, 32,202,4,41, 247,202,255,41,
        7, 145,184,5,22, 123,206,4,8, 115,206,5,10, 105,216,6,16, 105,232,5,11, 94,243,6,26, 94,13,255,26,
        18, 29,248,6,23, 29,15,7,8, 37,23,0,9, 46,23,7,4, 50,27,0,32, 82,27,7,12, 94,39,6,8, 94,47,7,8, 102,55,6,4, 102,59,7,2, 104,61,0,8, 112,61,1,2, 114,59,2,8, 114,51,3,12, 102,39,2,4, 102,35,3,8, 94,27,2,8, 94,19,255,8,
        9, 180,249,6,8, 180,1,5,3, 177,4,4,16, 161,4,5,4, 157,8,6,52, 157,60,7,7, 164,67,0,38, 202,67,7,4, 206,71,255,4,
        10, 191,249,6,9, 191,2,5,6, 185,8,4,19, 166,8,5,4, 162,12,6,17, 162,29,7,4, 166,33,6,18, 166,51,7,12, 178,63,0,26, 204,63,255,26
};


typedef struct Point2D {
	int x, y;
} Point2D;

typedef struct Edge {
	short xIn, xOut;
} Edge;

typedef struct PathPoint {
	unsigned char x,y,dir,pixelLength;
} PathPoint;

typedef struct PathPointTrack {
	PathPoint *pathPoint;
	int pixelWalk;
	int index;
	int numPoints;
} PathPointTrack;


static Point2D dir[8] = { {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}, {0,1}, {1,1} };

static PathPointTrack *pathPointTracks;
static Point2D moveLightmapOffset = {0,0};

static unsigned long startingTime;

static unsigned char *heightmap;
static unsigned short *lightmap;
static int *bumpOffset;
static int *bumpOffsetScreen;

static unsigned short *bigLight[NUM_BIG_LIGHTS];
static Point2D bigLightPoint[NUM_BIG_LIGHTS];
static Edge *bigLightEdges;

static unsigned short *biggerLight;
static Point2D biggerLightPoint;
static Edge *biggerLightEdges;

static unsigned short *particleLight;
static Point2D particlePoint[NUM_PARTICLES];
static Point2D electroidPoint[NUM_PARTICLES];

static int lightMinX = 0;
static int lightMaxX = LMAP_WIDTH-1;
static int lightMinY = 0;
static int lightMaxY = LMAP_HEIGHT-1;

static int numParticlesVisible = 0;
static int didElectroidsMovedYetOnce = 0;
static int startWaveBumpAndRGBlights = 0;

static dseq_event *ev_electroids;
static dseq_event *ev_scalerIn;
static dseq_event *ev_fade;


struct screen *bump_screen(void)
{
	return &scr;
}

static void initPathPoints()
{
	int i = 0;

	unsigned char *src = electroPathData;
	int numPaths = (int)*src++;

	pathPointTracks = (PathPointTrack*)malloc(numPaths * sizeof(PathPointTrack));

	do {
		int numPoints = *src++;
		pathPointTracks[i].pathPoint = (PathPoint*)src;
		pathPointTracks[i].pixelWalk = 0;
		pathPointTracks[i].index = 0;
		pathPointTracks[i].numPoints = numPoints;
		src += numPoints * sizeof(PathPoint);
	}while(++i < numPaths);
}

static int loadHeightMapTest()
{
	static struct img_pixmap bumpPic;

	unsigned int x, y, i = 0;
	uint32_t* src;
	int imgWidth, imgHeight;

	img_init(&bumpPic);
	if (imgass_load(&bumpPic, "data/bump/circ_col.png") == -1) {
		fprintf(stderr, "failed to load bump image\n");
		return -1;
	}

	src = (uint32_t*)bumpPic.pixels;
	imgWidth = bumpPic.width;
	imgHeight = bumpPic.width;

	for (y = 0; y < HMAP_HEIGHT; ++y) {
		for (x = 0; x < HMAP_WIDTH; ++x) {
			uint32_t c32 = src[((y & (imgHeight - 1)) * imgWidth) + (x & (imgWidth - 1))];
			int a = (c32 >> 24) & 255;
			int r = (c32 >> 16) & 255;
			int g = (c32 >> 8) & 255;
			int b = c32 & 255;
			int c = (a + r + g + b) / 4;
			heightmap[i++] = c;
		}
	}

	return 0;
}

static void generateBigLight(unsigned short *lightBitmap, Edge *lightEdges, unsigned int lightWidth, unsigned int lightHeight, int rgbIndex)
{
	const int lightRadius = lightWidth / 2;

	const float rgbMul[12] = { 	0.425, 0.225, 0.0,
								0.5, 0.0,    0.0,
								0.0,    0.5, 0.0,
								0.0,    0.0,    0.5 };

	const float *rgb = &rgbMul[3 * rgbIndex];

	unsigned int x, y;
	int c,r,g,b;
	for (y = 0; y < lightHeight; y++) {
		int yc = y - (lightHeight / 2);
		int xIn = -1;
		int xOut = -1;
		int lastC = 0;
		for (x = 0; x < lightWidth; x++) {
			int xc = x - (lightWidth / 2);
			float d = (float)sqrt(xc * xc + yc * yc);
			float invDist = ((float)lightRadius - (float)sqrt(xc * xc + yc * yc)) / (float)lightRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(invDist * 63);
			r = c >> 1;
			g = c;
			b = c >> 1;

			if (lastC == 0 && c > 0) {
				xIn = x;
			}
			if (lastC != 0 && c == 0) {
				xOut = x;
			}
			lastC = c;

			*lightBitmap++ = ((int)(r * rgb[0]) << 11) | ((int)(g * rgb[1]) << 5) | (int)(b * rgb[2]);
		}
		lightEdges->xIn = xIn;
		lightEdges->xOut = xOut;
		++lightEdges;
	}
}

static void generateParticleLight()
{
	const int particleRadius = PARTICLE_LIGHT_WIDTH / 2;

	unsigned int x, y;
	int c;
	unsigned int i = 0;
	for (y = 0; y < PARTICLE_LIGHT_HEIGHT; y++) {
		int yc = y - (PARTICLE_LIGHT_HEIGHT / 2);
		for (x = 0; x < PARTICLE_LIGHT_WIDTH; x++) {
			int xc = x - (PARTICLE_LIGHT_WIDTH / 2);

			float invDist = ((float)particleRadius - (float)sqrt(xc * xc + yc * yc)) / (float)particleRadius;
			if (invDist < 0.0f) invDist = 0.0f;

			c = (int)(pow(invDist, 5.0f) * 29);
			CLAMP(c,0,29)
			particleLight[i++] = ((c >> 1) << 11) | (c << 5) | (c >> 1);
		}
	}
}

static void precalculateBumpInclination()
{
	unsigned int x, y, i = 0;

	for (y = 0; y < HMAP_HEIGHT; y++) {
		const int yp0 = y * HMAP_WIDTH;
		const int yp1 = ((y+1) & (HMAP_HEIGHT-1)) * HMAP_WIDTH;
		for (x = 0; x < HMAP_WIDTH; x++) {
			const int ii = yp0 + x;
			const int iRight = yp0 + ((x + 1) & (HMAP_WIDTH - 1));
			const int iDown = yp1 +x;
			const float offsetPower = 0.75f;

			const int dx = (int)((heightmap[ii] - heightmap[iRight]) * offsetPower);
			const int dy = (int)((heightmap[ii] - heightmap[iDown]) * offsetPower);
			const int di = dy * LMAP_WIDTH + dx;

			bumpOffset[i++] = di;
		}
	}
}

static void initFaintLightmapBackground()
{
	int x, y, i;
	const float scale = 1.0f / 32.0f;
	const int perX = (int)(scale * FAINT_LMAP_WIDTH);
	const int perY = (int)(scale * FAINT_LMAP_HEIGHT);

	faintLmapTex = (unsigned char*)malloc(FAINT_LMAP_WIDTH * FAINT_LMAP_HEIGHT);

	i = 0;
	for (y = 0; y < FAINT_LMAP_HEIGHT; ++y) {
		for (x = 0; x < FAINT_LMAP_WIDTH; ++x) {
			float f = pfbm2((float)x * scale, (float)y * scale, perX, perY, 8) + 0.25f;
			/*float f = pturbulence2((float)x * scale, (float)y * scale, perX, perY, 4); */
			CLAMP(f, 0.0f, 0.99f);
			faintLmapTex[i++] = (int)(f * 9.0f);
		}
	}
}

static int init(void)
{
	int i;

	const int hm_size = HMAP_WIDTH * HMAP_HEIGHT;
	const int lm_size = LMAP_WIDTH * LMAP_HEIGHT;


	heightmap = malloc(sizeof(*heightmap) * hm_size);
	lightmap = malloc(sizeof(*lightmap) * lm_size);
	bumpOffset = malloc(sizeof(*bumpOffset) * hm_size);
	bumpOffsetScreen = malloc(sizeof(*bumpOffsetScreen) * FB_WIDTH * FB_HEIGHT);

	for (i = 0; i < NUM_BIG_LIGHTS; i++) {
		bigLight[i] = malloc(sizeof(*bigLight[i]) * BIG_LIGHT_WIDTH * BIG_LIGHT_HEIGHT);
	}
	bigLightEdges = (Edge*)malloc(BIG_LIGHT_HEIGHT * sizeof(*bigLightEdges));

	biggerLight = malloc(sizeof(*biggerLight) * BIGGER_LIGHT_WIDTH * BIGGER_LIGHT_HEIGHT);
	biggerLightEdges = (Edge*)malloc(BIGGER_LIGHT_HEIGHT * sizeof(*biggerLightEdges));

	particleLight = malloc(sizeof(*particleLight) * PARTICLE_LIGHT_WIDTH * PARTICLE_LIGHT_HEIGHT);

	memset(lightmap, 0, sizeof(*lightmap) * lm_size);
	memset(bumpOffset, 0, sizeof(*bumpOffset) * hm_size);
	memset(particlePoint, 0, sizeof(*particlePoint) * NUM_PARTICLES);
	memset(electroidPoint, 0, sizeof(*electroidPoint) * NUM_PARTICLES);

	if (loadHeightMapTest() == -1) return - 1;

	initPathPoints();

	precalculateBumpInclination();

	for (i=0; i<NUM_BIG_LIGHTS; ++i) {
		generateBigLight(bigLight[i], bigLightEdges, BIG_LIGHT_WIDTH, BIG_LIGHT_HEIGHT, i+1);
	}
	generateBigLight(biggerLight, biggerLightEdges, BIGGER_LIGHT_WIDTH, BIGGER_LIGHT_HEIGHT, 0);

	generateParticleLight();

	initFaintLightmapBackground();

	ev_electroids = dseq_lookup("bump.electroids");
	ev_scalerIn = dseq_lookup("bump.scalerIn");
	ev_fade = dseq_lookup("bump.fade");

	return 0;
}

static void destroy(void)
{
	int i;

	free(heightmap);
	free(lightmap);
	free(bumpOffset);
	free(bumpOffsetScreen);
	free(particleLight);
	free(bigLightEdges);
	free(biggerLightEdges);

	for (i=0; i<NUM_BIG_LIGHTS; ++i) {
		free(bigLight[i]);
	}
	free(biggerLight);
}

static void start(long trans_time)
{
	startingTime = time_msec;
}


static void renderParticleLight(Point2D *p, int width, int height, unsigned short *light)
{
	int x, y, dx;
	unsigned short *src, *dst;

	int x0 = p->x;
	int y0 = p->y;
	int x1 = p->x + width;
	int y1 = p->y + height;

	int xl = 0;
	int yl = 0;

	if (x0 < 0) {
		xl = -x0;
		x0 = 0;
	}

	if (y0 < 0) {
		yl = -y0;
		y0 = 0;
	}

	if (x1 > FB_WIDTH) x1 = FB_WIDTH;
	if (y1 > FB_HEIGHT) y1 = FB_HEIGHT;

	dx = x1 - x0;

	dst = lightmap + (y0 + LMAP_OFFSET_Y) * LMAP_WIDTH + x0 + LMAP_OFFSET_X;
	src = light + yl * width + xl;

	for (y = y0; y < y1; y++) {
		for (x = x0; x < x1; x++) {
			*dst++ += *src++;
		}
		dst += LMAP_WIDTH - dx;
		src += width - dx;
	}
}

static void renderParticles()
{
	int i;
	for (i = 0; i < numParticlesVisible; i++) {
		renderParticleLight(&particlePoint[i], PARTICLE_LIGHT_WIDTH, PARTICLE_LIGHT_HEIGHT, particleLight);
	}
}

static void renderLight(unsigned short *lightBitmap, Edge *lightEdges, unsigned int lightWidth, unsigned int lightHeight, Point2D *lightPos, int justBlit)
{
	int y, yLine;
	unsigned short* src;
	unsigned short* dst;

	const int x0 = lightPos->x;
	int y0 = lightPos->y;
	int y1 = lightPos->y + lightHeight;

	CLAMP(y0, -LMAP_OFFSET_Y, LMAP_HEIGHT - LMAP_OFFSET_Y - 1);
	CLAMP(y1, -LMAP_OFFSET_Y, LMAP_HEIGHT - LMAP_OFFSET_Y - 1);

	dst = lightmap + (y0 + LMAP_OFFSET_Y) * LMAP_WIDTH + x0 + LMAP_OFFSET_X;
	src = lightBitmap;

	yLine = 0;
	if (justBlit==1) {
		for (y = y0; y < y1; y++) {
			const short xIn = lightEdges[yLine].xIn;
			if (xIn != -1) {
				const short xOut = lightEdges[yLine].xOut;
				memcpy(dst + xIn, src + xIn, 2 * (xOut - xIn));
			}
			dst += LMAP_WIDTH;
			src += lightWidth;
			yLine++;
		}
	} else {
		for (y = y0; y < y1; y++) {
			const short xIn = lightEdges[yLine].xIn;
			if (xIn != -1) {
				const short xOut = lightEdges[yLine].xOut;

				uint16_t* srcIn = src + xIn;
				uint16_t* dstIn = dst + xIn;

				uint32_t* src32, * dst32;
				int count;

				int xp0 = x0 + xIn;
				int xp1 = x0 + xOut - 1;

				if (xp0 & 1) {
					*dstIn++ |= *srcIn++;
					++xp0;
				}

				src32 = (uint32_t*)srcIn;
				dst32 = (uint32_t*)dstIn;
				count = (xp1 - xp0) >> 1;
				while (count-- != 0) {
#if defined(__mips) || defined(__sparc) || defined(__sparc__)
					uint32_t val;
					read_unaligned32(&val, src32++);
					*dst32++ |= val;
#else
					*dst32++ |= *src32++;
#endif
				};

				srcIn = (uint16_t*)src32;
				dstIn = (uint16_t*)dst32;
				if (xp1 & 1) {
					*dstIn++ |= *srcIn++;
				}
			}
			dst += LMAP_WIDTH;
			src += lightWidth;
			yLine++;
		}
	}
}

static void animateBiggerLight()
{
	int x0, x1, y0, y1;

	Point2D center;
	float dt = (float)(time_msec - startingTime) / 1000.0f;
	float tt = 1.0f - sin(dt);
	float disperse = tt * 16.0f;

	center.x = (FB_WIDTH >> 1) - (BIGGER_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (BIGGER_LIGHT_HEIGHT / 2);

	biggerLightPoint.x = center.x + sin(0.6f * dt) * (12.0f + disperse);
	biggerLightPoint.y = center.y + sin(0.8f * dt) * (28.0f + disperse);
		
	x0 = biggerLightPoint.x + LMAP_OFFSET_X;
	x1 = biggerLightPoint.x + LMAP_OFFSET_X + BIGGER_LIGHT_WIDTH;
	y0 = biggerLightPoint.y + LMAP_OFFSET_Y;
	y1 = biggerLightPoint.y + LMAP_OFFSET_Y + BIGGER_LIGHT_HEIGHT;

	if (x0 < lightMinX) lightMinX = x0;
	if (x1 > lightMaxX) lightMaxX = x1;
	if (y0 < lightMinY) lightMinY = y0;
	if (y1 > lightMaxY) lightMaxY = y1;
}

static void animateBigLights()
{
	int i;
	Point2D center;
	float dt = (float)(time_msec - startingTime) / 1000.0f;
	float tt = 1.0f - sin(dt);
	float disperse = tt * 8.0f;

	center.x = (FB_WIDTH >> 1) - (BIG_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (BIG_LIGHT_HEIGHT / 2);

	bigLightPoint[0].x = center.x + sin(1.2f * dt) * (96.0f + disperse);
	bigLightPoint[0].y = center.y + sin(0.8f * dt) * (64.0f + disperse);

	bigLightPoint[1].x = center.x + sin(1.5f * dt) * (80.0f + disperse);
	bigLightPoint[1].y = center.y + sin(1.1f * dt) * (48.0f + disperse);

	bigLightPoint[2].x = center.x + sin(2.0f * dt) * (72.0f + disperse);
	bigLightPoint[2].y = center.y + sin(1.3f * dt) * (56.0f + disperse);

	for (i = 0; i < NUM_BIG_LIGHTS; ++i) {
		Point2D* p = &bigLightPoint[i];
		int x0, x1, y0, y1;

		x0 = p->x + LMAP_OFFSET_X;
		x1 = p->x + LMAP_OFFSET_X + BIG_LIGHT_WIDTH;
		y0 = p->y + LMAP_OFFSET_Y;
		y1 = p->y + LMAP_OFFSET_Y + BIG_LIGHT_HEIGHT;

		if (x0 < lightMinX) lightMinX = x0;
		if (x1 > lightMaxX) lightMaxX = x1;
		if (y0 < lightMinY) lightMinY = y0;
		if (y1 > lightMaxY) lightMaxY = y1;
	}
}


#define SAFE_OFF_Y_TOP 60
#define SAFE_OFF_Y_BOTTOM 60

static void renderBump(unsigned short *vram)
{
	int x,y;

	int* bumpSrc = bumpOffsetScreen;
	for (y = 0; y < FB_HEIGHT; ++y) {
		unsigned short* lightSrc = &lightmap[(y+LMAP_OFFSET_Y) * LMAP_WIDTH + LMAP_OFFSET_X];
		for (x = 0; x < FB_WIDTH; ++x) {
			*vram++ = lightSrc[*bumpSrc++];
		}
	}
}

static void updateCurrentParticleLightmapEraseMinMax(Point2D *p)
{
	const int x0 = p->x + LMAP_OFFSET_X;
	const int x1 = p->x + LMAP_OFFSET_X + PARTICLE_LIGHT_WIDTH;
	const int y0 = p->y + LMAP_OFFSET_Y;
	const int y1 = p->y + LMAP_OFFSET_Y + PARTICLE_LIGHT_HEIGHT;

	if (x0 < lightMinX) lightMinX = x0;
	if (x1 > lightMaxX) lightMaxX = x1;
	if (y0 < lightMinY) lightMinY = y0;
	if (y1 > lightMaxY) lightMaxY = y1;
}

static void animateParticles()
{
	int i;
	Point2D center, p;
	float dt = (float)(time_msec - startingTime) / 2000.0f;
	float tt = 0.25f * sin(dt) + 0.75f;

	center.x = (FB_WIDTH >> 1) - (PARTICLE_LIGHT_WIDTH / 2);
	center.y = (FB_HEIGHT >> 1) - (PARTICLE_LIGHT_HEIGHT / 2);

	for (i = 0; i < numParticlesVisible; i++) {
		p.x = center.x + (sin(1.2f * (i*i*i + dt)) * 74.0f + sin(0.6f * (i + dt)) * 144.0f) * tt;
		p.y = center.y + (sin(1.8f * (i + dt)) * 68.0f + sin(0.5f * (i*i + dt)) * 132.0f) * tt;

		updateCurrentParticleLightmapEraseMinMax(&p);

		particlePoint[i].x = p.x;
		particlePoint[i].y = p.y;
	}
}

static void animateParticleElectroids(int t)
{
	static int prevT = 0;
	const int dt = t - prevT;

	int i;
	static int lastNumParticlesUpdated = 0;

	if (dt < 0 || dt > 20) {
		for (i = 0; i < numParticlesVisible; ++i) {
			const int pptI = i;
			Point2D* p = &electroidPoint[i];
			PathPointTrack* ppTrack = &pathPointTracks[pptI];
			PathPoint* pathPoint = &ppTrack->pathPoint[ppTrack->index];

			int speed = i & 3;
			if (speed == 0) speed = 1;

			if (ppTrack->pixelWalk == 0) {
				p->x = pathPoint->x - (PARTICLE_LIGHT_WIDTH / 2);
				p->y = pathPoint->y - (PARTICLE_LIGHT_HEIGHT / 2);
			}
			else {
				const int pDirIndex = pathPoint->dir;
				if (pDirIndex < 8) {
					const Point2D* pDir = &dir[pDirIndex];
					p->x += speed * pDir->x;
					p->y += speed * pDir->y;
				}
			}

			ppTrack->pixelWalk += speed;
			if (ppTrack->pixelWalk >= pathPoint->pixelLength) {
				ppTrack->pixelWalk = 0;
				ppTrack->index++;
				if (ppTrack->pathPoint[ppTrack->index].dir == 255 || ppTrack->index >= ppTrack->numPoints) {
					ppTrack->index = 0;
				}
			}
		}
		lastNumParticlesUpdated = numParticlesVisible;
		didElectroidsMovedYetOnce = 1;
		prevT = t;
	}

	if (didElectroidsMovedYetOnce != 0) {
		Point2D* srcPoint = electroidPoint;
		Point2D* dstPoint = particlePoint;
		for (i = 0; i < lastNumParticlesUpdated; ++i) {
			dstPoint->x = (srcPoint->x - moveLightmapOffset.x) & (HMAP_WIDTH - 1);
			dstPoint->y = (srcPoint->y - moveLightmapOffset.y) & (HMAP_HEIGHT - 1);
			updateCurrentParticleLightmapEraseMinMax(dstPoint);
			++srcPoint;
			++dstPoint;
		}
	}
}

static void eraseLightmapArea(int x0, int y0, int x1, int y1)
{
	CLAMP(x0, 0, LMAP_WIDTH - 1)
	CLAMP(x1, 0, LMAP_WIDTH - 1)
	CLAMP(y0, 0, LMAP_HEIGHT - 1)
	CLAMP(y1, 0, LMAP_HEIGHT - 1)

	if (x1 >= x0) {
		const int size = (x1 - x0 + 1) * 2;

		int y;
		for (y = y0; y <= y1; ++y) {
			memset(&lightmap[y * LMAP_WIDTH + x0], 0, size);
		}
	}

	lightMinX = LMAP_WIDTH-1;
	lightMaxX = 0;
	lightMinY = LMAP_HEIGHT-1;
	lightMaxY = 0;
}

static void renderBitmapLineBump(int u, int du, int* src, int* dst)
{
	int x;
	for (x=0; x<FB_WIDTH; ++x) {
		int tu = (u >> FP_SCALE) & (HMAP_WIDTH - 1);
		*dst++ = src[tu] + x;
		u += du;
	};
}

static void blitBumpTexDefault(int t)
{
	const int tt = t >> 5;

	int* dst = bumpOffsetScreen;
	int x,y;
	for (y = 0; y < FB_HEIGHT; ++y) {
		const int yi = (y + tt) & (HMAP_HEIGHT - 1);
		int *src = &bumpOffset[yi * HMAP_WIDTH];

		for (x = 0; x < FB_WIDTH; ++x) {
			*dst++ = src[x & (HMAP_WIDTH-1)] + x;
		}
	}

	moveLightmapOffset.x = 0;
	moveLightmapOffset.y = tt;
}

static void blitBumpTexWave(int t)
{
	const int tt = t >> 5;

	int* dst = bumpOffsetScreen;
	int y;
	for (y = 0; y < FB_HEIGHT; ++y) {
		const int yi = (y + tt) & (HMAP_HEIGHT - 1);
		int* src;
		int z, u, v, du;

		int yp = FB_HEIGHT + sin((y + 4*tt) / 56.0f) * (FB_HEIGHT / 6) + sin((y + 6 * tt) / 48.0f) * (FB_HEIGHT / 8);
		if (yp == 0) yp = 1;
		z = (1024 * PROJ_MUL) / yp;

		du = 64 * z;
		u = (-FB_WIDTH / 2) * du;

		v = ((z >> 8) + yi) & (HMAP_HEIGHT - 1);
		src = &bumpOffset[v * HMAP_WIDTH];

		renderBitmapLineBump(u, du, src, dst);

		dst += FB_WIDTH;
	}
}

static void blitBumpTextureScript(int t)
{
	if (startWaveBumpAndRGBlights == 0) {
		blitBumpTexDefault(t);
	} else {
		blitBumpTexWave(t);
	}
}

static void renderLightmapOverlay(int t)
{
	int x, y;
	int tt = t >> 6;

	unsigned int* dst = (unsigned int*)(lightmap + LMAP_OFFSET_Y * LMAP_WIDTH + LMAP_OFFSET_X);
	for (y = 0; y < FB_HEIGHT; ++y) {
		unsigned char* src = &faintLmapTex[(y & (FAINT_LMAP_HEIGHT - 1)) * FAINT_LMAP_WIDTH];
		for (x = 0; x < FB_WIDTH / 2; ++x) {
			int c = src[(x + tt) & (FAINT_LMAP_WIDTH - 1)] + src[(x + 2 * tt) & (FAINT_LMAP_WIDTH - 1)];
			*dst++ = (c << 16) | c;
		}
		dst += (LMAP_WIDTH - FB_WIDTH)/2;
	}
}

static void bumpScript()
{
	static int val1 = 0;
	static int prevVal1 = -1;

	numParticlesVisible = dseq_value(ev_electroids);
	CLAMP(numParticlesVisible, 0, NUM_PARTICLES);

	if (dseq_triggered(ev_scalerIn)) {
		startWaveBumpAndRGBlights = 1;
	}
}

static void fadeLightmap(float ft)
{
	int x, y, s;
	uint16_t* src;

	if (ft > 0.99f) return;

	s = (int)(ft * 256);
	src = lightmap + LMAP_OFFSET_Y * LMAP_WIDTH + LMAP_OFFSET_X;
	for (y = 0; y < FB_HEIGHT; ++y) {
		for (x = 0; x < FB_WIDTH; ++x) {
			uint16_t c = *src;
			int r = (UNPACK_R16(c) * s) >> 8;
			int g = (UNPACK_G16(c) * s) >> 8;
			int b = (UNPACK_B16(c) * s) >> 8;
			*src++ = PACK_RGB16(r, g, b);
		}
		src += LMAP_WIDTH - FB_WIDTH;
	}
}

static void draw(void)
{
	int i;
	const int t = time_msec - startingTime;
	const float ft = dseq_value(ev_fade);

	bumpScript();

	if (ft > 0.0f) {
		eraseLightmapArea(lightMinX, lightMinY, lightMaxX, lightMaxY);

		if (startWaveBumpAndRGBlights == 0) {
			renderLightmapOverlay(t);
			animateBiggerLight();
			renderLight(biggerLight, biggerLightEdges, BIGGER_LIGHT_WIDTH, BIGGER_LIGHT_HEIGHT, &biggerLightPoint, 0);
		}
		else {
			animateBigLights();
			for (i = 0; i < NUM_BIG_LIGHTS; ++i) {
				const int shouldBlit = (i == 0) ? 1 : 0;
				renderLight(bigLight[i], bigLightEdges, BIG_LIGHT_WIDTH, BIG_LIGHT_HEIGHT, &bigLightPoint[i], shouldBlit);
			}
		}

		if (numParticlesVisible > 0) {
			animateParticleElectroids(t);
			if (didElectroidsMovedYetOnce != 0) {
				renderParticles();
			}
		}

		fadeToBlack16bpp(ft, lightmap + LMAP_OFFSET_Y * LMAP_WIDTH + LMAP_OFFSET_X, FB_WIDTH, FB_HEIGHT, LMAP_WIDTH);

		blitBumpTextureScript(-2*t);

		renderBump((unsigned short*)fb_pixels);
	} else {
		memset(fb_pixels, 0, FB_WIDTH * FB_HEIGHT * 2);
	}

	swap_buffers(0);
}
