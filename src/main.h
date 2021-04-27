#ifndef CUT_MAIN_H
#define CUT_MAIN_H

#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "declarations.h"
#include "execution.h"
#include "file-operations.h"
#include "fragments.h"
#include "globals.h"
#include "messages.h"
#include "output.h"
#include "queue.h"

#if defined(__linux__)
# include "linux.h"
#elif defined(__APPLE__)
# include "apple.h"
#elif defined(__unix)
# include "unix.h"
#elif defined(_WIN32)
# include "windows.h"
#else
# error "unsupported platform"
#endif

CUT_NS_BEGIN

const char *cut_needs[1] = {""};

CUT_NORETURN int cut_FatalExit(const char *reason, const char *file, unsigned line) {
    FILE *log = fopen(cut_emergencyLog, "a");
    if (!log)
        exit(cut_PANIC);

    time_t now = time(NULL);
    struct tm *timeInfo = localtime(&now);

    char when[80];
    strftime(when, sizeof(when), "%F %T", timeInfo);
    fprintf(log, "%s | %s (%u) | %s\n", when, file, line, reason);

    fclose(log);
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

void cut_Register(struct cut_Setup *setup) {
    if (cut_unitTests.size == cut_unitTests.capacity) {
        cut_unitTests.capacity += 16;
        cut_unitTests.tests = (struct cut_UnitTest *)realloc(cut_unitTests.tests,
            sizeof(struct cut_UnitTest) * cut_unitTests.capacity);
        if (!cut_unitTests.tests)
            CUT_DIE("cannot allocate memory for unit tests");
    }
    struct cut_UnitTest *test = &cut_unitTests.tests[cut_unitTests.size];
    test->id = 0;
    test->setup = setup;
    test->results = (struct cut_UnitResult *) malloc(sizeof(struct cut_UnitResult));
    if (!test->results)
        CUT_DIE("cannot allocate memory for unit tests");

    test->resultSize = 1;
    test->currentResult = test->results;
    memset(test->currentResult, 0, sizeof(*test->currentResult));

    ++cut_unitTests.size;
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
    "\t--list            Print list of all tests.\n"
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

CUT_PRIVATE int cut_List() {
    fprintf(stderr, "Tests:\n");
    for (int i = 0; i < cut_unitTests.size; ++i) {
        struct cut_UnitTest *test = &cut_unitTests.tests[i];
        fprintf(stderr, "\t%s\n", test->setup->name);
    }
    return 0;
}

int main(int argc, char **argv) {

    struct cut_Arguments arguments;

    cut_ParseArguments(&arguments, argc, argv);

    if (arguments.help)
        return cut_Help(&arguments);
    if (arguments.list)
        return cut_List();

    return cut_Runner(&arguments);
}

CUT_NS_END

#endif