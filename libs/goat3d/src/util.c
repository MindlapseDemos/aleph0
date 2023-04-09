/*
goat3d - 3D scene, and animation file format library.
Copyright (C) 2013-2023  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <string.h>
#include "util.h"

static int b64bits(int c);

int calc_b64_size(const char *s)
{
	int len = strlen(s);
	const char *end = s + len;
	while(end > s && *--end == '=') len--;
	return len * 3 / 4;
}


GOAT3DAPI void *goat3d_b64decode(const char *str, void *buf, int *bufsz)
{
	unsigned char *dest, *end;
	unsigned char acc;
	int bits, sz;
	unsigned int gidx;

	if(buf) {
		sz = *bufsz;
	} else {
		sz = calc_b64_size(str);
		if(!(buf = malloc(sz))) {
			return 0;
		}
		if(bufsz) *bufsz = sz;
	}
	dest = buf;
	end = (unsigned char*)buf + sz;

	sz = 0;
	gidx = 0;
	acc = 0;
	while(*str) {
		if((bits = b64bits(*str++)) == -1) {
			continue;
		}

		switch(gidx++ & 3) {
		case 0:
			acc = bits << 2;
			break;
		case 1:
			if(dest < end) *dest = acc | (bits >> 4);
			dest++;
			acc = bits << 4;
			break;
		case 2:
			if(dest < end) *dest = acc | (bits >> 2);
			dest++;
			acc = bits << 6;
			break;
		case 3:
			if(dest < end) *dest = acc | bits;
			dest++;
		default:
			break;
		}
	}

	if(gidx & 3) {
		if(dest < end) *dest = acc;
		dest++;
	}

	if(bufsz) *bufsz = dest - (unsigned char*)buf;
	return buf;
}

static int b64bits(int c)
{
	if(c >= 'A' && c <= 'Z') {
		return c - 'A';
	}
	if(c >= 'a' && c <= 'z') {
		return c - 'a' + 26;
	}
	if(c >= '0' && c <= '9') {
		return c - '0' + 52;
	}
	if(c == '+') return 62;
	if(c == '/') return 63;

	return -1;
}

GOAT3DAPI void goat3d_bswap32(void *buf, int count)
{
	int i;
	register uint32_t x;
	uint32_t *ptr = buf;

	for(i=0; i<count; i++) {
		x = *ptr;
		*ptr++ = (x >> 24) | (x << 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000);
	}
}
