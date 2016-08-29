#include "mouse.h"

typedef unsigned short uint16_t;

#define INTR	0x33

#define QUERY	0
#define SHOW	1
#define HIDE	2
#define READ	3
#define WRITE	4
#define PIXRATE	0xf

#define XLIM	7
#define YLIM	8

int have_mouse(void)
{
	uint16_t res = 0;
	_asm {
		mov eax, QUERY
		int INTR
		mov res, ax
	}
	return res;
}

void show_mouse(int show)
{
	uint16_t cmd = show ? SHOW : HIDE;
	_asm {
		mov ax, cmd
		int INTR
	}
}

int read_mouse(int *xp, int *yp)
{
	uint16_t x, y, state;
	_asm {
		mov eax, READ
		int INTR
		mov state, bx
		mov x, cx
		mov y, dx
	}

	if(xp) *xp = x;
	if(yp) *yp = y;
	return state;
}

void set_mouse(int x, int y)
{
	_asm {
		mov eax, WRITE
		mov ecx, x
		mov edx, y
		int INTR
	}
}

void set_mouse_limits(int xmin, int ymin, int xmax, int ymax)
{
	_asm {
		mov eax, XLIM
		mov ecx, xmin
		mov edx, xmax
		int INTR
		mov eax, YLIM
		mov ecx, ymin
		mov edx, ymax
		int INTR
	}
}

void set_mouse_rate(int xrate, int yrate)
{
	_asm {
		mov ax, PIXRATE
		mov ecx, xrate
		mov edx, yrate
		int INTR
	}
}

void set_mouse_mode(enum mouse_mode mode)
{
	if(mode == MOUSE_GFX) {
		set_mouse_rate(1, 1);
	} else {
		set_mouse_rate(8, 16);
	}
}
