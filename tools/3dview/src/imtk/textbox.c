#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "imtk.h"
#include "state.h"
#include "draw.h"

#define TEXTBOX_SIZE	100
#define TEXTBOX_HEIGHT	20

static void draw_textbox(int id, char *text, int cursor, int x, int y, int over);


void imtk_textbox(int id, char *textbuf, size_t buf_sz, int x, int y)
{
	int key, len, cursor = 0, over = 0;

	assert(id >= 0);

	if(x == IMTK_AUTO || y == IMTK_AUTO) {
		imtk_layout_get_pos(&x, &y);
	}

	len = strlen(textbuf);

	/* HACK! using last element of textbuf for saving cursor position */
	if((cursor = textbuf[buf_sz - 1]) > len || cursor < 0) {
		cursor = len;
	}

	if(imtk_hit_test(x, y, TEXTBOX_SIZE, TEXTBOX_HEIGHT)) {
		imtk_set_hot(id);
		over = 1;
	}

	if(imtk_button_state(IMTK_LEFT_BUTTON)) {
		if(over) {
			imtk_set_active(id);
		}
	} else {
		if(imtk_is_active(id)) {
			imtk_set_active(-1);
			if(imtk_is_hot(id) && over) {
				imtk_set_focus(id);
			}
		}
	}

	if(imtk_has_focus(id)) {
		while((key = imtk_get_key()) != -1) {
			if(!(key & 0xff00) && isprint(key)) {
				if(len < buf_sz - 1) {
					if(cursor == len) {
						textbuf[len++] = (char)key;
						cursor = len;
					} else {
						memmove(textbuf + cursor + 1, textbuf + cursor, len - cursor);
						len++;
						textbuf[cursor++] = (char)key;
					}
				}
			} else {
				key &= 0xff;

				switch(key) {
				case '\b':
					if(cursor > 0) {
						if(cursor == len) {
							textbuf[--cursor] = 0;
						} else {
							memmove(textbuf + cursor - 1, textbuf + cursor, len - cursor);
							textbuf[--len] = 0;
							cursor--;
						}
					}
					break;

				case 127:	/* del */
					if(cursor < len) {
						memmove(textbuf + cursor, textbuf + cursor + 1, len - cursor);
						textbuf[--len] = 0;
					}
					break;

				case GLUT_KEY_LEFT:
					if(cursor > 0) {
						cursor--;
					}
					break;

				case GLUT_KEY_RIGHT:
					if(cursor < len) {
						cursor++;
					}
					break;

				case GLUT_KEY_HOME:
					cursor = 0;
					break;

				case GLUT_KEY_END:
					cursor = len;
					break;

				default:
					break;
				}
			}
		}
	}

	textbuf[buf_sz - 1] = cursor;
	draw_textbox(id, textbuf, cursor, x, y, over);
	imtk_layout_advance(TEXTBOX_SIZE, TEXTBOX_HEIGHT);
}


static void draw_textbox(int id, char *text, int cursor, int x, int y, int over)
{
	float tcol[4], bcol[4];
	unsigned int attr = 0;

	if(over) {
		attr |= IMTK_FOCUS_BIT;
	}
	memcpy(tcol, imtk_get_color(IMTK_BOTTOM_COLOR | attr), sizeof tcol);
	memcpy(bcol, imtk_get_color(IMTK_TOP_COLOR | attr), sizeof bcol);

	imtk_draw_rect(x, y, TEXTBOX_SIZE, TEXTBOX_HEIGHT, tcol, bcol);

	if(imtk_has_focus(id)) {
		int strsz;
		char tmp;

		tmp = text[cursor];
		text[cursor] = 0;
		strsz = imtk_string_size(text);
		text[cursor] = tmp;

		glBegin(GL_QUADS);
		glColor4fv(imtk_get_color(IMTK_CURSOR_COLOR));
		glVertex2f(x + strsz + 2, y + 2);
		glVertex2f(x + strsz + 4, y + 2);
		glVertex2f(x + strsz + 4, y + 18);
		glVertex2f(x + strsz + 2, y + 18);
		glEnd();
	}

	glColor4fv(imtk_get_color(IMTK_TEXT_COLOR));
	imtk_draw_string(x + 2, y + 15, text);

	imtk_draw_frame(x, y, TEXTBOX_SIZE, TEXTBOX_HEIGHT, FRAME_INSET);
}

