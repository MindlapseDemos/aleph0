/*
goat3d - 3D scene, and animation file format library.
Copyright (C) 2013-2018  John Tsiombikas <nuclear@member.fsf.org>

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
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

int goat3d_log_level = 256;

void goat3d_logmsg(int prio, const char *fmt, ...)
{
	va_list ap;

	if(goat3d_log_level < prio) {
		return;
	}

	fprintf(stderr, "goat3d: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
