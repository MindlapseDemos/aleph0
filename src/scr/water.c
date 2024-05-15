/* Water effect */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "demo.h"
#include "screen.h"
#include "opt_rend.h"
#include "gfxutil.h"
#include "noise.h"

#include "opt_3d.h"
#include "opt_rend.h"


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

#define FP_SCALE 16

static struct screen scr = {
	"water",
	init,
	destroy,
	start,
	0,
	draw
};

#define WATER_TEX_WIDTH 256
#define WATER_TEX_HEIGHT 256

#define CLOUD_TEX_WIDTH 256
#define CLOUD_TEX_HEIGHT 256
#define CLOUD_PERM 7
#define CLOUD_SHADES 32

#define SKY_HEIGHT 8192
#define WATER_FLOOR 2048
#define WATER_SHADES 16
#define PROJ_MUL 256

#define NUM_RAINDROPS 128
#define RAINDROPS_RANGE_X 1024
#define RAINDROPS_RANGE_Z 1024
#define RAINDROPS_RANGE_Y 1536
#define RAINDROPS_HEIGHT_Y 384
#define RAINDROPS_DIST 384
#define RAIN_SPEED_Y 16

static unsigned long startingTime;

static unsigned short *waterPal[WATER_SHADES];

static unsigned char *waterBuffer1;
static unsigned char *waterBuffer2;

unsigned char *wb1, *wb2;

unsigned char *cloudTex;
unsigned short *cloudPal[CLOUD_SHADES];

unsigned char* skyTex;
unsigned char* waterTex = NULL;

Vertex3D *rainDrops;

Mesh3D* meshFlower;
Object3D objFlower;


static void swapWaterBuffers()
{
	unsigned char *temp = wb2;
	wb2 = wb1;
	wb1 = temp;

	waterTex = wb1;
}

struct screen *water_screen(void)
{
	return &scr;
}

static void initCloudTex()
{
	int x, y, i, n;

	cloudTex = (unsigned char*)malloc(CLOUD_TEX_WIDTH * CLOUD_TEX_HEIGHT);
	skyTex = (unsigned char*)malloc(CLOUD_TEX_WIDTH * CLOUD_TEX_HEIGHT);

	i = 0;
	for (y = 0; y < CLOUD_TEX_HEIGHT; ++y) {
		int yp = y;
		for (x = 0; x < CLOUD_TEX_WIDTH; ++x) {
			float sumF = 0.0f;
			int xp = x;
			float d = 1.0f;
			for (n = 0; n < CLOUD_PERM; ++n) {
				float m = 1.0f / (CLOUD_PERM - n);
				int repX = CLOUD_TEX_WIDTH * d;
				int repY = CLOUD_TEX_HEIGHT * d;
				if (repX != 0 && repY != 0) {
					sumF += pnoise2((float)xp * d, (float)yp * d, repX, repY) * m;
				}
				d /= 2.0f;
			}

			sumF += 0.25f;
			CLAMP01(sumF)
			cloudTex[i++] = (unsigned char)(sumF * 127);
		}
	}

	for (i = 0; i < CLOUD_SHADES; ++i) {
		unsigned short *pal;
		cloudPal[i] = (unsigned short*)malloc(256 * sizeof(unsigned short));
		pal = cloudPal[i];
		for (n = 0; n < 255; ++n) {
			int r = n;
			int g = n - 128;
			int b = n + 64;

			r = (r * (CLOUD_SHADES - i - 1)) / (CLOUD_SHADES / 2) + 8;
			g = (g * (CLOUD_SHADES - i - 1)) / (CLOUD_SHADES / 2) + 4;
			b = (b * (CLOUD_SHADES - i - 1)) / (CLOUD_SHADES / 2) + 16;

			CLAMP(r, 0, 255);
			CLAMP(g, 0, 255);
			CLAMP(b, 0, 127);

			*pal++ = PACK_RGB16(r, g, b);
		}
	}
}

static void initRainDrops()
{
	int i;

	rainDrops = (Vertex3D*)malloc(sizeof(Vertex3D) * NUM_RAINDROPS);

	for (i = 0; i < NUM_RAINDROPS; ++i) {
		rainDrops[i].x = (rand() % RAINDROPS_RANGE_X) - RAINDROPS_RANGE_X / 2;
		rainDrops[i].y = (rand() % RAINDROPS_RANGE_Y) + RAINDROPS_HEIGHT_Y;
		rainDrops[i].z = (rand() % RAINDROPS_RANGE_Z) + RAINDROPS_DIST;
	}
}

static void initObjects()
{
	meshFlower = genMesh(GEN_OBJ_SPHERICAL, 80);

	objFlower.mesh = meshFlower;

	setClipValY(-WATER_FLOOR / 16);
}

static int init(void)
{
	int i;
	const int size = WATER_TEX_WIDTH * WATER_TEX_HEIGHT;

	waterBuffer1 = (unsigned char*)malloc(size);
	waterBuffer2 = (unsigned char*)malloc(size);
	memset(waterBuffer1, 0, size);
	memset(waterBuffer2, 0, size);

	wb1 = waterBuffer1;
	wb2 = waterBuffer2;

	for (i = 0; i < WATER_SHADES; ++i) {
		float s = 1.0f - (float)(WATER_SHADES - i - 1) / (WATER_SHADES * 1.125);
		CLAMP01(s)
		waterPal[i] = (unsigned short*)malloc(sizeof(unsigned short) * 256);
		setPalGradient(0, 127, 0, 0, 7*s, 31 * s, 63 * s, 31 * s, waterPal[i]);
		setPalGradient(128, 255, 0, 0, 0, 0, 0, 0, waterPal[i]);
	}

	initCloudTex();
	initRainDrops();

	initOptEngine(4096);
	initObjects();

	return 0;
}

static void freePals()
{
	int i;
	for (i = 0; i < CLOUD_SHADES; ++i) {
		free(cloudPal[i]);
	}
	for (i = 0; i < WATER_SHADES; ++i) {
		free(waterPal[i]);
	}
}

static void destroy(void)
{
	free(cloudTex);
	free(skyTex);
	free(waterBuffer1);
	free(waterBuffer2);

	freeMesh(meshFlower);
	freeOptEngine();

	freePals();
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

#ifdef __WATCOMC__
void updateWaterAsm5(void *buffer1, void *buffer2, void *vramStart);
#else
static void updateWater32(unsigned char *buffer1, unsigned char *buffer2)
{
	int count = (WATER_TEX_WIDTH / 4) * (WATER_TEX_HEIGHT - 2) - 2;

	unsigned int *src1 = (unsigned int*)buffer1;
	unsigned int *src2 = (unsigned int*)buffer2;
	unsigned int *vram = (unsigned int*)((unsigned char*)wb1 + WATER_TEX_WIDTH + 4);

	do {
		const unsigned int c0 = *(unsigned int*)((unsigned char*)src1-1);
		const unsigned int c1 = *(unsigned int*)((unsigned char*)src1+1);
		const unsigned int c2 = *(src1-(WATER_TEX_WIDTH / 4));
		const unsigned int c3 = *(src1+(WATER_TEX_WIDTH / 4));

		/* Subtract and then absolute value of 4 bytes packed in 8bits(From Hacker's Delight) */
		const unsigned int c = (((c0 + c1 + c2 + c3) >> 1) & 0x7f7f7f7f) - *src2;
		const unsigned int a = c & 0x80808080;
		const unsigned int b = a >> 7;
		const unsigned int cc = (c ^ ((a - b) | a)) + b;

		*src2++ = cc;
		src1++;
	} while (--count > 0);
}
#endif

static void renderBlob(int xp, int yp, unsigned char *buffer)
{
	int i,j;
	unsigned char *dst = buffer + yp * WATER_TEX_WIDTH + xp;

	for (j=0; j<3; ++j) {
		for (i=0; i<3; ++i) {
			*(dst + i) = 0x3f;
		}
		dst += WATER_TEX_WIDTH;
	}
}

static void makeRipples(unsigned char *buff, int t)
{
	/*int i;
	for (i = 0; i<3; ++i) {
		const int xp = WATER_TEX_WIDTH / 2 + (int)(sin((t / 32 + 64*i)/24.0) * 96);
		const int yp = WATER_TEX_HEIGHT / 2 + (int)(sin((t / 32 + 64*i)/16.0) * 64);

		renderBlob(xp,yp,buff);
	}*/

	renderBlob(24 + (rand() % 224), 24 + (rand() % 224), buff);
}

static void moveRain()
{
	int i;

	for (i = 0; i < NUM_RAINDROPS; ++i) {
		int y = rainDrops[i].y;
		int x = rainDrops[i].x;

		y -= RAIN_SPEED_Y;
		if (y < -WATER_FLOOR/8) {
			y = (rand() % RAINDROPS_RANGE_Y) + RAINDROPS_HEIGHT_Y;
		}
		rainDrops[i].y = y;

		x -= 2;
		if (x < -RAINDROPS_RANGE_X / 2) {
			x = RAINDROPS_RANGE_X / 2;
		}
		rainDrops[i].x = x;
	}
}

static void runWaterEffect(int t)
{
	static int prevT = 0;
	const int dt = t - prevT;
	if (dt < 0 || dt > 20) {
		unsigned char* buff1 = wb1 + WATER_TEX_WIDTH + 4;
		unsigned char* buff2 = wb2 + WATER_TEX_WIDTH + 4;
		unsigned char* vramOffset = (unsigned char*)buff1 + WATER_TEX_WIDTH + 4;

		moveRain();

		makeRipples(buff1, t);

		#ifdef __WATCOMC__
				updateWaterAsm5(buff1, buff2, vramOffset);
		#else
				updateWater32(buff1, buff2);
		#endif

		swapWaterBuffers();

		prevT = t;
	}
}

static void renderBitmapLineX(int u, int du, int texWidth, int length, unsigned char* src, unsigned short *pal, unsigned short* dst)
{
	while (length-- > 0) {
		int tu = (u >> FP_SCALE) & (texWidth - 1);
		*dst++ = pal[src[tu]];
		u += du;
	};
}

static void renderBitmapLineX2(int u, int du, int texWidth1, int texWidth2, int length, unsigned char* src1, unsigned char* src2, unsigned short* pal, unsigned short* dst)
{
	while (length-- > 0) {
		int tu = u >> FP_SCALE;
		int tu1 = tu & (texWidth1 - 1);
		int wat0 = src1[tu1];
		int wat1 = src1[(tu1 + 2) & (texWidth1 - 1)];
		int dx = (wat1 - wat0) << 1;
		int tu2 = (tu + dx) & (texWidth2 - 1);
		*dst++ = pal[src1[tu1] + (src2[tu2] >> 2)];
		u += du;
	};
}

static void blendClouds(int t)
{
	int x, y;
	unsigned int* dst = (unsigned int*)skyTex;

	for (y = 0; y < CLOUD_TEX_HEIGHT; ++y) {
		int v1 = (y + t) & (CLOUD_TEX_HEIGHT - 1);
		int v2 = (y + t * 2) & (CLOUD_TEX_HEIGHT - 1);
		unsigned int *src1 = (unsigned int*)&cloudTex[v1 * CLOUD_TEX_WIDTH];
		unsigned int *src2 = (unsigned int*)&cloudTex[v2 * CLOUD_TEX_WIDTH];
		for (x = 0; x < CLOUD_TEX_WIDTH/4; ++x) {
			*dst++ = *src1++ + *src2++;
		}
	}
}

static void testBlitCloudTex()
{
	unsigned short* dst = (unsigned short*)fb_pixels;
	int y;
	for (y = 0; y < FB_HEIGHT/2; ++y) {
		unsigned char *src;
		int z, u, v, du;
		int palNum = (CLOUD_SHADES * y) / (FB_HEIGHT / 2);

		int yp = FB_HEIGHT / 2 - y;
		if (yp == 0) yp = 1;
		z = (SKY_HEIGHT * PROJ_MUL) / yp;

		du = z * 3;
		u = (-FB_WIDTH / 2) * du;

		v = (z >> 8) & (CLOUD_TEX_HEIGHT - 1);
		src = &skyTex[v * CLOUD_TEX_WIDTH];

		CLAMP(palNum, 0, CLOUD_SHADES-1)
		renderBitmapLineX(u, du, CLOUD_TEX_WIDTH, FB_WIDTH, src, cloudPal[palNum], dst);

		dst += FB_WIDTH;
	}
}

static void testBlitCloudWater()
{
	int y;

	if (!waterTex) return;

	for (y = FB_HEIGHT / 2; y < FB_HEIGHT; ++y) {
		unsigned char* src;
		unsigned short* dst = (unsigned short*)fb_pixels + y * FB_WIDTH;

		int z, u, v, v1, v2, du;
		int palNum = (WATER_SHADES * (y - FB_HEIGHT / 2)) / (FB_HEIGHT / 2);

		int yp = FB_HEIGHT / 2 - y;
		if (yp == 0) yp = 1;
		z = (WATER_FLOOR * PROJ_MUL) / yp;

		du = z * 8;
		u = (-FB_WIDTH / 2) * du;

		v = z >> 6;
		v1 = v & (WATER_TEX_HEIGHT - 1);
		v2 = (CLOUD_TEX_HEIGHT - 1) - v & (CLOUD_TEX_HEIGHT - 1);
		src = &waterTex[v1 * WATER_TEX_WIDTH];

		CLAMP(palNum, 0, WATER_SHADES - 1)
		renderBitmapLineX2(u, du, WATER_TEX_WIDTH, CLOUD_TEX_WIDTH, FB_WIDTH, src, &skyTex[v2 * CLOUD_TEX_WIDTH], waterPal[palNum], dst);
	}
}

static void drawRain(int zRangeMin, int zRangeMax)
{
	int i;

	for (i = 0; i < NUM_RAINDROPS; ++i) {
		int z = rainDrops[i].z;
		if (z >= zRangeMin && z < zRangeMax) {
			Vertex3D v1, v2;
			int x = rainDrops[i].x;
			int y = rainDrops[i].y;
			v1.xs = (x * PROJ_MUL) / z + FB_WIDTH / 2;
			v1.ys = FB_HEIGHT / 2 - (y * PROJ_MUL) / z;
			v2.xs = v1.xs + 1;
			v2.ys = FB_HEIGHT / 2 - ((y + 2*RAIN_SPEED_Y) * PROJ_MUL) / z;

			drawAntialiasedLine16bpp(&v1, &v2, 4 + ((z - RAINDROPS_DIST) >> 9), fb_pixels);
		}
	}
}

static void sceneRunFlower(int t)
{
	clearZbuffer();

	setObjectPos(0, -64, 512, &objFlower);
	setObjectRot(t, 2*t, 3*t, &objFlower);

	transformObject3D(&objFlower);
	renderObject3D(&objFlower);
}

static void draw(void)
{
	const int t = time_msec - startingTime;
	const int frontRainZ = 640;

	runWaterEffect(t);

	blendClouds(t >> 5);
	testBlitCloudTex();

	testBlitCloudWater();

	drawRain(frontRainZ, 16384);
	sceneRunFlower(t);
	drawRain(0, frontRainZ);

	swap_buffers(0);
}
