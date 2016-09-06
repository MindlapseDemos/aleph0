#include <stdio.h>
#include <stdlib.h>
#include "screen.h"

static int init(void);
static void destroy(void);
static void draw(void);

static struct screen scr = {
	"polytest",
	init,
	destroy,
	0, 0,
	draw
};

struct screen *polytest_screen(void)
{
	return &scr;
}

static int init(void)
{
	return 0;
}

static void destroy(void)
{
}

static void draw(void)
{
}
