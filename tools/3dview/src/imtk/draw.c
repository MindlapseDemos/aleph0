#include <string.h>
#include <math.h>
#include <assert.h>
#include "draw.h"
#include "imtk.h"
#include "drawtext.h"

#define COLOR_MASK	0xff

/* default colors, can be changed with imtk_set_color */
static float colors[][4] = {
	{0.0, 0.0, 0.0, 1.0},		/* text color */
	{0.75, 0.75, 0.75, 1.0},	/* top color */
	{0.56, 0.56, 0.56, 1.0},	/* bot color */
	{0.9, 0.9, 0.9, 1.0},		/* lit bevel */
	{0.3, 0.3, 0.3, 1.0},		/* shadowed bevel */
	{0.8, 0.25, 0.18, 1.0},		/* cursor color */
	{0.68, 0.85, 1.3, 1.0},		/* selection color */
	{0.75, 0.1, 0.095, 1.0}		/* check color */
};

static float focus_factor = 1.15;
static float press_factor = 0.8;
static float alpha = 1.0;
static float bevel = 1.0;

void imtk_set_color(unsigned int col, float r, float g, float b, float a)
{
	int idx = col & COLOR_MASK;
	assert(idx >= 0 && idx < sizeof colors / sizeof *colors);

	colors[idx][0] = r;
	colors[idx][1] = g;
	colors[idx][2] = b;
	colors[idx][3] = a;
}

float *imtk_get_color(unsigned int col)
{
	static float ret[4];
	int idx = col & COLOR_MASK;

	memcpy(ret, colors + idx, sizeof ret);
	if(col & IMTK_FOCUS_BIT) {
		ret[0] *= focus_factor;
		ret[1] *= focus_factor;
		ret[2] *= focus_factor;
	}
	if(col & IMTK_PRESS_BIT) {
		ret[0] *= press_factor;
		ret[1] *= press_factor;
		ret[2] *= press_factor;
	}
	if(col & IMTK_SEL_BIT) {
		ret[0] *= colors[IMTK_SELECTION_COLOR][0];
		ret[1] *= colors[IMTK_SELECTION_COLOR][1];
		ret[2] *= colors[IMTK_SELECTION_COLOR][2];
	}
	ret[3] *= alpha;
	return ret;
}

void imtk_set_alpha(float a)
{
	alpha = a;
}

float imtk_get_alpha(void)
{
	return alpha;
}

void imtk_set_bevel_width(float b)
{
	bevel = b;
}

float imtk_get_bevel_width(void)
{
	return bevel;
}

void imtk_set_focus_factor(float fact)
{
	focus_factor = fact;
}

float imtk_get_focus_factor(void)
{
	return focus_factor;
}

void imtk_set_press_factor(float fact)
{
	press_factor = fact;
}

float imtk_get_press_factor(void)
{
	return press_factor;
}

void imtk_draw_rect(int x, int y, int w, int h, float *ctop, float *cbot)
{
	glBegin(GL_QUADS);
	if(ctop) {
		glColor4fv(ctop);
	}
	glVertex2f(x, y);
	glVertex2f(x + w, y);

	if(cbot) {
		glColor4fv(cbot);
	}
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);
	glEnd();
}

void imtk_draw_frame(int x, int y, int w, int h, int style)
{
	float tcol[4], bcol[4];
	float b = imtk_get_bevel_width();

	if(!b) {
		return;
	}

	x -= b;
	y -= b;
	w += b * 2;
	h += b * 2;

	switch(style) {
	case FRAME_INSET:
		memcpy(tcol, imtk_get_color(IMTK_BEVEL_SHAD_COLOR), sizeof tcol);
		memcpy(bcol, imtk_get_color(IMTK_BEVEL_LIT_COLOR), sizeof bcol);
		break;

	case FRAME_OUTSET:
	default:
		memcpy(tcol, imtk_get_color(IMTK_BEVEL_LIT_COLOR), sizeof tcol);
		memcpy(bcol, imtk_get_color(IMTK_BEVEL_SHAD_COLOR), sizeof bcol);
	}

	glBegin(GL_QUADS);
	glColor4fv(tcol);
	glVertex2f(x, y);
	glVertex2f(x + b, y + b);
	glVertex2f(x + w - b, y + b);
	glVertex2f(x + w, y);

	glVertex2f(x + b, y + b);
	glVertex2f(x, y);
	glVertex2f(x, y + h);
	glVertex2f(x + b, y + h - b);

	glColor4fv(bcol);
	glVertex2f(x + b, y + h - b);
	glVertex2f(x + w - b, y + h - b);
	glVertex2f(x + w, y + h);
	glVertex2f(x, y + h);

	glVertex2f(x + w - b, y + b);
	glVertex2f(x + w, y);
	glVertex2f(x + w, y + h);
	glVertex2f(x + w - b, y + h - b);
	glEnd();
}

void imtk_draw_disc(int x, int y, float rad, int subdiv, float *ctop, float *cbot)
{
	int i;
	float t, dtheta, theta = 0.0;
	float color[4];
	float cx = (float)x;
	float cy = (float)y;

	subdiv += 3;
	dtheta = 2.0 * M_PI / subdiv;

	color[0] = (ctop[0] + cbot[0]) * 0.5;
	color[1] = (ctop[1] + cbot[1]) * 0.5;
	color[2] = (ctop[2] + cbot[2]) * 0.5;
	color[3] = (ctop[3] + cbot[3]) * 0.5;

	glBegin(GL_TRIANGLE_FAN);
	glColor4fv(color);
	glVertex2f(cx, cy);

	for(i=0; i<=subdiv; i++) {
		float vx, vy;

		vx = cos(theta);
		vy = sin(theta);
		theta += dtheta;

		t = (vy + 1.0) / 2.0;
		color[0] = ctop[0] + (cbot[0] - ctop[0]) * t;
		color[1] = ctop[1] + (cbot[1] - ctop[1]) * t;
		color[2] = ctop[2] + (cbot[2] - ctop[2]) * t;
		color[3] = ctop[3] + (cbot[3] - ctop[3]) * t;

		glColor4fv(color);
		glVertex2f(cx + vx * rad, cy + vy * rad);
	}
	glEnd();
}

void imtk_draw_disc_frame(int x, int y, float inner, float outer, int subdiv, int style)
{
	int i;
	float t, dtheta, theta = 0.0;
	float color[4], tcol[4], bcol[4];
	float cx = (float)x;
	float cy = (float)y;

	switch(style) {
	case FRAME_INSET:
		memcpy(tcol, imtk_get_color(IMTK_BEVEL_SHAD_COLOR), sizeof tcol);
		memcpy(bcol, imtk_get_color(IMTK_BEVEL_LIT_COLOR), sizeof bcol);
		break;

	case FRAME_OUTSET:
	default:
		memcpy(tcol, imtk_get_color(IMTK_BEVEL_LIT_COLOR), sizeof tcol);
		memcpy(bcol, imtk_get_color(IMTK_BEVEL_SHAD_COLOR), sizeof bcol);
	}

	subdiv += 3;
	dtheta = 2.0 * M_PI / subdiv;

	glBegin(GL_QUAD_STRIP);

	for(i=0; i<=subdiv; i++) {
		float vx, vy;

		vx = cos(theta);
		vy = sin(theta);

		t = (vy + 1.0) / 2.0;
		color[0] = tcol[0] + (bcol[0] - tcol[0]) * t;
		color[1] = tcol[1] + (bcol[1] - tcol[1]) * t;
		color[2] = tcol[2] + (bcol[2] - tcol[2]) * t;
		color[3] = tcol[3] + (bcol[3] - tcol[3]) * t;

		vx = cos(theta - M_PI / 4.0);
		vy = sin(theta - M_PI / 4.0);
		theta += dtheta;

		glColor4fv(color);
		glVertex2f(cx + vx * inner, cy + vy * inner);
		glVertex2f(cx + vx * outer, cy + vy * outer);
	}
	glEnd();
}

void imtk_draw_string(int x, int y, const char *str)
{
	int xsz, ysz;

	imtk_get_viewport(&xsz, &ysz);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, xsz, ysz, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(x, y + dtx_baseline(), 0);
	glScalef(1, -1, 1);

	dtx_string(str);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

int imtk_string_size(const char *str)
{
	return dtx_string_width(str);
}
