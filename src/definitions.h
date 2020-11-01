#ifndef CUT_DEFINITIONS_H
#define CUT_DEFINITIONS_H

#define CUT_PRIVATE static

#ifdef __cplusplus
# define CUT_NS_BEGIN extern "C" {
# define CUT_NS_END }
#else
# define CUT_NS_BEGIN
# define CUT_NS_END
#endif

#if !defined(CUT_DEFAULT_TIMEOUT)
# define CUT_DEFAULT_TIMEOUT 3
#endif

#if !defined(CUT_NO_FORK)
# define CUT_NO_FORK cut_IsDebugger()
#else
# undef CUT_NO_FORK
# define CUT_NO_FORK 1
#endif

#if !defined(CUT_NO_COLOR)
# define CUT_NO_COLOR !cut_IsTerminalOutput()
#else
# undef CUT_NO_COLOR
# define CUT_NO_COLOR 1
#endif

#if !defined(CUT_MAX_PATH)
# define CUT_MAX_PATH 80
#endif

#if !defined(CUT_FORMAT)
# define CUT_FORMAT "standard"
#endif

#endif