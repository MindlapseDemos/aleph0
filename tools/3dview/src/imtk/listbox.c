#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "imtk.h"
#include "state.h"
#include "draw.h"

#define ITEM_HEIGHT		18
#define PAD				3

static int list_radio(int id, const char *list, int sel, int x, int y, void (*draw)());
static void draw_listbox(int id, const char *list, int sel, int x, int y, int width, int nitems, int over);
static void draw_radio(int id, const char *list, int sel, int x, int y, int width, int nitems, int over);

int imtk_listbox(int id, const char *list, int sel, int x, int y)
{
	return list_radio(id, list, sel, x, y, draw_listbox);
}

int imtk_radiogroup(int id, const char *list, int sel, int x, int y)
{
	return list_radio(id, list, sel, x, y, draw_radio);
}

static int list_radio(int id, const char *list, int sel, int x, int y, void (*draw)())
{
	int i, max_width, nitems, over;
	const char *ptr;

	assert(id >= 0);

	if(x == IMTK_AUTO || y == IMTK_AUTO) {
		imtk_layout_get_pos(&x, &y);
	}

	max_width = 0;
	over = 0;

	ptr = list;
	for(i=0; *ptr; i++) {
		int strsz = imtk_string_size(ptr) + 2 * PAD;
		if(strsz > max_width) {
			max_width = strsz;
		}
		ptr += strlen(ptr) + 1;

		if(imtk_hit_test(x, y + i * ITEM_HEIGHT, max_width, ITEM_HEIGHT)) {
			imtk_set_hot(id);
			over = i + 1;
		}
	}
	nitems = i;

	if(imtk_button_state(IMTK_LEFT_BUTTON)) {
		if(over) {
			imtk_set_active(id);
		}
	} else {
		if(imtk_is_active(id)) {
			imtk_set_active(-1);
			if(imtk_is_hot(id) && over) {
				sel = over - 1;
			}
		}
	}

	draw(id, list, sel, x, y, max_width, nitems, over);
	imtk_layout_advance(max_width, ITEM_HEIGHT * nitems);
	return sel;
}

char *imtk_create_list(const char *first, ...)
{
	int sz;
	char *buf, *item;
	va_list ap;

	if(!first) {
		return 0;
	}

	sz = strlen(first) + 2;
	if(!(buf = malloc(sz))) {
		return 0;
	}
	memcpy(buf, first, sz - 2);
	buf[sz - 1] = buf[sz - 2] = 0;

	va_start(ap, first);
	while((item = va_arg(ap, char*))) {
		int len = strlen(item);
		char *tmp = realloc(buf, sz + len + 1);
		if(!tmp) {
			free(buf);
			return 0;
		}
		buf = tmp;

		memcpy(buf + sz - 1, item, len);
		sz += len + 1;
		buf[sz - 1] = buf[sz - 2] = 0;
	}
	va_end(ap);

	return buf;
}

void imtk_free_list(char *list)
{
	free(list);
}

static void draw_listbox(int id, const char *list, int sel, int x, int y, int width, int nitems, int over)
{
	int i;
	const char *item = list;

	glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));

	for(i=0; i<nitems; i++) {
		int item_y = i * ITEM_HEIGHT + y;
		unsigned int attr = 0;
		float tcol[4], bcol[4];

		if(over - 1 == i) {
			attr |= IMTK_FOCUS_BIT;
		}

		if(sel == i) {
			attr |= IMTK_SEL_BIT;
			memcpy(tcol, imtk_get_color(IMTK_TOP_COLOR | attr), sizeof tcol);
			memcpy(bcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof bcol);
		} else {
			memcpy(tcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof tcol);
			memcpy(bcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof bcol);
		}

		imtk_draw_rect(x, item_y, width, ITEM_HEIGHT, tcol, bcol);

		glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));
		imtk_draw_string(x + 3, item_y + ITEM_HEIGHT - 5, item);
		item += strlen(item) + 1;
	}

	imtk_draw_frame(x, y, width, ITEM_HEIGHT * nitems, FRAME_INSET);
}

static void draw_radio(int id, const char *list, int sel, int x, int y, int width, int nitems, int over)
{
	int i;
	const char *item = list;
	float rad = ITEM_HEIGHT * 0.5;

	for(i=0; i<nitems; i++) {
		int item_y = i * ITEM_HEIGHT + y;
		unsigned int attr = 0;
		float tcol[4], bcol[4];

		if(over - 1 == i) {
			attr |= IMTK_FOCUS_BIT;
		}

		imtk_draw_disc_frame(x + rad, item_y + rad, rad * 0.9, rad * 0.75, 5, FRAME_INSET);

		memcpy(tcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof tcol);
		memcpy(bcol, imtk_get_color(IMTK_TOP_COLOR | attr), sizeof bcol);
		imtk_draw_disc(x + rad, item_y + rad, rad * 0.75, 5, tcol, bcol);

		if(i == sel) {
			attr |= IMTK_SEL_BIT;
			memcpy(tcol, imtk_get_color(IMTK_TOP_COLOR | attr), sizeof tcol);
			memcpy(bcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof bcol);

			imtk_draw_disc(x + rad, item_y + ITEM_HEIGHT / 2, rad * 0.6, 5, tcol, bcol);
		}

		glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));
		imtk_draw_string(x + rad * 2.0 + 3, item_y + ITEM_HEIGHT - 5, item);
		item += strlen(item) + 1;
	}
}
