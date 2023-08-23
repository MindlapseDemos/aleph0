#ifndef DRAW_H_
#define DRAW_H_

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif
#include "imtk.h"

enum {
	FRAME_OUTSET,
	FRAME_INSET
};

void imtk_draw_rect(int x, int y, int w, int h, float *ctop, float *cbot);
void imtk_draw_frame(int x, int y, int w, int h, int style);
void imtk_draw_disc(int x, int y, float rad, int subdiv, float *ctop, float *cbot);
void imtk_draw_disc_frame(int x, int y, float inner, float outer, int subdiv, int style);
void imtk_draw_string(int x, int y, const char *str);
int imtk_string_size(const char *str);

#endif	/* DRAW_H_ */
