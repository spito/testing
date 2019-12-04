#ifndef CUT_CORE_H
#define CUT_CORE_H

/* XXX: make it work on MacOS, broken otherwise due to some intereferences which prevent to define u_XXX types required in sysctl.h
Should not harm elsewhere.
 */

#include <sys/types.h>

#if !defined(CUT) && !defined(DEBUG) && !defined(CUT_MAIN)

# define ASSERT(e) (void)0
# define ASSERT_FILE(f, content) (void)0
# define CHECK(e) (void)0
# define CHECK_FILE(f, content) (void)0
# define INPUT(content) (void)0
# define TEST(name) static void unitTest_ ## name()
# define KONTR_TEST(name, points, prerequisities) static void unitTest_ ## name()
# define GLOBAL_TEAR_UP() static void cut_GlobalTearUpInstance()
# define GLOBAL_TEAR_DOWN() static void cut_GlobalTearDownInstance()
# define SUBTEST(name) if (0)
# define REPEATED_SUBTEST(name, count) if (0)
# define SUBTEST_NO 0
# define DEBUG_MSG(...) (void)0
# define TEST_POINTS(n) 
# define TEST_TIMEOUT(n) 
# define TEST_SUPPRESS 
# define TEST_NEEDS(...)
# define TEST_SETTINGS(...)

#else

# define CUT_PRIVATE static

# if !defined(CUT_DEFAULT_TIMEOUT)
#  define CUT_DEFAULT_TIMEOUT 3
# endif

# if !defined(CUT_NO_FORK)
#  define CUT_NO_FORK cut_IsDebugger()
# else
#  undef CUT_NO_FORK
#  define CUT_NO_FORK 1
# endif

# if !defined(CUT_NO_COLOR)
#  define CUT_NO_COLOR !cut_IsTerminalOutput()
# else
#  undef CUT_NO_COLOR
#  define CUT_NO_COLOR 1
# endif

# if !defined(CUT_MAX_PATH)
#  define CUT_MAX_PATH 80
# endif

# if !defined(CUT_FORMAT)
#  define CUT_FORMAT "standard"
# endif



# if defined(__linux__)
#  include "linux-define.h"
# elif defined(__APPLE__) || defined(__unix)
#  include "unix-define.h"
# elif defined(_WIN32)
#  include "windows-define.h"
# else
#  error "unsupported platform"
# endif

# define _POSIX_C_SOURCE 199309L
# define _XOPEN_SOURCE 500
# include <stdio.h>

# define ASSERT(e) do { if (!(e)) {                                             \
        cut_Stop(#e, __FILE__, __LINE__);                                       \
    } } while(0)

# define ASSERT_FILE(f, content) do {                                           \
    if (!cut_File(f, content)) {                                                \
        cut_Stop("content of file is not equal", __FILE__, __LINE__);           \
    } } while(0)

# define CHECK(e) do { if (!(e)) {                                              \
        cut_Check(#e, __FILE__, __LINE__);                                      \
    } } while(0)

# define CHECK_FILE(f, content) do {                                            \
    if (!cut_File(f, content)) {                                                \
        cut_Check("content of file is not equal", __FILE__, __LINE__);          \
    } } while(0)

# define INPUT(content) do {                                                    \
    if (!cut_Input(content)) {                                                  \
        cut_Stop("cannot set contents a an input file", __FILE__, __LINE__);    \
    } } while(0)
/*
# define KONTR_TEST(name, points, prerequisities)                               \
    void cut_instance_ ## name(int *, int);                                     \
    CUT_CONSTRUCTOR(cut_Register ## name) {                                     \
        const char *required[] = prerequisities;                                \
        size_t requiredSize = sizeof(required) / sizeof(*required);             \
        cut_Register(cut_instance_ ## name, #name, points, required,            \
                     requiredSize __FILE__, __LINE__);                          \
    }
    void cut_instance_ ## name(CUT_UNUSED(int *cut_subtest), CUT_UNUSED(int cut_current))
*/

# define TEST_POINTS(n) .points = n
# define TEST_TIMEOUT(n) .timeout = n, .timeoutDefined = 1
# define TEST_SUPPRESS .suppress = 1
# define TEST_NEEDS(...) { "", ## __VA_ARGS__ }
# define TEST_SETTINGS(...) {"", ## __VA_ARGS__ }

# define CUT_GET_NEEDS2(dummy, settings, needs, ...) needs
# define CUT_GET_NEEDS(dummy, ...) CUT_GET_NEEDS2(dummy, ## __VA_ARGS__, {""}, {""})
# define CUT_GET_SETTINGS2(dummy, settings, needs, ...) settings
# define CUT_GET_SETTINGS(dummy, ...) CUT_GET_SETTINGS2(dummy, ## __VA_ARGS__, {""}, {""})

# define TEST(name, ...)                                                        \
    void cut_instance_ ## name(int *, int);                                     \
    CUT_CONSTRUCTOR(cut_Register ## name) {                                     \
        static struct {                                                         \
            const char *dummy;                                                  \
            int timeout;                                                        \
            int timeoutDefined;                                                 \
            int suppress;                                                       \
            double points;                                                      \
            const char **needs;                                                 \
            size_t needSize;                                                    \
        } settings = CUT_GET_SETTINGS(dummy, ## __VA_ARGS__);                   \
        static const char *needs[] = CUT_GET_NEEDS(dummy, ## __VA_ARGS__);      \
        settings.needSize = sizeof(needs)/sizeof(*needs);                       \
        settings.needs = needs;                                                 \
        cut_Register(cut_instance_ ## name, #name,  __FILE__, __LINE__, &settings);        \
    }                                                                           \
    void cut_instance_ ## name(CUT_UNUSED(int *cut_subtest), CUT_UNUSED(int cut_current))

# define GLOBAL_TEAR_UP()                                                       \
    void cut_GlobalTearUpInstance();                                            \
    CUT_CONSTRUCTOR(cut_RegisterTearUp) {                                       \
        cut_RegisterGlobalTearUp(cut_GlobalTearUpInstance);                     \
    }                                                                           \
    void cut_GlobalTearUpInstance()

# define GLOBAL_TEAR_DOWN()                                                     \
    void cut_GlobalTearUpInstance();                                            \
    CUT_CONSTRUCTOR(cut_RegisterTearDown) {                                     \
        cut_RegisterGlobalTearDown(cut_GlobalTearDownInstance);                 \
    }                                                                           \
    void cut_GlobalTearDownInstance()

# define SUBTEST(name)                                                          \
    if (++*cut_subtest == cut_current)                                          \
        cut_Subtest(0, #name);                                                  \
    if (*cut_subtest == cut_current)

# define REPEATED_SUBTEST(name, count)                                          \
    *cut_subtest = (count);                                                     \
    if (cut_current && count)                                                   \
        cut_Subtest(cut_current, #name);                                        \
    if (cut_current && count)

# define SUBTEST_NO cut_current

# define DEBUG_MSG(...) cut_DebugMessage(__FILE__, __LINE__, __VA_ARGS__)

# ifdef __cplusplus
extern "C" {
# endif

typedef void(*cut_Instance)(int *, int);
typedef void(*cut_GlobalTear)();
void cut_Register(cut_Instance instance, const char *name, const char *file, size_t line, void *settings);
void cut_RegisterGlobalTearUp(cut_GlobalTear instance);
void cut_RegisterGlobalTearDown(cut_GlobalTear instance);
int cut_File(FILE *file, const char *content);
CUT_NORETURN void cut_Stop(const char *text, const char *file, size_t line);
void cut_Check(const char *text, const char *file, size_t line);
int cut_Input(const char *content);
void cut_Subtest(int number, const char *name);
void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...);

# if defined(CUT_MAIN)

#  include <stdlib.h>
#  include <string.h>
#  include <setjmp.h>
#  include <stdarg.h>


#  include "fragments.h"
#  include "declarations.h"
#  include "globals.h"
#  include "messages.h"
#  include "output.h"
#  include "execution.h"
#  include "queue.h"


#  if defined(__linux__)
#   include "linux.h"
#  elif defined(__APPLE__) || defined(__unix)
#   include "unix.h"
#  elif defined(_WIN32)
#   include "windows.h"
#  else
#   error "unsupported platform"
#  endif

CUT_NORETURN int cut_FatalExit(const char *reason) {
    if (cut_outputsRedirected) {
        FILE *log = fopen(cut_emergencyLog, "w");
        if (log) {
            fprintf(log, "CUT internal error during unit test: %s\n", reason);
            fclose(log);
        }
    } else {
        fprintf(stderr, "CUT internal error: %s\n", reason);
    }
    exit(cut_FATAL_EXIT);
}

CUT_NORETURN int cut_ErrorExit(const char *reason, ...) {
    va_list args;
    va_start(args, reason);
    fprintf(stderr, "Error happened: ");
    vfprintf(stderr, reason, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(cut_ERROR_EXIT);
}


void cut_Register(cut_Instance instance, const char *name, const char *file, size_t line, void *_settings) {
    struct cut_Settings *settings = (struct cut_Settings *)_settings;
    if (cut_unitTests.size == cut_unitTests.capacity) {
        cut_unitTests.capacity += 16;
        cut_unitTests.tests = (struct cut_UnitTest *)realloc(cut_unitTests.tests,
            sizeof(struct cut_UnitTest) * cut_unitTests.capacity);
        if (!cut_unitTests.tests)
            cut_FatalExit("cannot allocate memory for unit tests");
    }
    cut_unitTests.tests[cut_unitTests.size].instance = instance;
    cut_unitTests.tests[cut_unitTests.size].name = name;
    cut_unitTests.tests[cut_unitTests.size].file = file;
    cut_unitTests.tests[cut_unitTests.size].line = line;
    cut_unitTests.tests[cut_unitTests.size].settings = settings;
    ++cut_unitTests.size;
}

void cut_RegisterGlobalTearUp(cut_GlobalTear instance) {
    if (cut_globalTearUp)
        cut_FatalExit("cannot overwrite tear up function");
    cut_globalTearUp = instance;
}

void cut_RegisterGlobalTearDown(cut_GlobalTear instance) {
    if (cut_globalTearDown)
        cut_FatalExit("cannot overwrite tear down function");
    cut_globalTearDown = instance;
}

CUT_PRIVATE void cut_ParseArguments(struct cut_Arguments *arguments, int argc, char **argv) {
    static const char *help = "--help";
    static const char *list = "--list";
    static const char *timeout = "--timeout";
    static const char *noFork = "--no-fork";
    static const char *doFork = "--fork";
    static const char *noColor = "--no-color";
    static const char *output = "--output";
    static const char *subtest = "--subtest";
    static const char *exactTest = "--test";
    static const char *shortPath = "--short-path";
    static const char *format = "--format";
    arguments->help = 0;
    arguments->list = 0;
    arguments->timeout = CUT_DEFAULT_TIMEOUT;
    arguments->timeoutDefined = 0;
    arguments->noFork = CUT_NO_FORK;
    arguments->noColor = CUT_NO_COLOR;
    arguments->output = NULL;
    arguments->matchSize = 0;
    arguments->match = NULL;
    arguments->testId = -1;
    arguments->subtestId = -1;
    arguments->selfName = argv[0];
    arguments->shortPath = -1;
    arguments->format = CUT_FORMAT;

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--", 2)) {
            ++arguments->matchSize;
            continue;
        }
        if (!strcmp(help, argv[i])) {
            arguments->help = 1;
            continue;
        }
        if (!strcmp(list, argv[i])) {
            arguments->list = 1;
            continue;
        }
        if (!strcmp(timeout, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%u", &arguments->timeout))
                cut_ErrorExit("option %s requires numeric argument", timeout);
            arguments->timeoutDefined = 1;
            continue;
        }
        if (!strcmp(noFork, argv[i])) {
            arguments->noFork = 1;
            continue;
        }
        if (!strcmp(doFork, argv[i])) {
            arguments->noFork = 0;
            continue;
        }
        if (!strcmp(noColor, argv[i])) {
            arguments->noColor = 1;
            continue;
        }
        if (!strcmp(output, argv[i])) {
            ++i;
            if (i < argc) {
                arguments->output = argv[i];
                arguments->noColor = 1;
            }
            else {
                cut_ErrorExit("option %s requires string argument", output);
            }
            continue;
        }
        if (!strcmp(exactTest, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%d", &arguments->testId))
                cut_ErrorExit("option %s requires numeric argument %d %d", exactTest, i, argc);
            continue;
        }
        if (!strcmp(subtest, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%d", &arguments->subtestId))
                cut_ErrorExit("option %s requires numeric argument", subtest);
            continue;
        }
        if (!strcmp(shortPath, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%d", &arguments->shortPath))
                cut_ErrorExit("option %s requires numeric argument", shortPath);
            if (arguments->shortPath > CUT_MAX_PATH)
                arguments->shortPath = CUT_MAX_PATH;
            continue;
        }
        if (!strcmp(format, argv[i])) {
            ++i;
            if (i < argc)
                arguments->format = argv[i];
            else
                cut_ErrorExit("option %s requires string argument", format);
            continue;
        }
        cut_ErrorExit("option %s is not recognized", argv[i]);
    }
    if (!arguments->matchSize)
        return;
    arguments->match = (char **)malloc(arguments->matchSize * sizeof(char *));
    if (!arguments->match)
        cut_ErrorExit("cannot allocate memory for list of selected tests");
    int index = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strncmp(argv[i], "--", 2)) {
            if (!strcmp(timeout, argv[i]) || !strcmp(output, argv[i])
             || !strcmp(subtest, argv[i]) || !strcmp(exactTest, argv[i])
             || !strcmp(shortPath, argv[i]))
            {
                ++i;
            }
            continue;
        }
        arguments->match[index++] = argv[i];
    }
}

CUT_PRIVATE void cut_ClearInfo(struct cut_Info *info) {
    while (info) {
        struct cut_Info *current = info;
        info = info->next;
        free(current->message);
        free(current->file);
        free(current);
    }
}

CUT_PRIVATE void cut_ClearUnitResult(struct cut_UnitResult *result) {
    free(result->name);
    free(result->file);
    free(result->statement);
    free(result->exceptionType);
    free(result->exceptionMessage);
    cut_ClearInfo(result->debug);
    cut_ClearInfo(result->check);
}

CUT_PRIVATE int cut_Help(const struct cut_Arguments *arguments) {
    const char *text = ""
    "Run as %s [options] [test names]\n"
    "\n"
    "Options:\n"
    "\t--help            Print out this help.\n"
    "\t--list            Print list of all tests. Respect --format.\n"
    "\t--format <type>   Use different kinds of output formats:\n"
    "\t         json     use json format\n"
    "\t         <?>      use standard terminal friendly format\n"
    "\t--timeout <N>     Set timeout of each test in seconds. 0 for no timeout.\n"
    "\t--no-fork         Disable forking. Timeout is turned off.\n"
    "\t--fork            Force forking. Usefull during debugging with fork enabled.\n"
    "\t--no-color        Turn off colors.\n"
    "\t--output <file>   Redirect output to the file.\n"
    "\t--short-path <N>  Make filenames in the output shorter.\n"
    "Hidden options (for internal purposes only):\n"
    "\t--test <N>        Run test of index N.\n"
    "\t--subtest <N>     Run subtest of index N (for all tests).\n"
    "\n"
    "Test names - any other parameter is accepted as a filter of test names. "
    "In case there is at least one filter parameter, a test is executed only if "
    "at least one of the filters is a substring of the test name."
    "";

    fprintf(stderr, text, arguments->selfName);
    return 0;
}

CUT_PRIVATE int cut_List(const struct cut_Shepherd *shepherd) {
    shepherd->listTests(shepherd);
    return 0;
}

int main(int argc, char **argv) {
    return cut_Runner(argc, argv);
}

#  undef CUT_PRIVATE

# endif // CUT_MAIN

# ifdef __cplusplus
} // extern "C"
# endif

#endif // CUT

#endif // CUT_CORE_H
