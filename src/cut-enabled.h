#ifndef CUT_ENABLED_H
#define CUT_ENABLED_H

#define _POSIX_C_SOURCE 199309L

#include "os-specific.h"
#include "public-declarations.h"

#define ASSERT(e) do { if (!(e)) {                                                      \
        cut_Stop(#e, __FILE__, __LINE__);                                               \
    } } while(0)

#define ASSERT_FILE(f, content) do {                                                    \
    if (!cut_File(f, content)) {                                                        \
        cut_Stop("content of file is not equal", __FILE__, __LINE__);                   \
    } } while(0)

#define CHECK(e) do { if (!(e)) {                                                       \
        cut_Check(#e, __FILE__, __LINE__);                                              \
    } } while(0)

#define CHECK_FILE(f, content) do {                                                     \
    if (!cut_File(f, content)) {                                                        \
        cut_Check("content of file is not equal", __FILE__, __LINE__);                  \
    } } while(0)

#define INPUT(content) do {                                                             \
    if (!cut_Input(content)) {                                                          \
        cut_Stop("cannot set contents a an input file", __FILE__, __LINE__);            \
    } } while(0)

#define TEST_POINTS(n) .points = n
#define TEST_TIMEOUT(n) .timeout = n, .timeoutDefined = 1
#define TEST_SUPPRESS .suppress = 1
#define TEST_NEEDS(...) { "", ## __VA_ARGS__ }
#define TEST_SETTINGS(...) {"", ## __VA_ARGS__ }

#define CUT_GET_NEEDS2(dummy, settings, needs, ...) needs
#define CUT_GET_NEEDS(dummy, ...) CUT_GET_NEEDS2(dummy, ## __VA_ARGS__, {""}, {""})
#define CUT_GET_SETTINGS2(dummy, settings, needs, ...) settings
#define CUT_GET_SETTINGS(dummy, ...) CUT_GET_SETTINGS2(dummy, ## __VA_ARGS__, {""}, {""})

#define TEST(name, ...)                                                                 \
    void cut_instance_ ## name(int *, int);                                             \
    CUT_CONSTRUCTOR(cut_Register ## name) {                                             \
        static struct cut_Settings settings = CUT_GET_SETTINGS(dummy, ## __VA_ARGS__);  \
        static const char *needs[] = CUT_GET_NEEDS(dummy, ## __VA_ARGS__);              \
        settings.needSize = sizeof(needs)/sizeof(*needs);                               \
        settings.needs = needs;                                                         \
        cut_Register(cut_instance_ ## name,                                             \
                     #name,  __FILE__, __LINE__, &settings);                            \
    }                                                                                   \
    void cut_instance_ ## name(CUT_UNUSED(int *cut_subtest),                            \
                               CUT_UNUSED(int cut_current))

#define GLOBAL_TEAR_UP()                                                                \
    void cut_GlobalTearUpInstance();                                                    \
    CUT_CONSTRUCTOR(cut_RegisterTearUp) {                                               \
        cut_RegisterGlobalTearUp(cut_GlobalTearUpInstance);                             \
    }                                                                                   \
    void cut_GlobalTearUpInstance()

#define GLOBAL_TEAR_DOWN()                                                              \
    void cut_GlobalTearUpInstance();                                                    \
    CUT_CONSTRUCTOR(cut_RegisterTearDown) {                                             \
        cut_RegisterGlobalTearDown(cut_GlobalTearDownInstance);                         \
    }                                                                                   \
    void cut_GlobalTearDownInstance()

#define SUBTEST(name)                                                                   \
    if (++*cut_subtest == cut_current)                                                  \
        cut_Subtest(0, name);                                                           \
    if (*cut_subtest == cut_current)

#define REPEATED_SUBTEST(name, count)                                                   \
    *cut_subtest = (count);                                                             \
    if (cut_current && count)                                                           \
        cut_Subtest(cut_current, name);                                                 \
    if (cut_current && count)

#define SUBTEST_NO cut_current

#define DEBUG_MSG(...) cut_DebugMessage(__FILE__, __LINE__, __VA_ARGS__)

#include <stdio.h>

CUT_NS_BEGIN

typedef void(*cut_Instance)(int *, int);
typedef void(*cut_GlobalTear)();
void cut_Register(cut_Instance instance, const char *name, const char *file, size_t line, struct cut_Settings *settings);
void cut_RegisterGlobalTearUp(cut_GlobalTear instance);
void cut_RegisterGlobalTearDown(cut_GlobalTear instance);
int cut_File(FILE *file, const char *content);
CUT_NORETURN void cut_Stop(const char *text, const char *file, size_t line);
void cut_Check(const char *text, const char *file, size_t line);
int cut_Input(const char *content);
void cut_Subtest(int number, const char *name);
void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...);

CUT_NS_END

#endif