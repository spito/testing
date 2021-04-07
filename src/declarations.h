#ifndef CUT_DECLARATIONS_H
#define CUT_DECLARATIONS_H

#include <stdint.h>
#include <stdio.h>

#include "definitions.h"
#include "os-specific.h"
#include "public-declarations.h"

CUT_NS_BEGIN

struct cut_Info {
    char *message;
    char *file;
    unsigned line;
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

enum cut_ResultStatus {
    cut_RESULT_UNKNOWN,
    cut_RESULT_OK,
    cut_RESULT_FILTERED_OUT,
    cut_RESULT_SUPPRESSED,
    cut_RESULT_FAILED,
    cut_RESULT_RETURNED_NON_ZERO,
    cut_RESULT_SIGNALLED,
    cut_RESULT_TIMED_OUT,
    cut_RESULT_SKIPPED
};

enum cut_SkipReason {
    cut_SKIP_REASON_NO_SKIP = 0,
    cut_SKIP_REASON_FILTERED_OUT = cut_RESULT_FILTERED_OUT,
    cut_SKIP_REASON_SUPPRESSED = cut_RESULT_SUPPRESSED,
    cut_SKIP_REASON_FAILED = cut_RESULT_SKIPPED
};

struct cut_UnitResult {
    char *name;
    int number;
    int subtests;
    enum cut_ResultStatus status;
    char *file;
    unsigned line;
    char *statement;
    char *exceptionType;
    char *exceptionMessage;
    int returnCode;
    int signal;
    struct cut_Info *debug;
    struct cut_Info *check;
};

struct cut_UnitTest {
    cut_Instance instance;
    const char *name;
    const char *file;
    unsigned line;
    struct cut_Settings *settings;
    enum cut_SkipReason skipReason;
};

struct cut_UnitTestArray {
    int size;
    int capacity;
    struct cut_UnitTest *tests;
};

struct cut_Arguments {
    int help;
    int list;
    unsigned timeout;
    int timeoutDefined;
    int noFork;
    int noColor;
    char *output;
    int testId;
    int subtestId;
    int matchSize;
    char **match;
    const char *selfName;
    int shortPath;
    const char *format;
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

struct cut_Queue {
    size_t size;
    struct cut_QueueItem *first;
    struct cut_QueueItem *last;
    struct cut_QueueItem *trash;
};

struct cut_QueueItem {
    int testId;
    int *refCount;
    struct cut_QueueItem *next;
    struct cut_Queue depending;
};

struct cut_Shepherd {
    void *data;
    FILE *output;
    int executed;
    int succeeded;
    int suppressed;
    int failed;
    int filteredOut;
    int skipped;

    double points;
    double maxPoints;

    int pipeWrite;
    int pipeRead;

    const struct cut_Arguments *arguments;
    struct cut_Queue *queuedTests;
    void (*unitRunner)(struct cut_Shepherd *, int, int, struct cut_UnitResult *);
    void (*listTests)(const struct cut_Shepherd *);
    void (*startTest)(struct cut_Shepherd *, int testId);
    void (*startSubTests)(struct cut_Shepherd *, int testId, int subtests);
    void (*startSubTest)(struct cut_Shepherd *, int testId, int subtest);
    void (*endSubTest)(struct cut_Shepherd *, int testId, int subtest, const struct cut_UnitResult *);
    void (*endTest)(struct cut_Shepherd *, int testId, const struct cut_UnitResult *);
    void (*finalize)(struct cut_Shepherd *);
    void (*clear)(struct cut_Shepherd *);
};

struct cut_EnqueuePair {
    int *appliedNeeds;
    struct cut_QueueItem *self;
    int retry;
};


// core:public

typedef void(*cut_Instance)(int *, int);
void cut_Register(cut_Instance instance, const char *name, const char *file, unsigned line, struct cut_Settings *settings);
int cut_File(FILE *file, const char *content);
CUT_NORETURN void cut_Stop(const char *text, const char *file, unsigned line);
void cut_Check(const char *text, const char *file, unsigned line);
int cut_Input(const char *content);
void cut_Subtest(int number, const char *name);
void cut_DebugMessage(const char *file, unsigned line, const char *fmt, ...);

// core:private

CUT_NORETURN int cut_FatalExit(const char *reason);
CUT_NORETURN int cut_ErrorExit(const char *reason, ...);

CUT_PRIVATE void cut_ClearInfo(struct cut_Info *info);
CUT_PRIVATE void cut_ClearUnitResult(struct cut_UnitResult *result);

CUT_PRIVATE int cut_Help(const struct cut_Arguments *arguments);
CUT_PRIVATE int cut_List(const struct cut_Shepherd *shepherd);

// arguments

CUT_PRIVATE void cut_ParseArguments(struct cut_Arguments *arguments, int argc, char **argv);

// execution

CUT_PRIVATE void cut_ExceptionBypass(int testId, int subtest);
CUT_PRIVATE void cut_RunUnitForkless(struct cut_Shepherd *shepherd, int testId, int subtest,
                                     struct cut_UnitResult *result);
CUT_PRIVATE void cut_RunUnitTest(struct cut_Shepherd *shepherd, struct cut_UnitResult *result, int testId);
CUT_PRIVATE void cut_RunUnitTests(struct cut_Shepherd *shepherd);
CUT_PRIVATE void cut_RunUnitSubTest(struct cut_Shepherd *shepherd, int testId, int subtest);
CUT_PRIVATE void cut_RunUnitSubTests(struct cut_Shepherd *shepherd, int testId, int subtests);

CUT_PRIVATE int cut_FilterOutUnit(const struct cut_Arguments *arguments, int testId);
CUT_PRIVATE void cut_EnqueueTests(struct cut_Shepherd *shepherd);
CUT_PRIVATE int cut_TestComparator(const void *_lhs, const void *_rhs);
CUT_PRIVATE void cut_InitShepherd(struct cut_Shepherd *shepherd, const struct cut_Arguments *arguments,
                                  struct cut_Queue *queue);
CUT_PRIVATE void cut_ClearShepherd(struct cut_Shepherd *shepherd);

CUT_PRIVATE int cut_Runner(int argc, char **argv);

// messages

CUT_PRIVATE void cut_SendOK(int counter);
void cut_DebugMessage(const char *file, unsigned line, const char *fmt, ...);
void cut_Stop(const char *text, const char *file, unsigned line);
void cut_Check(const char *text, const char *file, unsigned line);
#ifdef __cplusplus
CUT_PRIVATE void cut_StopException(const char *type, const char *text);
#endif
CUT_PRIVATE void cut_Timedout();
void cut_Subtest(int number, const char *name);
CUT_PRIVATE void *cut_PipeReader(int pipeRead, struct cut_UnitResult *result);
CUT_PRIVATE int cut_SetSubtestName(struct cut_UnitResult *result, int number, const char *name);
CUT_PRIVATE int cut_AddInfo(struct cut_Info **info, unsigned line, const char *file, const char *text);
CUT_PRIVATE int cut_SetFailResult(struct cut_UnitResult *result, unsigned line, const char *file, const char *text);
CUT_PRIVATE int cut_SetExceptionResult(struct cut_UnitResult *result, const char *type, const char *text);

// output

CUT_PRIVATE const char *cut_GetStatus(const struct cut_UnitResult *result, enum cut_Colors *color);
CUT_PRIVATE const char *cut_ShortPath(const struct cut_Arguments *arguments, const char *path);
CUT_PRIVATE const char *cut_Signal(int signal);
CUT_PRIVATE const char *cut_ReturnCode(int returnCode);
CUT_PRIVATE void cut_ShepherdNoop(struct cut_Shepherd *shepherd, ...);

// queue

CUT_PRIVATE void cut_InitQueue(struct cut_Queue *queue);
CUT_PRIVATE struct cut_QueueItem *cut_QueuePushTest(struct cut_Queue *queue, int testId);
CUT_PRIVATE void cut_ClearQueueItems(struct cut_QueueItem *current);
CUT_PRIVATE void cut_ClearQueue(struct cut_Queue *queue);
CUT_PRIVATE struct cut_QueueItem *cut_QueuePopTest(struct cut_Queue *queue);
CUT_PRIVATE void cut_QueueMeltTest(struct cut_Queue *queue, struct cut_QueueItem *toMelt);
CUT_PRIVATE int cut_QueueRePushTest(struct cut_Queue *queue, struct cut_QueueItem *toRePush);
CUT_PRIVATE struct cut_QueueItem *cut_QueueAddTest(struct cut_Queue *queue, struct cut_QueueItem *toAdd);

// platform specific functions
CUT_PRIVATE int cut_IsTerminalOutput();
CUT_PRIVATE int cut_IsDebugger();

CUT_PRIVATE void cut_RedirectIO();
CUT_PRIVATE void cut_ResumeIO();

CUT_PRIVATE int64_t cut_Read(int fd, char *destination, size_t bytes);
CUT_PRIVATE int64_t cut_Write(int fd, const char *source, size_t bytes);

CUT_PRIVATE int cut_PreRun(const struct cut_Arguments *arguments);
CUT_PRIVATE void cut_RunUnit(struct cut_Shepherd *shepherd, int testId, int subtest,
                             struct cut_UnitResult *result);

int cut_File(FILE *file, const char *content);
CUT_PRIVATE int cut_PrintColorized(FILE *output, enum cut_Colors color, const char *text);

CUT_NS_END

#endif // CUT_DECLARATIONS_H
