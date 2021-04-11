#ifndef CUT_PUBLIC_DECLARATIONS_H
#define CUT_PUBLIC_DECLARATIONS_H

#include "definitions.h"

#include <stdio.h>

CUT_NS_BEGIN

#define CUT_MODE_TEXT 0
#define CUT_MODE_BINARY 1

struct cut_Settings {
    int timeout;
    int timeoutDefined;
    int suppress;
    double points;
    const char **needs;
    unsigned long needSize;
};

extern const char *cut_needs[1];
typedef void(*cut_Instance)(int *, int);
typedef void(*cut_Reporter)(const char *, const char *, unsigned);

CUT_NORETURN void cut_Stop(const char *text, const char *file, unsigned line);
void cut_Check(const char *text, const char *file, unsigned line);
void cut_Debug(const char *text, const char *file, unsigned line);

void cut_Register(cut_Instance instance, const char *name, const char *file, unsigned line, struct cut_Settings *settings);
void cut_FileCompare(cut_Reporter reporter, FILE *fd, const char *content, int mode, const char *fdStr, const char *file, unsigned line);
void cut_FormatMessage(cut_Reporter reporter, const char *file, unsigned line, const char *fmt, ...);
int cut_Input(const char *content);
void cut_Subtest(int number, const char *name);

CUT_NS_END

#endif