#ifndef CUT_WINDOWS_DEFINE_H
#define CUT_WINDOWS_DEFINE_H

#define _WIN32_WINNT 0x0500

#if defined(__GNUC__) || defined(__clang)
# define CUT_NORETURN __attribute__((noreturn))
# define CUT_CONSTRUCTOR(name) __attribute__((constructor)) static void name()
# define CUT_UNUSED(name) __attribute__((unused)) name
#elif defined(_MSC_VER)
# define CUT_NORETURN __declspec(noreturn)
# pragma section(".CRT$XCU",read)
# define CUT_CONSTRUCTOR(name)                                                  \
    static void __cdecl name( void );                                           \
    __declspec(allocate(".CRT$XCU")) void (__cdecl * name ## _)(void) = name;   \
    static void __cdecl name( void )
# define CUT_UNUSED(name) name
#else
# error "unsupported compiler"
#endif

#endif // CUT_WINDOWS_DEFINE_H
