#include "state.h"
#include "imtk.h"

struct key_node {
	int key;
	struct key_node *next;
};


struct imtk_state st = {
	1, 1,			/* scr_width/scr_height */
	0, 0, 0, 0, 0,	/* mousex/mousey, prevx, prevy, mouse_bnmask */
	-1, -1, -1, -1	/* active, hot, input, prev_active */
};

static struct key_node *key_list, *key_tail;



void imtk_inp_key(int key, int state)
{
	if(state == IMTK_DOWN) {
		struct key_node *node;

		if(!(node = malloc(sizeof *node))) {
			return;
		}
		node->key = key;
		node->next = 0;

		if(key_list) {
			key_tail->next = node;
			key_tail = node;
		} else {
			key_list = key_tail = node;
		}
	}

	imtk_post_redisplay();
}

void imtk_inp_mouse(int bn, int state)
{
	if(state == IMTK_DOWN) {
		st.mouse_bnmask |= 1 << bn;
	} else {
		st.mouse_bnmask &= ~(1 << bn);
	}
	imtk_post_redisplay();
}

void imtk_inp_motion(int x, int y)
{
	st.mousex = x;
	st.mousey = y;

	imtk_post_redisplay();
}

void imtk_set_viewport(int x, int y)
{
	st.scr_width = x;
	st.scr_height = y;
}

void imtk_get_viewport(int *width, int *height)
{
	if(width) *width = st.scr_width;
	if(height) *height = st.scr_height;
}


void imtk_set_active(int id)
{
	if(id == -1 || st.hot == id) {
		st.prev_active = st.active;
		st.active = id;
	}
}

int imtk_is_active(int id)
{
	return st.active == id;
}

int imtk_set_hot(int id)
{
	if(st.active == -1) {
		st.hot = id;
		return 1;
	}
	return 0;
}

int imtk_is_hot(int id)
{
	return st.hot == id;
}

void imtk_set_focus(int id)
{
	st.input = id;
}

int imtk_has_focus(int id)
{
	return st.input == id;
}

int imtk_hit_test(int x, int y, int w, int h)
{
	return st.mousex >= x && st.mousex < (x + w) &&
		st.mousey >= y && st.mousey < (y + h);
}

void imtk_get_mouse(int *xptr, int *yptr)
{
	if(xptr) *xptr = st.mousex;
	if(yptr) *yptr = st.mousey;
}

void imtk_set_prev_mouse(int x, int y)
{
	st.prevx = x;
	st.prevy = y;
}

void imtk_get_prev_mouse(int *xptr, int *yptr)
{
	if(xptr) *xptr = st.prevx;
	if(yptr) *yptr = st.prevy;
}

int imtk_button_state(int bn)
{
	return st.mouse_bnmask & (1 << bn);
}


int imtk_get_key(void)
{
	int key = -1;
	struct key_node *node = key_list;

	if(node) {
		key = node->key;
		key_list = node->next;
		if(!key_list) {
			key_tail = 0;
		}
		free(node);
	}
	return key;
}
