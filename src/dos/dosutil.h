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
#ifndef DOSUTIL_H_
#define DOSUTIL_H_

#include <dos.h>
#include <conio.h>

#ifdef __DJGPP__
#include <pc.h>

#define outp(p, v)	outportb(p, v)
#define outpw(p, v)	outportw(p, v)
#define outpd(p, v)	outportl(p, v)

#define inp(p)		inportb(p)
#define inpw(p)		inportw(p)
#define inpd(p)		inportl(p)
#endif

#endif	/* DOSUTIL_H_ */
