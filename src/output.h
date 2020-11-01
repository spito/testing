#ifndef CUT_OUTPUT_H
#define CUT_OUTPUT_H

#include "declarations.h"
#include "output-json.h"
#include "output-standard.h"

CUT_NS_BEGIN

CUT_PRIVATE const char *cut_GetStatus(const struct cut_UnitResult *result, enum cut_Colors *color) {
    static const char *unknown = "UNKNOWN";
    static const char *ok = "OK";
    static const char *suppressed = "SUPPRESSED";
    static const char *filteredOut = "FILTERED OUT";
    static const char *returnedNonZero = "NON ZERO";
    static const char *signalled = "SIGNALLED";
    static const char *timedOut = "TIMED OUT";
    static const char *fail = "FAIL";
    static const char *skipped = "SKIPPED";
    static const char *internalFail = "INTERNAL ERROR";

    switch (result->status) {
    case cut_RESULT_OK:
        *color = cut_GREEN_COLOR;
        return ok;
    case cut_RESULT_SUPPRESSED:
        *color = cut_YELLOW_COLOR;
        return suppressed;
    case cut_RESULT_FILTERED_OUT:
        *color = cut_GREEN_COLOR;
        return filteredOut;
    case cut_RESULT_FAILED:
        *color = cut_RED_COLOR;
        return fail;
    case cut_RESULT_RETURNED_NON_ZERO:
        *color = cut_RED_COLOR;
        return returnedNonZero;
    case cut_RESULT_SIGNALLED:
        *color = cut_RED_COLOR;
        return signalled;
    case cut_RESULT_TIMED_OUT:
        *color = cut_RED_COLOR;
        return timedOut;
    case cut_RESULT_SKIPPED:
        *color = cut_YELLOW_COLOR;
        return skipped;
    case cut_RESULT_UNKNOWN:
    default:
        *color = cut_YELLOW_COLOR;
        return unknown;
    }
}

CUT_PRIVATE const char *cut_ShortPath(const struct cut_Arguments *arguments, const char *path) {
    static char shortenedPath[CUT_MAX_PATH + 1];
    char *cursor = shortenedPath;
    const char *dots = "...";
    const size_t dotsLength = strlen(dots);
    int pathLength = strlen(path);
    if (arguments->shortPath < 0 || pathLength <= arguments->shortPath)
        return path;
    
    int fullName = 0;
    const char *end = path + strlen(path);
    const char *name = end;
    for (; end - name < arguments->shortPath && path < name; --name) {
        if (*name == '/' || *name == '\\') {
            fullName = 1;
            break;
        }
    }
    int consumed = (end - name) + dotsLength;
    if (consumed < arguments->shortPath) {
        size_t remaining = arguments->shortPath - consumed;
        size_t firstPart = remaining - remaining / 2;
        strncpy(cursor, path, firstPart);
        cursor += firstPart;
        strcpy(cursor, dots);
        cursor += dotsLength;
        remaining -= firstPart;
        name -= remaining;
    }
    strcpy(cursor, name);
    return shortenedPath;
}

CUT_PRIVATE const char *cut_Signal(int signal) {
    static char number[16];
    const char *names[] = {
        "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT",
        "SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV", "SIGSYS",
        "SIGPIPE", "SIGALRM", "SIGTERM", "SIGUSR1", "SIGUSR2"
    };
    if (0 < signal <= sizeof(names) / sizeof(*names))
        sprintf(number, "%s (%d)", names[signal - 1], signal);
    else
        sprintf(number, "%d", signal);
    return number;
}

CUT_PRIVATE const char *cut_ReturnCode(int returnCode) {
    static char number[16];
    switch (returnCode) {
    case cut_ERROR_EXIT:
        return "ERROR EXIT";
    case cut_FATAL_EXIT:
        return "FATAL EXIT";
    default:
        sprintf(number, "%d", returnCode);
        return number;
    }
}

CUT_PRIVATE void cut_ShepherdNoop(struct cut_Shepherd *shepherd, ...) {
}

CUT_NS_END

#endif
