#include <string.h>
#include "imtk.h"
#include "draw.h"

#define PROGR_SIZE             100
#define PROGR_HEIGHT			15

static void draw_progress(int id, float pos, int x, int y);

void imtk_progress(int id, float pos, int x, int y)
{
	if(x == IMTK_AUTO || y == IMTK_AUTO) {
		imtk_layout_get_pos(&x, &y);
	}

	draw_progress(id, pos, x, y);
	imtk_layout_advance(PROGR_SIZE, PROGR_HEIGHT);
}

static void draw_progress(int id, float pos, int x, int y)
{
	int bar_size = PROGR_SIZE * pos;
	float b = imtk_get_bevel_width();
	float tcol[4], bcol[4];

	if(pos < 0.0) pos = 0.0;
	if(pos > 1.0) pos = 1.0;

	memcpy(tcol, imtk_get_color(IMTK_BOTTOM_COLOR), sizeof tcol);
	memcpy(bcol, imtk_get_color(IMTK_TOP_COLOR), sizeof bcol);

	/* trough */
	imtk_draw_rect(x - b, y - b, PROGR_SIZE + b * 2, PROGR_HEIGHT + b * 2, tcol, bcol);
	imtk_draw_frame(x - b, y - b, PROGR_SIZE + b * 2, PROGR_HEIGHT + b * 2, FRAME_INSET);

	/* bar */
	if(pos > 0.0) {
		memcpy(tcol, imtk_get_color(IMTK_TOP_COLOR), sizeof tcol);
		memcpy(bcol, imtk_get_color(IMTK_BOTTOM_COLOR), sizeof bcol);

		imtk_draw_rect(x, y, bar_size, PROGR_HEIGHT, tcol, bcol);
		imtk_draw_frame(x, y, bar_size, PROGR_HEIGHT, FRAME_OUTSET);
	}
}
