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

#endif	/* UTIL_H_ */
