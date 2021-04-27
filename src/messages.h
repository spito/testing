#ifndef CUT_MESSAGES_H
#define CUT_MESSAGES_H

#include "declarations.h"
#include "fragments.h"

CUT_NS_BEGIN

CUT_PRIVATE int cut_SendMessage(const struct cut_Fragment *message) {
    size_t remaining = message->serializedLength;
    size_t position = 0;

    int64_t r;
    while (remaining && (r = cut_Write(cut_pipeWrite, message->serialized + position, remaining)) > 0) {
        position += (size_t)r;
        remaining -= (size_t)r;
    }
    return r != -1;
}

CUT_PRIVATE int cut_ReadMessage(int pipeRead, struct cut_Fragment *message) {
    cut_FragmentReceiveStatus status;
    memset(&status, 0, sizeof(status));

    message->serialized = NULL;
    message->serializedLength = 0;
    int64_t r = 0;
    int64_t toRead = 0;
    size_t processed = 0;
    while ((toRead = cut_FragmentReceiveContinue(&status, message->serialized, r)) > 0) {
        processed = cut_FragmentReceiveProcessed(&status);

        if (message->serializedLength < processed + toRead) {
            message->serializedLength = (uint32_t)(processed + toRead);
            message->serialized = (char *)realloc(message->serialized, message->serializedLength);
            if (!message->serialized)
                CUT_DIE("cannot allocate memory for reading a message");
        }
        r = cut_Read(pipeRead, message->serialized + processed, (size_t)toRead);
    }
    processed = cut_FragmentReceiveProcessed(&status);
    if (processed < message->serializedLength) {
        memset(message->serialized, 0, message->serializedLength);
    }
    return toRead != -1;
}

CUT_PRIVATE void cut_ResetLocalMessage() {
    cut_localMessageCursor = NULL;
    cut_localMessageSize = 0;
}

CUT_PRIVATE int cut_SendLocalMessage(struct cut_Fragment *message) {
    if (!cut_noFork)
        return cut_SendMessage(message);
    if (message->serializedLength + cut_localMessageSize > CUT_MAX_LOCAL_MESSAGE_LENGTH)
        return 0;
    memcpy(cut_localMessage + cut_localMessageSize, message->serialized, message->serializedLength);
    cut_localMessageSize += message->serializedLength;
    return 1;
}

CUT_PRIVATE void cut_SendOK() {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_OK);
    cut_FragmentSerialize(&message) || CUT_DIE("cannot serialize ok:fragment");
    cut_SendLocalMessage(&message) || CUT_DIE("cannot send ok:message");
    cut_FragmentClear(&message);
}

void cut_CommonMessage(int id, const char *text, const char *file, unsigned line) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, id);
    unsigned *pLine = (unsigned*)cut_FragmentReserve(&message, sizeof(unsigned), NULL);
    if (!pLine)
        CUT_DIE("cannot insert fragment:line");
    *pLine = line;
    cut_FragmentAddString(&message, file) || CUT_DIE("cannot insert fragment:file");
    cut_FragmentAddString(&message, text) || CUT_DIE("cannot insert fragment:text");
    cut_FragmentSerialize(&message) || CUT_DIE("cannot serialize fragment");

    cut_SendLocalMessage(&message) || CUT_DIE("cannot send message");
    cut_FragmentClear(&message);
}

void cut_FormatMessage(cut_Reporter reporter, const char *file, unsigned line, const char *fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);
    size_t length = 1 + vsnprintf(NULL, 0, fmt, args1);
    char *buffer;
    (buffer = (char *)malloc(length)) || CUT_DIE("cannot allocate buffer");
    va_end(args1);
    vsnprintf(buffer, length, fmt, args2);
    va_end(args2);

    reporter(buffer, file, line);
    free(buffer);
}

void cut_Stop(const char *text, const char *file, unsigned line) {
    cut_CommonMessage(cut_MESSAGE_FAIL, text, file, line);
    longjmp(cut_executionPoint, 1);
}

void cut_Check(const char *text, const char *file, unsigned line) {
    cut_CommonMessage(cut_MESSAGE_CHECK, text, file, line);
}

void cut_Debug(const char *text, const char *file, unsigned line) {
    cut_CommonMessage(cut_MESSAGE_DEBUG, text, file, line);
}

CUT_PRIVATE void cut_StopException(const char *type, const char *text) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_EXCEPTION);
    cut_FragmentAddString(&message, type) || CUT_DIE("cannot insert exception:fragment:type");
    cut_FragmentAddString(&message, text) || CUT_DIE("cannot insert exception:fragment:text");
    cut_FragmentSerialize(&message) || CUT_DIE("cannot serialize exception:fragment");

    cut_SendLocalMessage(&message) || CUT_DIE("cannot send exception:message");
    cut_FragmentClear(&message);
}

CUT_PRIVATE void cut_Timedout() {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_TIMEOUT);
    cut_FragmentSignalSafeSerialize(&message) || CUT_DIE("cannot serialize timeout:fragment");
    cut_SendLocalMessage(&message) || CUT_DIE("cannot send timeout:message");
}

void cut_RegisterSubtest(int number, const char *name) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_SUBTEST);
    int * pNumber = (int *)cut_FragmentReserve(&message, sizeof(int), NULL);
    if (!pNumber)
        CUT_DIE("cannot insert subtest:fragment:number");
    *pNumber = number;
    cut_FragmentAddString(&message, name) || CUT_DIE("cannot insert subtest:fragment:name");
    cut_FragmentSerialize(&message) || CUT_DIE("cannot serialize subtest:fragment");

    cut_SendLocalMessage(&message) || CUT_DIE("cannot send subtest:message");
    cut_FragmentClear(&message);
}

CUT_PRIVATE int cut_ReadLocalMessage(int pipeRead, struct cut_Fragment *message) {
    if (!cut_localMessageSize)
        return cut_ReadMessage(pipeRead, message);
    // first read
    if (!cut_localMessageCursor)
        cut_localMessageCursor = cut_localMessage;

    // nothing to read
    if (cut_localMessageCursor >= cut_localMessage + cut_localMessageSize)
        return 0;

    cut_FragmentReceiveStatus status;
    memset(&status, 0, sizeof(status));
    message->serialized = NULL;
    message->serializedLength = 0;

    int64_t r = 0;
    int64_t toRead = 0;
    while ((toRead = cut_FragmentReceiveContinue(&status, message->serialized, r)) > 0) {
        size_t processed = cut_FragmentReceiveProcessed(&status);

        if (message->serializedLength < processed + toRead) {
            message->serializedLength = (uint32_t)(processed + toRead);
            message->serialized = (char *)realloc(message->serialized, message->serializedLength);
            if (!message->serialized)
                CUT_DIE("cannot allocate memory for reading a message");
        }
        memcpy(message->serialized + processed, cut_localMessageCursor, (size_t)toRead);
        cut_localMessageCursor += toRead;
        r = toRead;
    }
    return toRead != -1;
}

CUT_PRIVATE void *cut_PipeReader(int pipeRead, struct cut_UnitTest *test) {
    int repeat;
    do {
        repeat = 0;
        struct cut_Fragment message;
        cut_FragmentInit(&message, cut_NO_TYPE);
        cut_ReadLocalMessage(pipeRead, &message) || CUT_DIE("cannot read message");
        cut_FragmentDeserialize(&message) || CUT_DIE("cannot deserialize message");

        switch (message.id) {
        case cut_MESSAGE_SUBTEST:
            message.sliceCount == 2 || CUT_DIE("invalid debug:message format");
            cut_PrepareSubtests(
                test,
                *(int *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL)
            ) || CUT_DIE("cannot set subtest name");
            repeat = 1;
            break;

        case cut_MESSAGE_DEBUG:
            message.sliceCount == 3 || CUT_DIE("invalid debug:message format");
            cut_AddInfo(
                &test->currentResult->debug,
                *(unsigned *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL),
                cut_FragmentGet(&message, 2, NULL)
            ) || CUT_DIE("cannot add debug");
            repeat = 1;
            break;

        case cut_MESSAGE_OK:
            message.sliceCount == 0 || CUT_DIE("invalid ok:message format");
            cut_SetTestOk(test->currentResult);
            break;

        case cut_MESSAGE_FAIL:
            message.sliceCount == 3 || CUT_DIE("invalid fail:message format");
            cut_SetFailResult(
                test->currentResult,
                *(unsigned *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL),
                cut_FragmentGet(&message, 2, NULL)
            ) || CUT_DIE("cannot set fail result");
            break;

        case cut_MESSAGE_EXCEPTION:
            message.sliceCount == 2 || CUT_DIE("invalid exception:message format");
            cut_SetExceptionResult(
                test->currentResult,
                cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL)
            ) || CUT_DIE("cannot set exception result");
            break;

        case cut_MESSAGE_TIMEOUT:
            test->currentResult->status = cut_RESULT_TIMED_OUT;
            break;

        case cut_MESSAGE_CHECK:
            message.sliceCount == 3 || CUT_DIE("invalid check:message format");
            cut_SetCheckResult(
                test->currentResult,
                *(unsigned *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL),
                cut_FragmentGet(&message, 2, NULL)
            ) || CUT_DIE("cannot add check");
            repeat = 1;
            break;
        }
        cut_FragmentClear(&message);
    } while (repeat);
    return NULL;
}

CUT_PRIVATE int cut_PrepareSubtests(struct cut_UnitTest *test, int number, const char *name) {
    struct cut_UnitResult *results = (struct cut_UnitResult *) realloc(test->results, (number + 1) * sizeof(struct cut_UnitResult));
    if (!results)
        return 0;

    long offset = test->currentResult - test->results;
    test->results = results;
    test->currentResult = test->results + offset;


    for (int i = test->resultSize; i <= number; ++i) {
        memset(&results[i], 0, sizeof(results[i]));
        results[i].id = i;
        results[i].name = (char *)malloc(strlen(name) + 1);
        if (!results[i].name)
            return 0;
        strcpy(results[i].name, name);
    }
    test->resultSize = number + 1;
    return 1;
}

CUT_PRIVATE int cut_AddInfo(struct cut_Info **info,
                             unsigned line, const char *file, const char *text) {
    struct cut_Info *item = (struct cut_Info *)malloc(sizeof(struct cut_Info));
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
    while (*info) {
        info = &(*info)->next;
    }
    *info = item;
    return 1;
}

CUT_PRIVATE int cut_SetTestOk(struct cut_UnitResult *result) {
    if (result->status == cut_RESULT_UNKNOWN)
        result->status = cut_RESULT_OK;
    return 1;
}

CUT_PRIVATE int cut_SetCheckResult(struct cut_UnitResult *result,
                                   unsigned line, const char *file, const char *text) {
    result->status = cut_RESULT_FAILED;
    return cut_AddInfo(&result->check, line, file, text);
}

CUT_PRIVATE int cut_SetFailResult(struct cut_UnitResult *result,
                                  unsigned line, const char *file, const char *text) {
    result->file = (char *)malloc(strlen(file) + 1);
    if (!result->file)
        return 0;
    result->statement = (char *)malloc(strlen(text) + 1);
    if (!result->statement) {
        free(result->file);
        return 0;
    }
    result->line = line;
    strcpy(result->file, file);
    strcpy(result->statement, text);
    result->status = cut_RESULT_FAILED;
    return 1;
}

CUT_PRIVATE int cut_SetExceptionResult(struct cut_UnitResult *result,
                                       const char *type, const char *text) {
    result->exceptionType = (char *)malloc(strlen(type) + 1);
    if (!result->exceptionType)
        return 0;
    result->exceptionMessage = (char *)malloc(strlen(text) + 1);
    if (!result->exceptionMessage) {
        free(result->exceptionType);
        return 0;
    }
    strcpy(result->exceptionType, type);
    strcpy(result->exceptionMessage, text);
    result->status = cut_RESULT_FAILED;
    return 1;
}

CUT_NS_END

#endif // CUT_MESSAGES_H