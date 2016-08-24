/*
colcycle - color cycling image viewer
Copyright (C) 2016  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef GFX_H_
#define GFX_H_

#ifdef __cplusplus
extern "C" {
#endif

void *set_video_mode(int xsz, int ysz, int bpp);
int set_text_mode(void);

int get_color_depth(void);
int get_color_bits(int *rbits, int *gbits, int *bbits);
int get_color_shift(int *rshift, int *gshift, int *bshift);
int get_color_mask(unsigned int *rmask, unsigned int *gmask, unsigned int *bmask);

void set_palette(int idx, int r, int g, int b);

void wait_vsync(void);

#ifdef __cplusplus
}
#endif

#endif	/* GFX_H_ */
