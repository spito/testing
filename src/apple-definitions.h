#ifndef CUT_APPLE_DEFINE_H
#define CUT_APPLE_DEFINE_H

#ifdef _XOPEN_SOURCE
#define CUT_XOPEN_SOURCE _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#ifdef _POSIX_C_SOURCE
#define CUT_POSIX_C_SOURCE _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include <sys/types.h>
#include <sys/sysctl.h>

#ifdef CUT_POSIX_C_SOURCE
#define _POSIX_C_SOURCE CUT_POSIX_C_SOURCE
#undef CUT_POSIX_C_SOURCE
#endif

#ifdef CUT_XOPEN_SOURCE
#define _XOPEN_SOURCE CUT_XOPEN_SOURCE
#undef CUT_XOPEN_SOURCE
#endif

#if defined(__GNUC__) || defined(__clang)
# define CUT_NORETURN __attribute__((noreturn))
# define CUT_CONSTRUCTOR(name) __attribute__((constructor)) static void name()
# define CUT_UNUSED(name) __attribute__((unused)) name
#else
# error "unsupported compiler"
#endif

#if defined(__clang__)
# pragma clang system_header
#elif defined(__GNUC__)
# pragma GCC system_header
#endif

#endif // CUT_APPLE_DEFINE_H
