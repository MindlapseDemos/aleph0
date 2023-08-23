#include <string.h>
#include <assert.h>
#include "imtk.h"

struct layout {
	int box[4], span[4];
	int spacing;
	int dir;
};

#define MAX_STACK_DEPTH		4
static struct layout st[MAX_STACK_DEPTH];
static int top = 0;

int imtk_layout_push(void)
{
	int newtop = top + 1;

	assert(newtop < MAX_STACK_DEPTH);
	if(newtop >= MAX_STACK_DEPTH) {
		return -1;
	}

	st[newtop] = st[top++];
	return 0;
}

int imtk_layout_pop(void)
{
	assert(top > 0);
	if(top <= 0) {
		return -1;
	}
	top--;
	return 0;
}

void imtk_layout_start(int x, int y)
{
	st[top].box[0] = st[top].span[0] = x;
	st[top].box[1] = st[top].span[1] = y;
	st[top].box[2] = st[top].box[3] = st[top].span[2] = st[top].span[3] = 0;
/*	st[top].spacing = sp;
	st[top].dir = dir;*/
}

void imtk_layout_dir(int dir)
{
	st[top].dir = dir;
	if(dir == IMTK_VERTICAL) {
		imtk_layout_newline();
	}
}

void imtk_layout_spacing(int spacing)
{
	st[top].spacing = spacing;
}

void imtk_layout_advance(int width, int height)
{
	int max_span_y, max_box_y;

	st[top].span[2] += width + st[top].spacing;

	if(height > st[top].span[3]) {
		st[top].span[3] = height;
	}

	max_span_y = st[top].span[1] + st[top].span[3];
	max_box_y = st[top].box[1] + st[top].box[3];

	if(max_span_y > max_box_y) {
		st[top].box[3] = max_span_y - st[top].box[1];
	}
	if(st[top].span[2] > st[top].box[2]) {
		st[top].box[2] = st[top].span[2];
	}

	if(st[top].dir == IMTK_VERTICAL) {
		imtk_layout_newline();
	}
}

void imtk_layout_newline(void)
{
	st[top].span[0] = st[top].box[0];
	st[top].span[1] = st[top].box[1] + st[top].box[3] + st[top].spacing;
	st[top].span[2] = st[top].span[3] = 0;
}

void imtk_layout_get_pos(int *x, int *y)
{
	*x = st[top].span[0] + st[top].span[2];
	*y = st[top].span[1];
}

void imtk_layout_get_bounds(int *bbox)
{
	memcpy(bbox, st[top].box, sizeof st[top].box);
}

int imtk_layout_contains(int x, int y)
{
	if(x < st[top].box[0] || x >= st[top].box[0] + st[top].box[2]) {
		return 0;
	}
	if(y < st[top].box[1] || y >= st[top].box[1] + st[top].box[3]) {
		return 0;
	}
	return 1;
}
