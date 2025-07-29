#ifndef MIKMOD_CONFIG_H_
#define MIKMOD_CONFIG_H_

#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMCMP 1
#define HAVE_MEMORY_H 1
#define HAVE_POSIX_MEMALIGN 1
#define HAVE_SETENV 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRSTR 1

#if (!defined(_MSC_VER) || _MSC_VER >= 1800) && !defined(__sgi)
#define HAVE_STDINT_H 1
#define HAVE_SNPRINTF 1
#endif

#if defined(__unix__) || defined(unix) || defined(__APPLE__)
#define HAVE_DLFCN_H 1
#define HAVE_RTLD_GLOBAL 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_PTHREAD 1
#endif

#define STDC_HEADERS 1

#if defined(__linux__) && !defined(__ANDROID__)
#define DRV_ALSA	1
#endif

#ifdef __FreeBSD__
#define DRV_OSS		1
#define HAVE_SYS_SOUNDCARD_H
#endif

#ifdef __sgi
#define DRV_SGI		1
#endif

#ifdef __sun
#define DRV_SUN		1
#endif

#ifdef _WIN32
#if defined(_MSC_VER) && _MSC_VER <= 1200
/* old windows build with msvc6 uses SDL */
#define DRV_SDL	1
#else
/* for modern windows use directsound */
#define DRV_DS	1
#endif
#endif

#ifdef __EMSCRIPTEN__
#define DRV_SDL	1
#endif

#ifdef __APPLE__
#define DRV_OSX	1
#endif

#ifdef __ANDROID__
#define DRV_OSLES 1
#endif

#endif	/* MIKMOD_CONFIG_H_ */
