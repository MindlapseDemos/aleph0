#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include "inttypes.h"

#if defined(__WATCOMC__) || defined(_WIN32) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif

#ifdef __GNUC__
#define INLINE __inline
#define PACKED __attribute__((packed))

#elif defined(__WATCOMC__)
#define INLINE __inline
#define PACKED

#else
#define INLINE
#define PACKED
#endif

#define BSWAP16(x)	((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#define BSWAP32(x)	\
	((((x) >> 24) & 0xff) | \
	 (((x) >> 8) & 0xff00) | \
	 (((x) << 8) & 0xff0000) | \
	 ((x) << 24))

#if defined(__i386__) || defined(__x86_64__) || defined(__386__) || defined(MSDOS)
/* fast conversion of double -> 32bit int
 * for details see:
 *  - http://chrishecker.com/images/f/fb/Gdmfp.pdf
 *  - http://stereopsis.com/FPU.html#convert
 */
static INLINE int32_t cround64(double val)
{
	val += 6755399441055744.0;
	return *(int32_t*)&val;
}
#else
#define cround64(x)	((int32_t)(x))
#endif

static INLINE float rsqrt(float x)
{
	float xhalf = x * 0.5f;
	int32_t i = *(int32_t*)&x;
	i = 0x5f3759df - (i >> 1);
	x = *(float*)&i;
	x = x * (1.5f - xhalf * x * x);
	return x;
}

extern uint32_t perf_start_count, perf_interval_count;

#ifdef __WATCOMC__
void memset16(void *dest, uint16_t val, int count);
#pragma aux memset16 = \
	"cld" \
	"test ecx, 1" \
	"jz memset16_dwords" \
	"rep stosw" \
	"jmp memset16_done" \
	"memset16_dwords:" \
	"shr ecx, 1" \
	"push ax" \
	"shl eax, 16" \
	"pop ax" \
	"rep stosd" \
	"memset16_done:" \
	parm[edi][ax][ecx];

#ifdef USE_MMX
void memcpy64(void *dest, void *src, int count);
#pragma aux memcpy64 = \
	"cploop:" \
	"movq mm0, [edx]" \
	"movq [ebx], mm0" \
	"add edx, 8" \
	"add ebx, 8" \
	"dec ecx" \
	"jnz cploop" \
	"emms" \
	parm[ebx][edx][ecx] \
	modify[8087];
#else
#define memcpy64(dest, src, count)	memcpy(dest, src, (count) << 3)
#endif

void perf_start(void);
#pragma aux perf_start = \
	"xor eax, eax" \
	"cpuid" \
	"rdtsc" \
	"mov [perf_start_count], eax" \
	modify[eax ebx ecx edx];

void perf_end(void);
#pragma aux perf_end = \
	"xor eax, eax" \
	"cpuid" \
	"rdtsc" \
	"sub eax, [perf_start_count]" \
	"mov [perf_interval_count], eax" \
	modify [eax ebx ecx edx];

void debug_break(void);
#pragma aux debug_break = "int 3";

void halt(void);
#pragma aux halt = "hlt";
#endif

#ifdef __GNUC__
#if defined(__i386__) || defined(__x86_64__)
#define memset16(dest, val, count) \
	do { \
		uint32_t dummy1, dummy2; \
		asm volatile ( \
			"cld\n\t" \
			"test $1, %%ecx\n\t" \
			"jz 0f\n\t" \
			"rep stosw\n\t" \
			"jmp 1f\n\t" \
			"0:\n\t" \
			"shr $1, %%ecx\n\t" \
			"push %%ax\n\t" \
			"shl $16, %%eax\n\t" \
			"pop %%ax\n\t" \
			"rep stosl\n\t" \
			"1:\n\t"\
			: "=D"(dummy1), "=c"(dummy2) \
			: "0"(dest), "a"((uint16_t)(val)), "1"(count) \
			: "flags", "memory"); \
	} while(0)
#else
static void INLINE memset16(void *dest, uint16_t val, int count)
{
	uint16_t *ptr = dest;
	while(count--) *ptr++ = val;
}
#endif

#ifdef USE_MMX
#define memcpy64(dest, src, count) asm volatile ( \
	"0:\n\t" \
	"movq (%1), %%mm0\n\t" \
	"movq %%mm0, (%0)\n\t" \
	"add $8, %1\n\t" \
	"add $8, %0\n\t" \
	"dec %2\n\t" \
	"jnz 0b\n\t" \
	"emms\n\t" \
	:: "r"(dest), "r"(src), "r"(count) \
	: "%mm0")
#else
#define memcpy64(dest, src, count)	memcpy(dest, src, (count) << 3)
#endif

#define perf_start()  asm volatile ( \
	"xor %%eax, %%eax\n" \
	"cpuid\n" \
	"rdtsc\n" \
	"mov %%eax, %0\n" \
	: "=m"(perf_start_count) \
	:: "%eax", "%ebx", "%ecx", "%edx")

#define perf_end() asm volatile ( \
	"xor %%eax, %%eax\n" \
	"cpuid\n" \
	"rdtsc\n" \
	"sub %1, %%eax\n" \
	"mov %%eax, %0\n" \
	: "=m"(perf_interval_count) \
	: "m"(perf_start_count) \
	: "%eax", "%ebx", "%ecx", "%edx")

#define debug_break() \
	asm volatile("int $3")

#define halt() \
	asm volatile("hlt")
#endif

#ifdef _MSC_VER
static void __inline memset16(void *dest, uint16_t val, int count)
{
	__asm {
		cld
		mov ax, val
		mov edi, dest
		mov ecx, count
		test ecx, 1
		jz memset16_dwords
		rep stosw
		jmp memset16_done
		memset16_dwords:
		shr ecx, 1
		push ax
		shl eax, 16
		pop ax
		rep stosd
		memset16_done:
	}
}

#define perf_start() \
	do { \
		__asm { \
			xor eax, eax \
			cpuid \
			rdtsc \
			mov [perf_start_count], eax \
		} \
	} while(0)

#define perf_end() \
	do { \
		__asm { \
			xor eax, eax \
			cpuid \
			rdtsc \
			sub eax, [perf_start_count] \
			mov [perf_interval_count], eax \
		} \
	} while(0)

#define debug_break() \
	do { \
		__asm { int 3 } \
	} while(0)

static unsigned int __inline get_cs(void)
{
	unsigned int res;
	__asm {
		xor eax, eax
		mov ax, cs
		mov [res], eax
	}
	return res;
}
#endif

unsigned int get_cs(void);
#define get_cpl()	((int)(get_cs() & 3))

void get_msr(uint32_t msr, uint32_t *low, uint32_t *high);
void set_msr(uint32_t msr, uint32_t low, uint32_t high);


/* Non-failing versions of malloc/calloc/realloc. They never return 0, they call
 * demo_abort on failure. Use the macros, don't call the *_impl functions.
 */
#define malloc_nf(sz)	malloc_nf_impl(sz, __FILE__, __LINE__)
void *malloc_nf_impl(size_t sz, const char *file, int line);
#define calloc_nf(n, sz)	calloc_nf_impl(n, sz, __FILE__, __LINE__)
void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line);
#define realloc_nf(p, sz)	realloc_nf_impl(p, sz, __FILE__, __LINE__)
void *realloc_nf_impl(void *p, size_t sz, const char *file, int line);
#define strdup_nf(s)	strdup_nf_impl(s, __FILE__, __LINE__)
char *strdup_nf_impl(const char *s, const char *file, int line);

void enable_fpexcept(void);
void disable_fpexcept(void);

#endif	/* UTIL_H_ */
