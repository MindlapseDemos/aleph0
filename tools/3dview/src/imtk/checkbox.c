#include <string.h>
#include <assert.h>
#include "imtk.h"
#include "state.h"
#include "draw.h"


#define CHECKBOX_SIZE	14


static void draw_checkbox(int id, const char *label, int x, int y, int state, int over);

int imtk_checkbox(int id, const char *label, int x, int y, int state)
{
	int sz = CHECKBOX_SIZE;
	int full_size, over = 0;

	assert(id >= 0);

	if(x == IMTK_AUTO || y == IMTK_AUTO) {
		imtk_layout_get_pos(&x, &y);
	}

	full_size = sz + imtk_string_size(label) + 5;
	if(imtk_hit_test(x, y, full_size, sz)) {
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
				state = !state;
			}
		}
	}

	draw_checkbox(id, label, x, y, state, over);
	imtk_layout_advance(full_size, sz);
	return state;
}

static float v[][2] = {
	{-0.2, 0.63},	/* 0 */
	{0.121, 0.325},	/* 1 */
	{0.15, 0.5},	/* 2 */
	{0.28, 0.125},	/* 3 */
	{0.38, 0.33},	/* 4 */
	{0.42, -0.122},	/* 5 */
	{0.58, 0.25},	/* 6 */
	{0.72, 0.52},	/* 7 */
	{0.625, 0.65},	/* 8 */
	{0.89, 0.78},	/* 9 */
	{0.875, 0.92},	/* 10 */
	{1.13, 1.145}	/* 11 */
};
#define TRI(a, b, c)	(glVertex2fv(v[a]), glVertex2fv(v[b]), glVertex2fv(v[c]))

static void draw_checkbox(int id, const char *label, int x, int y, int state, int over)
{
	static const int sz = CHECKBOX_SIZE;
	unsigned int attr = 0;
	float tcol[4], bcol[4];

	if(over) {
		attr |= IMTK_FOCUS_BIT;
	}
	if(imtk_is_active(id)) {
		attr |= IMTK_PRESS_BIT;
	}
	memcpy(tcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof tcol);
	memcpy(bcol, imtk_get_color(IMTK_TOP_COLOR | attr), sizeof bcol);

	imtk_draw_rect(x, y, sz, sz, tcol, bcol);
	imtk_draw_frame(x, y, sz, sz, FRAME_INSET);

	glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));
	if(state) {
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(x, y + sz, 0);
		glScalef(sz * 1.2, -sz * 1.3, 1);

		glBegin(GL_TRIANGLES);
		glColor4fv(imtk_get_color(IMTK_CHECK_COLOR));
		TRI(0, 1, 2);
		TRI(1, 3, 2);
		TRI(3, 4, 2);
		TRI(3, 5, 4);
		TRI(4, 5, 6);
		TRI(4, 6, 7);
		TRI(4, 7, 8);
		TRI(8, 7, 9);
		TRI(8, 9, 10);
		TRI(10, 9, 11);
		glEnd();

		glPopMatrix();
	}

	glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));
	imtk_draw_string(x + sz + 5, y + sz - 2, label);
}

