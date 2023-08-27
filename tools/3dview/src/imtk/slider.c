#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "imtk.h"
#include "state.h"
#include "draw.h"

#define SLIDER_SIZE		100
#define THUMB_WIDTH		10
#define THUMB_HEIGHT	20

static int draw_slider(int id, float pos, float min, float max, int x, int y, int over);

float imtk_slider(int id, float pos, float min, float max, int x, int y)
{
	int mousex, mousey, thumb_x, thumb_y, txsz, over = 0;
	float range = max - min;

	assert(id >= 0);

	if(x == IMTK_AUTO || y == IMTK_AUTO) {
		imtk_layout_get_pos(&x, &y);
	}
	y += THUMB_HEIGHT / 2;

	imtk_get_mouse(&mousex, &mousey);

	pos = (pos - min) / range;
	if(pos < 0.0) pos = 0.0;
	if(pos > 1.0) pos = 1.0;

	thumb_x = x + SLIDER_SIZE * pos - THUMB_WIDTH / 2;
	thumb_y = y - THUMB_HEIGHT / 2;

	if(imtk_hit_test(thumb_x, thumb_y, THUMB_WIDTH, THUMB_HEIGHT)) {
		imtk_set_hot(id);
		over = 1;
	}

	if(imtk_button_state(IMTK_LEFT_BUTTON)) {
		if(over && imtk_is_hot(id)) {
			if(!imtk_is_active(id)) {
				imtk_set_prev_mouse(mousex, mousey);
			}
			imtk_set_active(id);
		}
	} else {
		if(imtk_is_active(id)) {
			imtk_set_active(-1);
		}
	}

	if(imtk_is_active(id)) {
		int prevx;
		float dx;

		imtk_get_prev_mouse(&prevx, 0);

		dx = (float)(mousex - prevx) / (float)SLIDER_SIZE;
		pos += dx;
		imtk_set_prev_mouse(mousex, mousey);

		if(pos < 0.0) pos = 0.0;
		if(pos > 1.0) pos = 1.0;
	}

	txsz = draw_slider(id, pos, min, max, x, y, over);
	imtk_layout_advance(SLIDER_SIZE + THUMB_WIDTH + txsz, THUMB_HEIGHT);
	return pos * range + min;
}


static int draw_slider(int id, float pos, float min, float max, int x, int y, int over)
{
	float range = max - min;
	int thumb_x, thumb_y;
	char buf[32];
	float tcol[4], bcol[4];
	unsigned int attr = 0;

	thumb_x = x + SLIDER_SIZE * pos - THUMB_WIDTH / 2;
	thumb_y = y - THUMB_HEIGHT / 2;

	memcpy(tcol, imtk_get_color(IMTK_BOTTOM_COLOR), sizeof tcol);
	memcpy(bcol, imtk_get_color(IMTK_TOP_COLOR), sizeof bcol);

	/* draw trough */
	imtk_draw_rect(x, y - 2, SLIDER_SIZE, 5, tcol, bcol);
	imtk_draw_frame(x, y - 2, SLIDER_SIZE, 5, FRAME_INSET);

	if(over) {
		attr |= IMTK_FOCUS_BIT;
	}
	if(imtk_is_active(id)) {
		attr |= IMTK_PRESS_BIT;
	}
	memcpy(tcol, imtk_get_color(IMTK_TOP_COLOR | attr), sizeof tcol);
	memcpy(bcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof bcol);

	/* draw handle */
	imtk_draw_rect(thumb_x, thumb_y, THUMB_WIDTH, THUMB_HEIGHT, tcol, bcol);
	imtk_draw_frame(thumb_x, thumb_y, THUMB_WIDTH, THUMB_HEIGHT, FRAME_OUTSET);

	/* draw display */
	sprintf(buf, "%.3f", pos * range + min);
	glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));
	imtk_draw_string(x + SLIDER_SIZE + THUMB_WIDTH / 2 + 2, y + 4, buf);
	return imtk_string_size(buf);
}

