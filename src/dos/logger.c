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
#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

#define LOGFNAME	"demo.log"

static FILE *logfile;

void logger_output(FILE *fp)
{
	if(logfile) fclose(logfile);
	logfile = fp;
}

void printlog(const char *fmt, ...)
{
	va_list ap;

	if(!logfile) {
		if(!(logfile = fopen(LOGFNAME, "w"))) {
			return;
		}
		setvbuf(logfile, 0, _IOLBF, 0);
	}

	va_start(ap, fmt);
	vfprintf(logfile, fmt, ap);
	va_end(ap);
}
