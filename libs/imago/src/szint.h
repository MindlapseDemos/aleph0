#ifndef INT_TYPES_H_
#define INT_TYPES_H_

#if defined(__DOS__) || defined(__MSDOS__)
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

typedef long intptr_t;
typedef unsigned long uintptr_t;
#else

#ifdef _MSC_VER
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64	int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#ifdef _WIN64
typedef __int64 intptr_t;
typedef unsigned __int64 uintptr_t;
#else
typedef __int32 intptr_t;
typedef unsigned __int32 uintptr_t;
#endif
#else	/* not msvc */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199900
#include <stdint.h>
#elif defined(__sgi)
#include <sys/types.h>
#else
#include <inttypes.h>
#endif

#endif	/* end !msvc */
#endif	/* end !dos */

#endif	/* INT_TYPES_H_ */
