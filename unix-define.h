#ifndef CUT_UNIX_DEFINE_H
#define CUT_UNIX_DEFINE_H

#if defined(__GNUC__) || defined(__clang)
# define CUT_NORETURN __attribute__((noreturn))
# define CUT_CONSTRUCTOR(name) __attribute__((constructor)) static void name()
#else
# error "unsupported compiler"
#endif

#endif // CUT_UNIX_DEFINE_H