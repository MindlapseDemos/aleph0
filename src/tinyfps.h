#ifndef TINY_FPS_H
#define TINY_FPS_H

#define FPS_FONT_WIDTH 4
#define FPS_FONT_HEIGHT 5
#define FPS_FONTS_NUM 10
#define FPS_FONT_NUM_PIXELS (FPS_FONTS_NUM * FPS_FONT_WIDTH * FPS_FONT_HEIGHT)

void initFpsFonts();
void drawFps(unsigned short *vram);

#endif
