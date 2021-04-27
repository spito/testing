#ifndef CUT_OS_SPECIFIC_H
#define CUT_OS_SPECIFIC_H

#if defined(_WIN32)
# include "windows-definitions.h"
#elif defined(__APPLE__)
# include "apple-definitions.h"
#elif defined(__linux__)
# include "linux-definitions.h"
#elif defined(__unix)
# include "unix-definitions.h"
#else
# error "unsupported platform"
#endif

#endif