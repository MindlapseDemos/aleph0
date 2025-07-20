#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <math.h>
#include "inttypes.h"

#if defined(__WATCOMC__) || defined(_WIN32) || defined(__DJGPP__)
#include <malloc.h>
#else
#include <alloca.h>
#endif

#ifndef M_PI
#define M_PI	3.1415926535
#endif

#define INLINE __inline

#ifdef __GNUC__
#define PACKED __attribute__((packed))

#elif defined(__WATCOMC__)
#define PACKED

#else
#define PACKED
#endif

#define BSWAP16(x)	((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#define BSWAP32(x)	\
	((((x) >> 24) & 0xff) | \
	 (((x) >> 8) & 0xff00) | \
	 (((x) << 8) & 0xff0000) | \
	 ((x) << 24))

void memcpy64_mmx(void *dest, void *src, int count);
void memcpy64_nommx(void *dest, void *src, int count);

extern void (*memcpy64)(void *dest, void *src, int count);

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

static INLINE void read_unaligned32(void *dest, void *src)
{
	*(uint32_t*)dest = *(uint32_t*)src;
}
#else
static INLINE int32_t cround64(double val)
{
	if(val < 0.0f) {
		return (int32_t)(val - 0.5f);
	}
	return (int32_t)(val + 0.5f);
}

static INLINE void read_unaligned32(void *dest, void *src)
{
	uint32_t *dptr = dest;
	unsigned char *bsrc = src;
#ifdef BUILD_BIGENDIAN
	*dptr = (bsrc[0] << 24) | (bsrc[1] << 16) | (bsrc[2] << 8) | bsrc[3];
#else
	*dptr = (bsrc[3] << 24) | (bsrc[2] << 16) | (bsrc[1] << 8) | bsrc[0];
#endif
}
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

#define fast_vnormalize(vptr)	fast_normalize((float*)(vptr))
static INLINE void fast_normalize(float *v)
{
	float s = rsqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
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

#ifndef NO_PENTIUM
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
#endif	/* !def NO_PENTIUM */

void debug_break(void);
#pragma aux debug_break = "int 3";

void halt(void);
#pragma aux halt = "hlt";
#endif

#ifdef __GNUC__
#ifndef NO_ASM
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
#endif	/* !NO_ASM */

#ifndef NO_PENTIUM
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
#endif	/* !def NO_PENTIUM */

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

#ifdef NO_PENTIUM
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
#endif	/* !def NO_PENTIUM */

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

#ifdef NO_ASM
static void INLINE memset16(void *dest, uint16_t val, int count)
{
	uint16_t *ptr = dest;
	while(count--) *ptr++ = val;
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

#ifdef __WATCOMC__
int strcasecmp(const char *a, const char *b);
int strncasecmp(const char *a, const char *b, size_t n);
#endif

unsigned int next_pow2(unsigned int x);
unsigned int calc_shift(unsigned int x);

#endif	/* UTIL_H_ */
