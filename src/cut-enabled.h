#ifndef CUT_ENABLED_H
#define CUT_ENABLED_H

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
        cut_Stop("cannot set contents as an input file", __FILE__, __LINE__);            \
    } } while(0)

#define TEST_POINTS(n) settings.points = n
#define TEST_TIMEOUT(n) settings.timeout = n, settings.timeoutDefined = 1
#define TEST_SUPPRESS settings.suppress = 1
#define TEST_NEEDS(...) (void)0 ; static const char *cut_needs[] = {"", ## __VA_ARGS__ }; (void)0

#define CUT_APPLY_ARGS(dummy, ...) (void)0, ## __VA_ARGS__ ;

#define TEST(name, ...)                                                                 \
    void cut_instance_ ## name(int *, int);                                             \
    CUT_CONSTRUCTOR(cut_Register ## name) {                                             \
        static struct cut_Settings settings = {};                                       \
        CUT_APPLY_ARGS(dummy, ## __VA_ARGS__ );                                         \
        settings.needSize = sizeof(cut_needs)/sizeof(*cut_needs);                       \
        settings.needs = cut_needs;                                                     \
        cut_Register(cut_instance_ ## name,                                             \
                     #name, __FILE__, __LINE__, &settings);                             \
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

extern const char *cut_needs[1];
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