#ifndef CUT_OS_SPECIFIC_H
#define CUT_OS_SPECIFIC_H

#if defined(__linux__) || defined(__APPLE__) || defined(__unix)
# include "common-nix.h"
#endif

#if defined(__linux__)
# include "linux-definitions.h"
#elif defined(__APPLE__)
# include "apple-definitions.h"
#elif defined(__unix)
# include "unix-definitions.h"
#elif defined(_WIN32)
# include "windows-definitions.h"
#else
# error "unsupported platform"
#endif

#endif