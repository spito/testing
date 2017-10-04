#ifndef CUT_CORE_H
#define CUT_CORE_H

#ifndef CUT

# define ASSERT(e) (void)0
# define ASSERT_FILE(f, content) (void)0
# define TEST(name) static void unitTest_ ## name()
# define SUBTEST(name) if (0)
# define DEBUG_MSG(...) (void)0

#else

#pragma GCC system_header

# define CUT_PRIVATE static

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
# include <stdlib.h>
# include <string.h>
# include <stdarg.h>
# include <setjmp.h>

# define ASSERT(e) do { if (!(e)) {                                             \
        cut_Stop(#e, __FILE__, __LINE__);                                       \
    } } while(0)

# define ASSERT_FILE(f, content) do {                                           \
    if (!cut_File(f, content)) {                                                \
        cut_Stop("content of file is not equal", __FILE__, __LINE__);           \
    } } while(0)

# define TEST(name)                                                             \
    void cut_instance_ ## name(int *, int);                                     \
    CUT_CONSTRUCTOR(cut_Register ## name) {                                     \
        cut_Register(cut_instance_ ## name, #name);                             \
    }                                                                           \
    void cut_instance_ ## name(CUT_UNUSED(int *cut_subtest), CUT_UNUSED(int cut_current))

# define SUBTEST(name)                                                          \
    if (++*cut_subtest == cut_current)                                          \
        cut_Subtest(#name);                                                     \
    if (*cut_subtest == cut_current)

# define DEBUG_MSG(...) cut_DebugMessage(__FILE__, __LINE__, __VA_ARGS__)

# ifdef __cplusplus
extern "C" {
# endif

typedef void(*cut_Instance)(int *, int);
void cut_Register(cut_Instance instance, const char *name);
int cut_File(FILE *file, const char *content);
CUT_NORETURN void cut_Stop(const char *text, const char *file, size_t line);
void cut_Subtest(const char *name);
void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...);

# if defined(CUT_MAIN)

#include <stdarg.h>


struct cut_Debug {
    char *message;
    char *file;
    int line;
    struct cut_Debug *next;
};

enum cut_MessageType {
    cut_NO_TYPE = 0,
    cut_MESSAGE_SUBTEST,
    cut_MESSAGE_DEBUG,
    cut_MESSAGE_OK,
    cut_MESSAGE_FAIL,
    cut_MESSAGE_EXCEPTION
};

struct cut_UnitResult {
    char *name;
    int subtests;
    int failed;
    char *file;
    int line;
    char *statement;
    char *exceptionType;
    char *exceptionMessage;
    int returnCode;
    int signal;
    struct cut_Debug *debug;
};

struct cut_UnitTest {
    cut_Instance instance;
    const char *name;
};

struct cut_UnitTestArray {
    int size;
    int capacity;
    struct cut_UnitTest *tests;
};

struct cut_Arguments {
    int timeout;
    int noFork;
    int noColor;
    char *output;
    char *testName;
    char *subtest;
    int matchSize;
    char **match;
};

enum cut_ReturnCodes {
    cut_OK_EXIT = 0,
    cut_ERROR_EXIT = 254,
    cut_FATAL_EXIT = 255
};

CUT_PRIVATE struct cut_Arguments cut_arguments;
CUT_PRIVATE struct cut_UnitTestArray cut_unitTests = {0, 0, NULL};
CUT_PRIVATE FILE *cut_output = NULL;
CUT_PRIVATE int cut_spawnChild = 0;
CUT_PRIVATE int cut_pipeWrite = 0;
CUT_PRIVATE int cut_pipeRead = 0;
CUT_PRIVATE FILE *cut_stdout = NULL;
CUT_PRIVATE FILE *cut_stderr = NULL;
CUT_PRIVATE jmp_buf cut_executionPoint;
CUT_PRIVATE const char *cut_emergencyLog = "cut.log";

#  include "fragments.h"

// common functions
CUT_NORETURN int cut_FatalExit(const char *reason);
CUT_NORETURN int cut_ErrorExit(const char *reason, ...);
void cut_Register(cut_Instance instance, const char *name);
CUT_PRIVATE void cut_SendOK(int counter);
void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...);
void cut_Stop(const char *text, const char *file, size_t line);
#  ifdef __cplusplus
CUT_PRIVATE int cut_StopException(const char *type, const char *text);
#  endif
CUT_PRIVATE void cut_ExceptionBypass(int testId, int subtest);
void cut_Subtest(const char *name);
CUT_PRIVATE void *cut_PipeReader(struct cut_UnitResult *result);
CUT_PRIVATE int cut_SetSubtestName(struct cut_UnitResult *result, const char *file);
CUT_PRIVATE int cut_AddDebug(struct cut_UnitResult *result,
    size_t line, const char *file, const char *text);
CUT_PRIVATE int cut_SetFailResult(struct cut_UnitResult *result,
    size_t line, const char *file, const char *text);
CUT_PRIVATE int cut_SetExceptionResult(struct cut_UnitResult *result,
    const char *type, const char *text);
CUT_PRIVATE void cut_ParseArguments(int argc, char **argv);
CUT_PRIVATE int cut_SkipUnit(const char *name);
CUT_PRIVATE const char *cut_GetStatus(const struct cut_UnitResult *result, int *length);
CUT_PRIVATE void cut_PrintResult(int base, int subtest, const struct cut_UnitResult *result);
CUT_PRIVATE void cut_CleanMemory(struct cut_UnitResult *result);
CUT_PRIVATE int cut_Runner(int argc, char **argv);

// platform specific functions
CUT_PRIVATE int cut_SendMessage(const struct cut_Fragment *message);
CUT_PRIVATE int cut_ReadMessage(struct cut_Fragment *message);
CUT_PRIVATE void cut_PreRun();
CUT_PRIVATE void cut_RunUnit(int testId, int subtest, struct cut_UnitResult *result);
int cut_File(FILE *file, const char *content);

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
    if (cut_spawnChild) {
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


void cut_Register(cut_Instance instance, const char *name) {
    if (cut_unitTests.size == cut_unitTests.capacity) {
        cut_unitTests.capacity += 16;
        cut_unitTests.tests = (struct cut_UnitTest *)realloc(cut_unitTests.tests,
            sizeof(struct cut_UnitTest) * cut_unitTests.capacity);
        if (!cut_unitTests.tests)
            cut_FatalExit("cannot allocate memory for unit tests");
    }
    cut_unitTests.tests[cut_unitTests.size].instance = instance;
    cut_unitTests.tests[cut_unitTests.size].name = name;
    ++cut_unitTests.size;
}

CUT_PRIVATE void cut_SendOK(int counter) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_OK);
    int *pCounter = (int *)cut_FragmentReserve(&message, sizeof(int), NULL);
    if (!pCounter)
        cut_FatalExit("cannot allocate memory ok:fragment:counter");
    *pCounter = counter;
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize ok:fragment");
    cut_SendMessage(&message) || cut_FatalExit("cannot send ok:message");
    cut_FragmentClean(&message);
}

void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    size_t length = 1 + vsnprintf(NULL, 0, fmt, args1);
    char *buffer;
    (buffer = (char *)malloc(length)) || cut_FatalExit("cannot allocate buffer");
    va_end(args1);
    vsnprintf(buffer, length, fmt, args2);
    va_end(args2);

    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_DEBUG);
    size_t *pLine = (size_t *)cut_FragmentReserve(&message, sizeof(size_t), NULL);
    if (!pLine)
        cut_FatalExit("cannot insert debug:fragment:line");
    *pLine = line;
    cut_FragmentAddString(&message, file) || cut_FatalExit("cannot insert debug:fragment:file");
    cut_FragmentAddString(&message, buffer) || cut_FatalExit("cannot insert debug:fragment:buffer");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize debug:fragment");

    cut_SendMessage(&message) || cut_FatalExit("cannot send debug:message");

    cut_FragmentClean(&message);
    free(buffer);
}

void cut_Stop(const char *text, const char *file, size_t line) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_FAIL);
    size_t *pLine = (size_t*)cut_FragmentReserve(&message, sizeof(size_t), NULL);
    if (!pLine)
        cut_FatalExit("cannot insert stop:fragment:line");
    *pLine = line;
    cut_FragmentAddString(&message, file) || cut_FatalExit("cannot insert stop:fragment:file");
    cut_FragmentAddString(&message, text) || cut_FatalExit("cannot insert stop:fragment:text");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize stop:fragment");

    cut_SendMessage(&message) || cut_FatalExit("cannot send stop:message");
    cut_FragmentClean(&message);
    longjmp(cut_executionPoint, 1);
}

# ifdef __cplusplus

CUT_PRIVATE int cut_StopException(const char *type, const char *text) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_EXCEPTION);
    cut_FragmentAddString(&message, type) || cut_FatalExit("cannot insert exception:fragment:type");
    cut_FragmentAddString(&message, text) || cut_FatalExit("cannot insert exception:fragment:text");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize exception:fragment");

    cut_SendMessage(&message) || cut_FatalExit("cannot send exception:message");
    cut_FragmentClean(&message);
}

} // extern C

#  include <stdexcept>
#  include <typeinfo>
#  include <string>

CUT_PRIVATE void cut_ExceptionBypass(int testId, int subtest) {
    if (setjmp(cut_executionPoint))
        return;
    try {
        int counter = 0;
        cut_unitTests.tests[testId].instance(&counter, subtest);
        cut_SendOK(counter);
    } catch (const std::exception &e) {
        std::string name = typeid(e).name();
        cut_StopException(name.c_str(), e.what());
    } catch (...) {
        cut_StopException("unknown type", "(empty message)");
    }
}

extern "C" {
# else
CUT_PRIVATE void cut_ExceptionBypass(int testId, int subtest) {
    if (setjmp(cut_executionPoint))
        return;
    int counter = 0;
    cut_unitTests.tests[testId].instance(&counter, subtest);
    cut_SendOK(counter);
}
# endif

void cut_Subtest(const char *name) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_SUBTEST);
    cut_FragmentAddString(&message, name) || cut_FatalExit("cannot insert subtest:fragment:name");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize subtest:fragment");

    cut_SendMessage(&message) || cut_FatalExit("cannot send subtest:message");
    cut_FragmentClean(&message);
}

CUT_PRIVATE void *cut_PipeReader(struct cut_UnitResult *result) {
    int repeat;
    do {
        repeat = 0;
        struct cut_Fragment message;
        cut_FragmentInit(&message, 0);
        cut_ReadMessage(&message) || cut_FatalExit("cannot read message");
        cut_FragmentDeserialize(&message) || cut_FatalExit("cannot deserialize message");

        switch (message.id) {
        case cut_MESSAGE_SUBTEST:
            message.sliceCount == 1 || cut_FatalExit("invalid debug:message format");
            cut_SetSubtestName(
                result,
                cut_FragmentGet(&message, 0, NULL)
            ) || cut_FatalExit("cannot set subtest name");
            repeat = 1;
            break;
        case cut_MESSAGE_DEBUG:
            message.sliceCount == 3 || cut_FatalExit("invalid debug:message format");
            cut_AddDebug(
                result,
                *(size_t *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL),
                cut_FragmentGet(&message, 2, NULL)
            ) || cut_FatalExit("cannot add debug");
            repeat = 1;
            break;
        case cut_MESSAGE_OK:
            message.sliceCount == 1 || cut_FatalExit("invalid ok:message format");
            result->subtests = *(int *)cut_FragmentGet(&message, 0, NULL);
            break;
        case cut_MESSAGE_FAIL:
            message.sliceCount == 3 || cut_FatalExit("invalid fail:message format");
            cut_SetFailResult(
                result,
                *(size_t *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL),
                cut_FragmentGet(&message, 2, NULL)
            ) || cut_FatalExit("cannot set fail result");
            break;
        case cut_MESSAGE_EXCEPTION:
            message.sliceCount == 2 || cut_FatalExit("invalid exception:message format");
            cut_SetExceptionResult(
                result,
                cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL)
            ) || cut_FatalExit("cannot set exception result");
            break;
        }
        cut_FragmentClean(&message);
    } while (repeat);
    return NULL;
}

CUT_PRIVATE int cut_SetSubtestName(struct cut_UnitResult *result, const char *name) {
    result->name = (char *)malloc(strlen(name));
    if (!result->name)
        return 0;
    strcpy(result->name, name);
    return 1;
}

CUT_PRIVATE int cut_AddDebug(struct cut_UnitResult *result,
                             size_t line, const char *file, const char *text) {
    struct cut_Debug *item = (struct cut_Debug *)malloc(sizeof(struct cut_Debug));
    if (!item)
        return 0;
    item->file = (char *)malloc(strlen(file) + 1);
    if (!item->file) {
        free(item);
        return 0;
    }
    item->message = (char *)malloc(strlen(text) + 1);
    if (!item->message) {
        free(item->file);
        free(item);
        return 0;
    }
    strcpy(item->file, file);
    strcpy(item->message, text);
    item->line = line;
    item->next = NULL;
    struct cut_Debug **ptr = &result->debug;
    while (*ptr) {
        ptr = &(*ptr)->next;
    }
    *ptr = item;
    return 1;
}

CUT_PRIVATE int cut_SetFailResult(struct cut_UnitResult *result,
                                  size_t line, const char *file, const char *text) {
    result->file = (char *)malloc(strlen(file));
    if (!result->file)
        return 0;
    result->statement = (char *)malloc(strlen(text));
    if (!result->statement) {
        free(result->file);
        return 0;
    }
    result->line = line;
    strcpy(result->file, file);
    strcpy(result->statement, text);
    result->failed = 1;
    return 1;
}

CUT_PRIVATE int cut_SetExceptionResult(struct cut_UnitResult *result,
                                       const char *type, const char *text) {
    result->exceptionType = (char *)malloc(strlen(type));
    if (!result->exceptionType)
        return 0;
    result->exceptionMessage = (char *)malloc(strlen(text));
    if (!result->exceptionMessage) {
        free(result->exceptionType);
        return 0;
    }
    strcpy(result->exceptionType, type);
    strcpy(result->exceptionMessage, text);
    result->failed = 1;
    return 1;
}

CUT_PRIVATE void cut_ParseArguments(int argc, char **argv) {
    static const char *timeout = "--timeout";
    static const char *noFork = "--no-fork";
    static const char *noColor = "--no-color";
    static const char *output = "--output";
    static const char *subtest = "--subtest";
    static const char *exactTest = "--test";
    cut_arguments.timeout = 3;
    cut_arguments.noFork = 0;
    cut_arguments.noColor = 0;
    cut_arguments.output = NULL;
    cut_arguments.testName = NULL;
    cut_arguments.matchSize = 0;
    cut_arguments.match = NULL;
    cut_arguments.subtest = NULL;
    enum { bufferLength = 512 };
    char buffer[bufferLength] = {0,};
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--", 2)) {
            ++cut_arguments.matchSize;
            continue;
        }
        if (!strcmp(timeout, argv[i])) {
            ++i;
            if (i >= argc || !sscanf(argv[i], "%d", &cut_arguments.timeout))
                cut_ErrorExit("option %s requires numeric argument", timeout);
            continue;
        }
        if (!strcmp(noFork, argv[i])) {
            cut_arguments.noFork = 1;
            continue;
        }
        if (!strcmp(noColor, argv[i])) {
            cut_arguments.noColor = 1;
            continue;
        }
        if (!strcmp(output, argv[i])) {
            ++i;
            if (i < argc)
                cut_arguments.output = argv[i];
            else
                cut_ErrorExit("option %s requires string argument", output);
            continue;
        }
        if (!strcmp(exactTest, argv[i])) {
            ++i;
            if (i < argc)
                cut_arguments.testName = argv[i];
            else
                cut_ErrorExit("option %s requires string argument", exactTest);
            continue;
        }
        if (!strcmp(subtest, argv[i])) {
            ++i;
            if (i < argc )
                cut_arguments.subtest = argv[i];
            else
                cut_ErrorExit("option %s requires string argument", subtest);
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
             || !strcmp(subtest, argv[i]) || !strcmp(exactTest, argv[i]))
            {
                ++i;
            }
            continue;
        }
        cut_arguments.match[index++] = argv[i];
    }
}

CUT_PRIVATE int cut_SkipUnit(const char *name) {
    if (cut_arguments.testName)
        return !strcmp(cut_arguments.testName, name);
    if (!cut_arguments.matchSize)
        return 0;
    for (int i = 0; i < cut_arguments.matchSize; ++i) {
        if (strstr(name, cut_arguments.match[i]))
            return 0;
    }
    return 1;
}

CUT_PRIVATE const char *cut_GetStatus(const struct cut_UnitResult *result, int *length) {
    static const char *ok = "\e[1;32mOK\e[0m";
    static const char *basicOk = "OK";
    static const char *fail = "\e[1;31mFAIL\e[0m";
    static const char *basicFail = "FAIL";
    static const char *internalFail = "\e[1;33mINTERNAL ERROR\e[0m";
    static const char *basicInternalFail = "INTERNAL ERROR";

    if (result->returnCode == cut_FATAL_EXIT) {
        *length = strlen(basicInternalFail);
        return cut_arguments.noColor ? basicInternalFail : internalFail;
    }
    if (result->failed) {
        *length = strlen(basicFail);
        return cut_arguments.noColor ? basicFail : fail;
    }
    *length = strlen(basicOk);
    return cut_arguments.noColor ? basicOk : ok;
}

CUT_PRIVATE void cut_PrintResult(int base, int subtest, const struct cut_UnitResult *result) {
    static const char *shortIndent = "    ";
    static const char *longIndent = "        ";
    int statusLength;
    const char *status = cut_GetStatus(result, &statusLength);
    int lastPosition = 80 - 1 - statusLength;
    int extended = 0;

    const char *indent = shortIndent;
    if (result->name && subtest) {
        if (subtest == 1)
            putc('\n', cut_output);
        lastPosition -= fprintf(cut_output, "%s%s", indent, result->name);
        indent = longIndent;
    } else {
        lastPosition -= base;
    }
    if (!subtest)
        extended = 1;

    for (int i = 0; i < lastPosition; ++i) {
        putc('.', cut_output);
    }
    fprintf(cut_output, "%s\n", status);
    if (result->failed) {
        if (result->signal == SIGALRM)
            fprintf(cut_output, "%stimeouted (%d s)\n", indent, cut_arguments.timeout);
        else if (result->signal)
            fprintf(cut_output, "%ssignal code: %d\n", indent, result->signal);
        if (result->returnCode)
            fprintf(cut_output, "%sreturn code: %d\n", indent, result->returnCode);
        if (result->statement && result->file && result->line)
            fprintf(cut_output, "%sassert '%s' (%s:%d)\n", indent,
                    result->statement, result->file, result->line);
        if (result->exceptionType && result->exceptionMessage)
            fprintf(cut_output, "%sexception %s: %s\n", indent,
                    result->exceptionType, result->exceptionMessage);
        extended = 1;
    }
    if (result->debug) {
        fprintf(cut_output, "%sdebug messages:\n", indent);
        extended = 1;
    }
    for (const struct cut_Debug *current = result->debug; current; current = current->next) {
        fprintf(cut_output, "%s  %s (%s:%d)\n", indent, current->message, current->file, current->line);
    }
    if (extended)
        fprintf(cut_output, "\n");
    fflush(cut_output);
}

CUT_PRIVATE void cut_CleanMemory(struct cut_UnitResult *result) {
    free(result->name);
    free(result->file);
    free(result->statement);
    free(result->exceptionType);
    free(result->exceptionMessage);
    while (result->debug) {
        struct cut_Debug *current = result->debug;
        result->debug = result->debug->next;
        free(current->message);
        free(current->file);
        free(current);
    }
}

CUT_PRIVATE int cut_Runner(int argc, char **argv) {
    cut_output = stdout;
    cut_ParseArguments(argc, argv);

    cut_PreRun();

    if (cut_arguments.output) {
        cut_output = fopen(cut_arguments.output, "w");
        if (!cut_output)
            cut_ErrorExit("cannot open file %s for writing", cut_arguments.output);
    }

    int failed = 0;
    int executed = 0;
    for (int i = 0; i < cut_unitTests.size; ++i) {
        if (cut_SkipUnit(cut_unitTests.tests[i].name))
            continue;
        ++executed;
        int base = fprintf(cut_output, "[%3i] %s", executed, cut_unitTests.tests[i].name);
        fflush(cut_output);
        int subtests = 1;
        int subtestFailure = 0;
        for (int subtest = 1; subtest <= subtests; ++subtest) {
            struct cut_UnitResult result;
            memset(&result, 0, sizeof(result));
            cut_RunUnit(i, subtest, &result);
            if (result.failed)
                ++subtestFailure;
            FILE *emergencyLog = fopen(cut_emergencyLog, "r");
            if (emergencyLog) {
                char buffer[512] = {0,};
                fread(buffer, 512, 1, emergencyLog);
                if (*buffer) {
                    result.statement = (char *)malloc(strlen(buffer) + 1);
                    strcpy(result.statement, buffer);
                }
                fclose(emergencyLog);
                remove(cut_emergencyLog);
            }
            cut_PrintResult(base, subtest, &result);
            cut_CleanMemory(&result);
            if (result.subtests > subtests)
                subtests = result.subtests;
        }
        if (subtests > 1) {
            base = fprintf(cut_output, "[%3i] %s (overall)", executed, cut_unitTests.tests[i].name);
            struct cut_UnitResult result;
            memset(&result, 0, sizeof(result));
            result.failed = subtestFailure;
            cut_PrintResult(base, 0, &result);
        }
        if (subtestFailure)
            ++failed;
    }
    fprintf(cut_output,
            "\nSummary:\n"
            "  tests:     %3i\n"
            "  succeeded: %3i\n"
            "  skipped:   %3i\n"
            "  failed:    %3i\n",
            cut_unitTests.size,
            executed - failed,
            cut_unitTests.size - executed,
            failed);
    free(cut_unitTests.tests);
    free(cut_arguments.match);
    if (cut_arguments.output)
        fclose(cut_output);
    return failed;
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
