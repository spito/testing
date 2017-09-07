#ifndef CUT_CORE_H
#define CUT_CORE_H

#ifndef CUT

# define ASSERT(e) (void)0
# define ASSERT_FILE(f, content) (void)0
# define TEST(name) static void unitTest_ ## name()
# define SUBTEST(name) if (0)
# define DEBUG_MSG(...) (void)0

#else

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

# define _POSIX_C_SOURCE 1
# include <stdio.h>
# include <string.h>

# define ASSERT(e) do { if (!(e)) {                                             \
        cut_Stop(#e, __FILE__, __LINE__);                                       \
    } } while( 0 )

# define ASSERT_FILE(f, content) do {                                           \
    if (!cut_File(f, content)) {                                                \
        cut_Stop("content of file is not equal", __FILE__, __LINE__);           \
    } } while( 0 )

# define TEST(name)                                                             \
    void cut_instance_ ## name(int, int, int *);                                \
    CUT_CONSTRUCTOR(cut_Register ## name) {                                     \
        cut_Register(cut_instance_ ## name, #name);                             \
    }                                                                           \
    void cut_instance_ ## name(int cut_subtest, int cut_current, int *cut_next)

# define SUBTEST(name)                                                          \
    if (++cut_subtest == cut_current + 1)                                       \
        *cut_next = cut_subtest;                                                \
    if (cut_subtest == cut_current)                                             \
        cut_Subtest(name);                                                      \
    if (cut_subtest == cut_current)

# define DEBUG_MSG(...) cut_DebugMessage(__FILE__, __LINE__, __VA_ARGS__)

# ifdef __cplusplus
extern "C" {
# endif

typedef void(*cut_Instance)(int, int, int *);
void cut_Register(cut_Instance instance, const char *name);
int cut_File(FILE *file, const char *content);
CUT_NORETURN void test_Stop(const char *message, const char *file, size_t line);
void cut_Subtest(const char *name);
void cut_DebugMessage(const char *file, size_t line, const char *fmt, ...);

# if defined(CUT_MAIN)

enum cut_MessageStatus {
    cut_OK,
    cut_OK_SUBTEST,
    cut_FAIL,
    cut_INTERNAL
};

struct cut_UnitMessage {
    char *name;
    enum cut_MessageStatus status;
    char *file;
    int line;
    char *statement;
    char *exceptionType;
    char *exceptionMessage;
};

struct cut_Debug {
    char *message;
    char *file;
    int line;
    struct cut_Debug *next;
};

enum cut_MessageType {
    cut_NO_TYPE = 0,
    cut_DEBUG,
    cut_RESULT
};

struct cut_Message {
    enum cut_MessageType type;
    union {
        struct cut_UnitMessage *message;
        struct cut_Debug *debug;
    } data;
};

struct cut_UnitResult {
    struct cut_UnitMessage message;
    int returnCode;
    int signal;
    struct cut_UnitResult *nextSubtest;
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
    char *output;
    char *testName;
    char *subtest;
    int matchSize;
    char **match;
};

CUT_PRIVATE struct cut_UnitTestArray cut_unitTests = {0, 0, NULL};

#  if defined(__unix)
#   include "unix.h"
#  elif defined(__APPLE__)
#   include "apple.h"
#  elif defined(_WIN32)
#   include "windows.h"
#  else
#   error "unsupported platform"
#  endif

CUT_PRIVATE char *cut_SerializeResult(const struct cut_UnitResult *result) {}

CUT_PRIVATE int cut_DeserializeResult(const char *buffer, struct cut_UnitResult *result) {}

CUT_PRIVATE char *cut_SerializeDebug(const struct cut_Debug *debug) {}

CUT_PRIVATE int cut_DeserializeDebug(const char *buffer, struct cut_Debug *debug) {}


CUT_PRIVATE void cut_ParseArguments(struct cut_Arguments *arguments, int argc, char **argv) {
    static const char *timeout = "--timeout";
    static const char *noFork = "--no-fork";
    static const char *output = "--output";
    static const char *subtest = "--subtest";
    static const char *exactTest = "--test";
    arguments->timeout = 3;
    arguments->noFork = 0;
    arguments->output = NULL;
    arguments->testName = NULL;
    arguments->matchSize = 0;
    arguments->match = NULL;
    arguments->subtest = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--", 2)) {
            ++arguments->matchSize;
            continue;
        }
        if (!strcmp(timeout, argv[i])) {
            ++i;
            if (i < argc) {
                arguments->timeout = atoi(argv[i]);
            }
            continue;
        }
        if (!strcmp(noFork, argv[i])) {
            arguments->noFork = 1;
            continue;
        }
        if (!strcmp(output, argv[i])) {
            ++i;
            if (i < argc) {
                arguments->output = argv[i];
            }
            continue;
        }
        if (!strcmp(exactTest, argv[i])) {
            ++i;
            if (i < argc) {
                arguments->testName = argv[i];
            }
            continue;
        }
        if (!strcmp(subtest, argv[i])) {
            ++i;
            if (i < argc {
                arguments->subtest = argv[i];
            }
            continue;
        }
    }
    if (!arguments->matchSize)
        return;
    arguments->match = malloc(arguments->matchSize * sizeof(char *));
    if (!arguments->match) {
        cut_InternalError(ENOMEM);
    }
    int index = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strncmp(argv[i], "--")) {
            if (!strcmp(timeout, argv[i]) || !strcmp(output, argv[i])
             || !strcmp(subtest, argv[i]) || !strcmp(exactTest, argv[i]))
            {
                ++i;
            }
            continue;
        }
        arguments->match[index++] = argv[i];
    }
}

CUT_PRIVATE int cut_SkipUnit(const struct cut_Arguments *arguments, const char *name) {
    if (arguments->testName)
        return !strcmp(arguments->testName, name);
    if (!arguments->matchSize)
        return 0;
    for (int i = 0; i < arguments->matchSize; ++i) {
        if (strstr(name,arguments->match[i]))
            return 0;
    }
    return 1;
}

CUT_PRIVATE void cut_PrintFail(FILE *output, const char *indent, struct cut_UnitResult *result) {
    if (result->message.status == cut_OK || result->message.status == cut_OK_SUBTEST)
        return;
    if (result->returnCode || result->signal) {
        fprintf(output, "%stest exited abnormally:\n", indent);
        if (result->signal)
            fprintf(output, "%s  signal code: %i\n", indent, result->signal);
        else
            fprintf(output, "%s  return code: %i\n", indent, result->returnCode);
    } else if (result->message.statement) {
        fprintf(output, "%sassert '%s' on %s:%i\n", indent, result->message.statement, result->message.file, result->message.line);
    } else if (result->message.exceptionType) {
        fprintf(output, "%suncatched exception of type %s\n", result->message.exceptionType);
        if (result->message.exceptionMessage)
            fprintf(output, "%s  \"%s\"", indent, result->message.exceptionMessage);
    } else {
        fprintf(output, "%ssome internal error\n", indent);
    }
}

CUT_PRIVATE void cut_PrintDebugMessages(FILE *output, const char *indent, struct cut_UnitResult *result) {
    if (!result->debug)
        return;
    fprintf(output, "%sdebug messages:\n", indent);
    for (const struct cut_Debug *current = result->debug; current; current = current->next) {
        fprintf(output, "%s  %s (%s:%d)\n", indent, current->message, current->file, current->line);
    }
}

CUT_PRIVATE const char *cut_GetStatus(struct cut_UnitResult *result) {
    switch (result->message.status) {
    case cut_OK:
        return "OK";
    case cut_OK_SUBTEST:
        return "SUBTEST OK";
    case cut_FAIL:
        return "FAIL";
    case cut_INTERNAL:
        return "INTERNAL FAIL";
    default:
        return "UNKNOWN";
    }
}

CUT_PRIVATE void cut_PrintResult(FILE *output, int base, struct cut_UnitResult *result) {
    static const char *indent = "  ";
    static const char *subIndent = "    ";
    const char *status = cut_GetStatus(result);
    int lastPosition = 80 - 1 - base - strlen(msg);
    for (int i = 0; i < lastPosition; ++i) {
        putc('.', output);
    }
    fprintf(output, "%s\n", status);

    if (result->nextSubtest) {
        putc('\n', output);
        struct cut_UnitResult *current = result->nextSubtest;
        for (; current; current = current->nextSubtest) {
            int base = fprintf(output, "%s[%s]", indent, current->message.name);
            const char *status = cut_GetStatus(current);
            int lastPosition = 80 - 1 - base - strlen(status);
            for (int i = 0; i < lastPosition; ++i)
                putc('.', output);
            fprintf(output, "%s\n", status);
            cut_PrintFail(output, subIndent, result);
            cut_PrintDebugMessages(output, subIndent, result->debug);
        }
    }
    cut_PrintFail(output, indent, result);
    cut_PrintDebugMessages(output, indent, result->debug);
    fflush(output);
}

CUT_PRIVATE void cut_CleanMemory(struct cut_UnitResult *result) {

}

CUT_PRIVATE int cut_Runner(int argc, char **argv) {
    struct cut_Arguments arguments;
    cut_ParseArguments(&arguments, argc, argv);

    cut_PreRun(&arguments);

    FILE *output = stdout;
    if (arguments.output) {
        output = fopen(arguments.output, "w");
        if (!output)
            cut_InternalError(EBADF);
    }

    int failed = 0;
    int executed = 0;
    for (int i = 0; i < cut_unitTests.size; ++i) {
        if (cut_SkipUnit(&arguments, cut_unitTests.tests[i].name))
            continue;
        ++executed;
        int base = fprintf(output, "[%3i] %s", executed, cut_unitTests.tests[i].name);
        fflush(output);
        struct cut_UnitResult result;
        memset(&result, 0, sizeof(result));
        cut_RunUnit(i, arguments.timeout, arguments.noFork, &result);
        cut_PrintResult(output, base, &result);
        cut_CleanMemory(&result);
    }
    fprintf(output,
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
    free(arguments.match);
    if (arguments.output)
        fclose(output);
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
