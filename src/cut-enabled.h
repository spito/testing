#ifndef CUT_ENABLED_H
#define CUT_ENABLED_H

#include "os-specific.h"
#include "public-declarations.h"

#define ASSERT(e) do {                                                                  \
    if (!(e)) {                                                                         \
        cut_Stop(#e, __FILE__, __LINE__);                                               \
    } } while(0)

#define ASSERT_FILE(f, content)                                                         \
    cut_FileCompare(cut_Stop, f, content, CUT_MODE_TEXT, #f, __FILE__, __LINE__)

#define ASSERT_FILE_BINARY(f, content)                                                  \
    cut_FileCompare(cut_Stop, f, content, CUT_MODE_BINARY, #f, __FILE__, __LINE__)

#define CHECK(e) do {                                                                   \
    if (!(e)) {                                                                         \
        cut_Check(#e, __FILE__, __LINE__);                                              \
    } } while(0)

#define CHECK_FILE(f, content)                                                          \
    cut_FileCompare(cut_Check, f, content, CUT_MODE_TEXT, #f, __FILE__, __LINE__)

#define CHECK_FILE_BINARY(f, content)                                                   \
    cut_FileCompare(cut_Check, f, content, CUT_MODE_BINARY, #f, __FILE__, __LINE__)

#define INPUT(content) do {                                                             \
    if (!cut_Input(content)) {                                                          \
        cut_Stop("cannot set contents as an input file", __FILE__, __LINE__);           \
    } } while(0)

#define TEST_POINTS(n) cut_setup.points = n
#define TEST_TIMEOUT(n) cut_setup.timeout = n, cut_setup.timeoutDefined = 1
#define TEST_SUPPRESS cut_setup.suppress = 1
#define TEST_NEEDS(...) (void)0 ; static const char *cut_needs[] = {"", ## __VA_ARGS__ }; (void)0


#define CUT_TEST_NAME(name) cut_Test_ ## name
#define CUT_REGISTER_NAME(name) cut_Register_ ## name

#define TEST(testName, ...)                                                             \
    void CUT_TEST_NAME(testName) (int , int);                                           \
    CUT_CONSTRUCTOR(CUT_REGISTER_NAME(testName)) {                                      \
        static struct cut_Setup cut_setup = CUT_SETUP_INIT;                             \
        __VA_ARGS__ ;                                                                   \
        cut_setup.test = CUT_TEST_NAME(testName);                                       \
        cut_setup.name = #testName;                                                     \
        cut_setup.file = __FILE__;                                                      \
        cut_setup.line = 0;                                                             \
        cut_setup.needSize = sizeof(cut_needs)/sizeof(*cut_needs);                      \
        cut_setup.needs = cut_needs;                                                    \
        cut_Register(&cut_setup);                                                       \
    }                                                                                   \
    void CUT_TEST_NAME(testName) (CUT_UNUSED(int cut_subtest),                          \
                                  CUT_UNUSED(int cut_current))

#define SUBTEST(name)                                                                   \
    ++cut_subtest;                                                                      \
    if (!cut_current)                                                                   \
        cut_RegisterSubtest(cut_subtest, name);                                         \
    if (cut_subtest == cut_current)

#define REPEATED_SUBTEST(name, count)                                                   \
    ASSERT(0 < count);                                                                  \
    cut_subtest += (count);                                                             \
    if (!cut_current)                                                                   \
        cut_RegisterSubtest(cut_subtest, name);                                         \
    if (cut_subtest - (count) < cut_current && cut_current <= cut_subtest)

#define SUBTEST_NO cut_current

#define DEBUG_MSG(...) cut_FormatMessage(cut_Debug, __FILE__, __LINE__, __VA_ARGS__)

#endif