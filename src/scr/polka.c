/* Dummy part to test the performance of my mini 3d engine, also occupied for future 3D Polka dots idea */

#include "demo.h"
#include "screen.h"

#include "opt_3d.h"

#include <math.h>


static int init(void);
static void destroy(void);
static void start(long trans_time);
static void draw(void);

static struct screen scr = {
	"polka",
	init,
	destroy,
	start,
	0,
	draw
};

static unsigned long startingTime;

struct screen *polka_screen(void)
{
	return &scr;
}

static int init(void)
{
	Opt3DinitPerfTest();

	return 0;
}

static void destroy(void)
{
	Opt3DfreePerfTest();
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void updateDotsVolumeBufferPlasma(int t)
{
	unsigned char *dst = getDotsVolumeBuffer();
	const float tt = t * 0.001f;

	int countZ = VERTICES_DEPTH;
	do {
		int countY = VERTICES_HEIGHT;
		do {
			int countX = VERTICES_WIDTH;
			do {
				unsigned char c = 0;
				float n = fmod(fabs(sin((float)countX/8.0f + tt) + sin((float)countY/6.0f + 2.0f * tt) + sin((float)countZ/7.0f - tt)), 1.0f);
				if (n > 0.9f) c = 1;

				*dst++ = c;
			} while(--countX != 0);
		} while(--countY != 0);
	} while(--countZ != 0);
}

static void draw(void)
{
	const int t = time_msec - startingTime;

	//updateDotsVolumeBufferPlasma(t);
	Opt3DrunPerfTest(t);

	swap_buffers(0);
}
