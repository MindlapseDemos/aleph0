#define HAVE_LIMITS_H 1
#define HAVE_MEMCMP 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRSTR 1

#ifdef DOS
#define DRV_SB 1
#define DRV_ULTRA 1
#define DRV_WSS 1

#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_IOCTL_H 1
#endif

#if defined(__unix__) || defined(__APPLE__)
#define DRV_SDL 1

#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SNPRINTF 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_UNISTD_H 1
#define HAVE_POSIX_MEMALIGN 1
#define HAVE_PTHREAD 1
#define HAVE_SETENV 1
#define HAVE_SRANDOM 1

#define MIKMOD_UNIX 1
#endif


#if defined(WIN32)
#define DRV_SDL 1

#define HAVE_WINDOWS_H 1
#define HAVE_MALLOC_H 1

#define NO_SDL_CONFIG 1
#endif

#undef MIKMOD_DEBUG

/* disable the high quality mixer (build only with the standart mixer) */
#undef NO_HQMIXER

/* Name of package */
#undef PACKAGE
/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT
/* Define to the full name of this package. */
#undef PACKAGE_NAME
/* Define to the full name and version of this package. */
#undef PACKAGE_STRING
/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME
/* Define to the home page for this package. */
#undef PACKAGE_URL
/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Version number of package */
#undef VERSION

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#if !defined(__cplusplus) && defined(_MSC_VER)
#define inline __inline
#endif
