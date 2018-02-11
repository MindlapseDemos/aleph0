#ifndef UTIL_H_
#define UTIL_H_

#include "inttypes.h"

#ifdef __GNUC__
#define INLINE __inline

#elif defined(__WATCOMC__)
#define INLINE __inline

#else
#define INLINE
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

uint32_t perf_start_count, perf_interval_count;

#ifdef __WATCOMC__
void perf_start(void);
#pragma aux perf_start = \
	"rdtsc" \
	"mov [perf_start_count], eax" \
	modify[eax edx];

void perf_end(void);
#pragma aux perf_end = \
	"rdtsc" \
	"sub eax, [perf_start_count]" \
	"mov [perf_interval_count], eax" \
	modify [eax edx];
#endif

#ifdef __GNUC__
#define perf_start()  asm volatile ( \
	"rdtsc\n" \
	"mov %%eax, %0\n" \
	: "=m"(perf_start_count) :: "%eax", "%edx")

#define perf_end() asm volatile ( \
	"rdtsc\n" \
	"sub %1, %%eax\n" \
	"mov %%eax, %0\n" \
	: "=m"(perf_interval_count) \
	: "m"(perf_start_count) \
	: "%eax", "%edx")
#endif

#ifdef _MSC_VER
#define perf_start() \
	do { \
		__asm { \
			rdtsc \
			mov [perf_start_count], eax \
		} \
	} while(0)

#define perf_end() \
	do { \
		__asm { \
			rdtsc \
			sub eax, [perf_start_count] \
			mov [perf_interval_count], eax \
		} \
	} while(0)
#endif

#endif	/* UTIL_H_ */
