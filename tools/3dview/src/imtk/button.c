#include <string.h>
#include <assert.h>
#include "imtk.h"
#include "state.h"
#include "draw.h"

static void calc_button_size(const char *label, int *wret, int *hret);
static void draw_button(int id, const char *label, int x, int y, int over);

int imtk_button(int id, const char *label, int x, int y)
{
	int w, h, res = 0;
	int over = 0;

	assert(id >= 0);

	if(x == IMTK_AUTO || y == IMTK_AUTO) {
		imtk_layout_get_pos(&x, &y);
	}

	calc_button_size(label, &w, &h);

	if(imtk_hit_test(x, y, w, h)) {
		imtk_set_hot(id);
		over = 1;
	}

	if(imtk_button_state(IMTK_LEFT_BUTTON)) {
		if(over) {
			imtk_set_active(id);
		}
	} else { /* mouse button up */
		if(imtk_is_active(id)) {
			imtk_set_active(-1);
			if(imtk_is_hot(id) && over) {
				res = 1;
			}
		}
	}

	draw_button(id, label, x, y, over);
	imtk_layout_advance(w, h);
	return res;
}

static void draw_button(int id, const char *label, int x, int y, int over)
{
	float tcol[4], bcol[4];
	int width, height, active = imtk_is_active(id);
	unsigned int attr = 0;

	if(over) attr |= IMTK_FOCUS_BIT;
	if(active) attr |= IMTK_PRESS_BIT;

	calc_button_size(label, &width, &height);

	memcpy(tcol, imtk_get_color(IMTK_TOP_COLOR | attr), sizeof tcol);
	memcpy(bcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof bcol);

	imtk_draw_rect(x, y, width, height, tcol, bcol);
	imtk_draw_frame(x, y, width, height, active ? FRAME_INSET : FRAME_OUTSET);

	glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));
	imtk_draw_string(x + 20, y + 15, label);
}

static void calc_button_size(const char *label, int *wret, int *hret)
{
	int strsz = imtk_string_size(label);
	if(wret) *wret = strsz + 40;
	if(hret) *hret = 20;
}
