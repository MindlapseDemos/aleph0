#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "util.h"
#include "demo.h"

void (*memcpy64)(void *dest, void *src, int count) = memcpy64_nommx;

uint32_t perf_start_count, perf_interval_count;


void *malloc_nf_impl(size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = malloc(sz))) {
		fprintf(stderr, "%s:%d failed to allocate %lu bytes\n", file, line, (unsigned long)sz);
		demo_abort();
	}
	return p;
}

void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = calloc(num, sz))) {
		fprintf(stderr, "%s:%d failed to allocate %lu bytes\n", file, line, (unsigned long)(num * sz));
		demo_abort();
	}
	return p;
}

void *realloc_nf_impl(void *p, size_t sz, const char *file, int line)
{
	if(!(p = realloc(p, sz))) {
		fprintf(stderr, "%s:%d failed to realloc %lu bytes\n", file, line, (unsigned long)sz);
		demo_abort();
	}
	return p;
}

char *strdup_nf_impl(const char *s, const char *file, int line)
{
	int len;
	char *res;

	len = strlen(s);
	if(!(res = malloc(len + 1))) {
		fprintf(stderr, "%s:%d failed to duplicate string\n", file, line);
		demo_abort();
	}
	memcpy(res, s, len + 1);
	return res;
}

#if defined(__APPLE__) && !defined(TARGET_IPHONE)
#include <xmmintrin.h>

void enable_fpexcept(void)
{
	unsigned int bits;
	bits = _MM_MASK_INVALID | _MM_MASK_DIV_ZERO | _MM_MASK_OVERFLOW | _MM_MASK_UNDERFLOW;
	_MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~bits);
}

void disable_fpexcept(void)
{
	unsigned int bits;
	bits = _MM_MASK_INVALID | _MM_MASK_DIV_ZERO | _MM_MASK_OVERFLOW | _MM_MASK_UNDERFLOW;
	_MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() | bits);
}

#elif defined(__GLIBC__) && !defined(__MINGW32__)
#define __USE_GNU
#include <fenv.h>

void enable_fpexcept(void)
{
	feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
}

void disable_fpexcept(void)
{
	fedisableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
}

#elif defined(_MSC_VER) || defined(__MINGW32__) || defined(__WATCOMC__)
#include <float.h>

#if defined(__MINGW32__) && !defined(_EM_OVERFLOW)
/* if gcc's float.h gets precedence, the mingw MSVC includes won't be declared */
#define _MCW_EM			0x8001f
#define _EM_INVALID		0x10
#define _EM_ZERODIVIDE	0x08
#define _EM_OVERFLOW	0x04
unsigned int __cdecl _clearfp(void);
unsigned int __cdecl _controlfp(unsigned int, unsigned int);
#elif defined(__WATCOMC__)
#define _clearfp	_clear87
#define _controlfp	_control87
#endif

void enable_fpexcept(void)
{
	_clearfp();
	_controlfp(_controlfp(0, 0) & ~(_EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW), _MCW_EM);
}

void disable_fpexcept(void)
{
	_clearfp();
	_controlfp(_controlfp(0, 0) | (_EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW), _MCW_EM);
}
#else
void enable_fpexcept(void) {}
void disable_fpexcept(void) {}
#endif

#ifdef __WATCOMC__
int strcasecmp(const char *a, const char *b)
{
	int ca, cb;

	while(*a && *b) {
		ca = tolower(*a++);
		cb = tolower(*b++);
		if(ca != cb) return ca - cb;
	}

	if(!*a && !*b) return 0;
	if(!*a) return -1;
	if(!*b) return 1;
	return 0;
}

int strncasecmp(const char *a, const char *b, size_t n)
{
	int ca, cb;
	while(n-- > 0 && *a && *b) {
		ca = tolower(*a++);
		cb = tolower(*b++);
		if(ca != cb) return ca - cb;
	}

	if(!*a && !*b) return 0;
	if(!*a) return -1;
	if(!*b) return 1;
	return 0;
}
#endif

#ifdef NO_ASM
void memcpy64_mmx(void *dest, void *src, int count)
{
	memcpy(dest, src, count << 3);
}

void memcpy64_nommx(void *dest, void *src, int count)
{
	memcpy(dest, src, count << 3);
}
#endif

unsigned int next_pow2(unsigned int x)
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

unsigned int calc_shift(unsigned int x)
{
	int res = -1;
	while(x) {
		x >>= 1;
		++res;
	}
	return res;
}
