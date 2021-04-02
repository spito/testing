#ifndef CUT_APPLE_DEFINE_H
#define CUT_APPLE_DEFINE_H

#include <sys/types.h>
#include <sys/sysctl.h>

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
