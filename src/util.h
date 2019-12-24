#ifndef UTIL_H_
#define UTIL_H_

#include "inttypes.h"

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

extern uint32_t perf_start_count, perf_interval_count;

#ifdef __WATCOMC__
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

void memset16(void *ptr, int val, int count);
#pragma aux memset16 = \
	"rep stosw" \
	parm[edi][eax][ecx];
#endif

#ifdef __GNUC__
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
	asm volatile ("int $3")

#define halt() \
	asm volatile("hlt")

#define memset16(ptr, val, count) asm volatile ( \
	"rep stosw\n\t" \
	:: "D"(ptr), "a"(val), "c"(count))
#endif

#ifdef _MSC_VER
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

#define memset16(ptr, val, count) \
	do { \
		__asm { \
			mov edi, ptr \
			mov ecx, count \
			mov eax, val \
			rep stosw \
		} \
	} while(0)
#endif

#endif	/* UTIL_H_ */
