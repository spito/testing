#ifndef CUT_PUBLIC_DECLARATIONS_H
#define CUT_PUBLIC_DECLARATIONS_H

#include "definitions.h"

CUT_NS_BEGIN

struct cut_Settings {
    int timeout;
    int timeoutDefined;
    int suppress;
    double points;
    const char **needs;
    unsigned long needSize;
};

CUT_NS_END

#endif