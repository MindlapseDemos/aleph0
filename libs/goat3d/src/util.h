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
#ifndef GOAT3D_UTIL_H_
#define GOAT3D_UTIL_H_

#include "goat3d.h"

#ifndef _MSC_VER
#ifdef __sgi
#include <inttypes.h>
#else
#include <stdint.h>
#endif
#else
typedef unsigned __int32 uint32_t;
#endif

#if defined(__mips)
#define GOAT3D_BIGEND
#endif

int calc_b64_size(const char *s);

GOAT3DAPI void *goat3d_b64decode(const char *str, void *buf, int *bufsz);
#define b64decode goat3d_b64decode

GOAT3DAPI void goat3d_bswap32(void *buf, int count);

#endif	/* GOAT3D_UTIL_H_ */
