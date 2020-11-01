#ifndef CUT_CORE_H
#define CUT_CORE_H

#if defined(CUT) || defined(CUT_MAIN) || defined(DEBUG)
# define CUT_ENABLED
#elif defined(CUT_ENABLED)
# undef CUT_ENABLED
#endif

#if defined(CUT_ENABLED)
# include "cut-enabled.h"
# if defined(CUT_MAIN)
#  include "main.h"
# endif
#else
# include "cut-disabled.h"
#endif

#endif // CUT_CORE_H
