#ifndef CUT_LINUX_DEFINE_H
#define CUT_LINUX_DEFINE_H

#if defined(__GNUC__) || defined(__clang)
# define CUT_NORETURN __attribute__((noreturn))
# define CUT_CONSTRUCTOR(name) __attribute__((constructor)) static void name()
# define CUT_UNUSED(name) __attribute__((unused)) name
#else
# error "unsupported compiler"
#endif

#endif // CUT_LINUX_DEFINE_H
