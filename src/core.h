#ifndef CUT_CORE_H
#define CUT_CORE_H

#if !defined(CUT) && !defined(DEBUG) && !defined(CUT_MAIN)

# define ASSERT(e) (void)0
# define ASSERT_FILE(f, content) (void)0
# define CHECK(e) (void)0
# define CHECK_FILE(f, content) (void)0
# define TEST(name) static void unitTest_ ## name()
# define SUBTEST(name) if (0)
# define REPEATED_SUBTEST(name, count) if (0)
# define SUBTEST_NO 0
# define DEBUG_MSG(...) (void)0


#else

# define CUT_PRIVATE static

# if !defined(CUT_TIMEOUT)
#  define CUT_TIMEOUT 3
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


# if defined(__unix)
#  include "unix-define.h"
# elif defined(__APPLE__)
#  include "apple-define.h"
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

# define TEST(name)                                                             \
    void cut_instance_ ## name(int *, int);                                     \
    CUT_CONSTRUCTOR(cut_Register ## name) {                                     \
        cut_Register(cut_instance_ ## name, #name, __FILE__, __LINE__);         \
    }                                                                           \
    void cut_instance_ ## name(CUT_UNUSED(int *cut_subtest), CUT_UNUSED(int cut_current))

# define SUBTEST(name)                                                          \
    if (++*cut_subtest == cut_current)                                          \
        cut_Subtest(0, #name);                                                  \
    if (*cut_subtest == cut_current)

# define REPEATED_SUBTEST(name, count)                                          \
    *cut_subtest = (count);                                                     \
    if (count)                                                                  \
        cut_Subtest(cut_current, #name);                                        \
    if (count)

# define SUBTEST_NO cut_current

# define DEBUG_MSG(...) cut_DebugMessage(__FILE__, __LINE__, __VA_ARGS__)

# ifdef __cplusplus
extern "C" {
# endif

typedef void(*cut_Instance)(int *, int);
void cut_Register(cut_Instance instance, const char *name, const char *file, size_t line);
int cut_File(FILE *file, const char *content);
CUT_NORETURN void cut_Stop(const char *text, const char *file, size_t line);
void cut_Check(const char *text, const char *file, size_t line);
void cut_Subtest(int number, const char *name);
void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...);

# if defined(CUT_MAIN)

#  include <stdlib.h>
#  include <string.h>
#  include <setjmp.h>
#  include <stdarg.h>


struct cut_Info {
    char *message;
    char *file;
    int line;
    struct cut_Info *next;
};

enum cut_MessageType {
    cut_NO_TYPE = 0,
    cut_MESSAGE_SUBTEST,
    cut_MESSAGE_DEBUG,
    cut_MESSAGE_OK,
    cut_MESSAGE_FAIL,
    cut_MESSAGE_EXCEPTION,
    cut_MESSAGE_TIMEOUT,
    cut_MESSAGE_CHECK
};

struct cut_UnitResult {
    char *name;
    int number;
    int subtests;
    int failed;
    char *file;
    int line;
    char *statement;
    char *exceptionType;
    char *exceptionMessage;
    int returnCode;
    int signal;
    int timeouted;
    struct cut_Info *debug;
    struct cut_Info *check;
};

struct cut_UnitTest {
    cut_Instance instance;
    const char *name;
    const char *file;
    size_t line;
};

struct cut_UnitTestArray {
    int size;
    int capacity;
    struct cut_UnitTest *tests;
};

struct cut_Arguments {
    int help;
    unsigned timeout;
    int noFork;
    int noColor;
    char *output;
    int testId;
    int subtestId;
    int matchSize;
    char **match;
    const char *selfName;
    int shortPath;
};

enum cut_ReturnCodes {
    cut_NORMAL_EXIT = 0,
    cut_ERROR_EXIT = 254,
    cut_FATAL_EXIT = 255
};

enum cut_Colors {
    cut_NO_COLOR = 0,
    cut_YELLOW_COLOR,
    cut_GREEN_COLOR,
    cut_RED_COLOR
};

#  include "globals.h"
#  include "fragments.h"
#  include "declarations.h"
#  include "messages.h"
#  include "execution.h"


#  if defined(__unix)
#   include "unix.h"
#  elif defined(__APPLE__)
#   include "apple.h"
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


void cut_Register(cut_Instance instance, const char *name, const char *file, size_t line) {
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
    ++cut_unitTests.size;
}



CUT_PRIVATE void cut_ParseArguments(int argc, char **argv) {
    static const char *help = "--help";
    static const char *timeout = "--timeout";
    static const char *noFork = "--no-fork";
    static const char *doFork = "--fork";
    static const char *noColor = "--no-color";
    static const char *output = "--output";
    static const char *subtest = "--subtest";
    static const char *exactTest = "--test";
    static const char *shortPath = "--short-path";
    cut_arguments.help = 0;
    cut_arguments.timeout = CUT_TIMEOUT;
    cut_arguments.noFork = CUT_NO_FORK;
    cut_arguments.noColor = CUT_NO_COLOR;
    cut_arguments.output = NULL;
    cut_arguments.matchSize = 0;
    cut_arguments.match = NULL;
    cut_arguments.testId = -1;
    cut_arguments.subtestId = -1;
    cut_arguments.selfName = argv[0];
    cut_arguments.shortPath = -1;

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--", 2)) {
            ++cut_arguments.matchSize;
            continue;
        }
        if (!strcmp(help, argv[i])) {
            cut_arguments.help = 1;
            continue;
        }
        if (!strcmp(timeout, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%u", &cut_arguments.timeout))
                cut_ErrorExit("option %s requires numeric argument", timeout);
            continue;
        }
        if (!strcmp(noFork, argv[i])) {
            cut_arguments.noFork = 1;
            continue;
        }
        if (!strcmp(doFork, argv[i])) {
            cut_arguments.noFork = 0;
            continue;
        }
        if (!strcmp(noColor, argv[i])) {
            cut_arguments.noColor = 1;
            continue;
        }
        if (!strcmp(output, argv[i])) {
            ++i;
            if (i < argc) {
                cut_arguments.output = argv[i];
                cut_arguments.noColor = 1;
            }
            else {
                cut_ErrorExit("option %s requires string argument", output);
            }
            continue;
        }
        if (!strcmp(exactTest, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%d", &cut_arguments.testId))
                cut_ErrorExit("option %s requires numeric argument %d %d", exactTest, i, argc);
            continue;
        }
        if (!strcmp(subtest, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%d", &cut_arguments.subtestId))
                cut_ErrorExit("option %s requires numeric argument", subtest);
            continue;
        }
        if (!strcmp(shortPath, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%d", &cut_arguments.shortPath))
                cut_ErrorExit("option %s requires numeric argument", shortPath);
            continue;
        }
        cut_ErrorExit("option %s is not recognized", argv[i]);
    }
    if (!cut_arguments.matchSize)
        return;
    cut_arguments.match = (char **)malloc(cut_arguments.matchSize * sizeof(char *));
    if (!cut_arguments.match)
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
        cut_arguments.match[index++] = argv[i];
    }
}

CUT_PRIVATE void cut_CleanInfo(struct cut_Info *info) {
    while (info) {
        struct cut_Info *current = info;
        info = info->next;
        free(current->message);
        free(current->file);
        free(current);
    }
}

CUT_PRIVATE void cut_CleanMemory(struct cut_UnitResult *result) {
    free(result->name);
    free(result->file);
    free(result->statement);
    free(result->exceptionType);
    free(result->exceptionMessage);
    cut_CleanInfo(result->debug);
    cut_CleanInfo(result->check);
}

CUT_PRIVATE int cut_Help() {
    const char *text = ""
    "Run as %s [options] [test names]\n"
    "\n"
    "Options:\n"
    "\t--help            Print out this help.\n"
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

    fprintf(cut_output, text, cut_arguments.selfName);
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
