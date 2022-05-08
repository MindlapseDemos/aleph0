#ifndef DOSUTIL_H_
#define DOSUTIL_H_

#include <dos.h>
#include <conio.h>

#ifdef __DJGPP__
#include <pc.h>

#define outp(p, v)	outportb(p, v)
#define outpw(p, v)	outportw(p, v)

#define inp(p)		inportb(p)
#define inpw(p)		inportw(p)
#endif

#endif	/* DOSUTIL_H_ */