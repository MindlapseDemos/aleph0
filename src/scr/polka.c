/* Dummy part to test the performance of my mini 3d engine, also occupied for future 3D Polka dots idea */

#include "demo.h"
#include "screen.h"

#include "opt_3d.h"


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
}

static void start(long trans_time)
{
	startingTime = time_msec;
}

static void draw(void)
{
	Opt3DrunPerfTest(time_msec - startingTime);

	swap_buffers(0);
}
