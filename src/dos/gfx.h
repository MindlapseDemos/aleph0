/*
DOS demo/game framework
Copyright (C) 2016-2025  John Tsiombikas <nuclear@mutantstargoat.com>

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

#include "szint.h"

struct video_mode {
	uint16_t mode;
	short xsz, ysz, bpp, pitch;
	short rbits, gbits, bbits;
	short rshift, gshift, bshift;
	uint32_t rmask, gmask, bmask;
	uint32_t fb_addr;
	short max_pages;
	short win_gran, win_gran_shift, win_64k_step;
};

#ifdef __cplusplus
extern "C" {
#endif

int init_video(void);
void cleanup_video(void);

struct video_mode *video_modes(void);
int num_video_modes(void);

#define VMODE_CURRENT	(-1)
struct video_mode *get_video_mode(int idx);

int match_video_mode(int xsz, int ysz, int bpp);
int find_video_mode(int mode);

/* argument is the mode list index [0, nmodes-1] */
void *set_video_mode(int idx, int nbuf);
int set_text_mode(void);

void set_palette(int idx, int r, int g, int b);

enum {
	FLIP_NOW,
	FLIP_VBLANK
};
/* page flip and return pointer to the start of the display area (front buffer) */
void *page_flip(int vsync);
extern void (*blit_frame)(void *pixels, int vsync);

#ifdef __WATCOMC__
void wait_vsync(void);
#pragma aux wait_vsync = \
	"mov dx, 0x3da" \
	"l1:" \
	"in al, dx" \
	"and al, 0x8" \
	"jnz l1" \
	"l2:" \
	"in al, dx" \
	"and al, 0x8" \
	"jz l2" \
	modify[al dx];
#endif

#ifdef __DJGPP__
#define wait_vsync()  asm volatile ( \
	"mov $0x3da, %%dx\n\t" \
	"0:\n\t" \
	"in %%dx, %%al\n\t" \
	"and $8, %%al\n\t" \
	"jnz 0b\n\t" \
	"0:\n\t" \
	"in %%dx, %%al\n\t" \
	"and $8, %%al\n\t" \
	"jz 0b\n\t" \
	:::"%eax","%edx")
#endif


#ifdef __cplusplus
}
#endif

#endif	/* GFX_H_ */
