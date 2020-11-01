#ifndef CUT_MESSAGES_H
#define CUT_MESSAGES_H

#include "declarations.h"

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
    cut_FragmentReceiveStatus status = CUT_FRAGMENT_RECEIVE_STATUS;

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
                cut_FatalExit("cannot allocate memory for reading a message");
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

CUT_PRIVATE void cut_SendOK(int counter) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_OK);
    int *pCounter = (int *)cut_FragmentReserve(&message, sizeof(int), NULL);
    if (!pCounter)
        cut_FatalExit("cannot allocate memory ok:fragment:counter");
    *pCounter = counter;

    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize ok:fragment");
    cut_SendLocalMessage(&message) || cut_FatalExit("cannot send ok:message");
    cut_FragmentClear(&message);
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

    cut_SendLocalMessage(&message) || cut_FatalExit("cannot send debug:message");
    cut_FragmentClear(&message);
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

    cut_SendLocalMessage(&message) || cut_FatalExit("cannot send stop:message");
    cut_FragmentClear(&message);
    longjmp(cut_executionPoint, 1);
}

void cut_Check(const char *text, const char *file, size_t line) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_CHECK);
    size_t *pLine = (size_t*)cut_FragmentReserve(&message, sizeof(size_t), NULL);
    if (!pLine)
        cut_FatalExit("cannot insert check:fragment:line");
    *pLine = line;
    cut_FragmentAddString(&message, file) || cut_FatalExit("cannot insert check:fragment:file");
    cut_FragmentAddString(&message, text) || cut_FatalExit("cannot insert check:fragment:text");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize check:fragment");

    cut_SendLocalMessage(&message) || cut_FatalExit("cannot send check:message");
    cut_FragmentClear(&message);
}

CUT_PRIVATE void cut_StopException(const char *type, const char *text) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_EXCEPTION);
    cut_FragmentAddString(&message, type) || cut_FatalExit("cannot insert exception:fragment:type");
    cut_FragmentAddString(&message, text) || cut_FatalExit("cannot insert exception:fragment:text");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize exception:fragment");

    cut_SendLocalMessage(&message) || cut_FatalExit("cannot send exception:message");
    cut_FragmentClear(&message);
}

CUT_PRIVATE void cut_Timeouted() {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_TIMEOUT);
    cut_FragmentSignalSafeSerialize(&message) || cut_FatalExit("cannot serialize timeout:fragment");
    cut_SendLocalMessage(&message) || cut_FatalExit("cannot send timeout:message");
}

void cut_Subtest(int number, const char *name) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_SUBTEST);
    int * pNumber = (int *)cut_FragmentReserve(&message, sizeof(int), NULL);
    if (!pNumber)
        cut_FatalExit("cannot insert subtest:fragment:number");
    *pNumber = number;
    cut_FragmentAddString(&message, name) || cut_FatalExit("cannot insert subtest:fragment:name");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize subtest:fragment");

    cut_SendLocalMessage(&message) || cut_FatalExit("cannot send subtest:message");
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

    cut_FragmentReceiveStatus status = CUT_FRAGMENT_RECEIVE_STATUS;
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
                cut_FatalExit("cannot allocate memory for reading a message");
        }
        memcpy(message->serialized + processed, cut_localMessageCursor, (size_t)toRead);
        cut_localMessageCursor += toRead;
        r = toRead;
    }
    return toRead != -1;
}

CUT_PRIVATE void *cut_PipeReader(int pipeRead, struct cut_UnitResult *result) {
    int repeat;
    do {
        repeat = 0;
        struct cut_Fragment message;
        cut_FragmentInit(&message, cut_NO_TYPE);
        cut_ReadLocalMessage(pipeRead, &message) || cut_FatalExit("cannot read message");
        cut_FragmentDeserialize(&message) || cut_FatalExit("cannot deserialize message");

        switch (message.id) {
        case cut_MESSAGE_SUBTEST:
            message.sliceCount == 2 || cut_FatalExit("invalid debug:message format");
            cut_SetSubtestName(
                result,
                *(int *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL)
            ) || cut_FatalExit("cannot set subtest name");
            repeat = 1;
            break;
        case cut_MESSAGE_DEBUG:
            message.sliceCount == 3 || cut_FatalExit("invalid debug:message format");
            cut_AddInfo(
                &result->debug,
                *(size_t *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL),
                cut_FragmentGet(&message, 2, NULL)
            ) || cut_FatalExit("cannot add debug");
            repeat = 1;
            break;
        case cut_MESSAGE_OK:
            message.sliceCount == 1 || cut_FatalExit("invalid ok:message format");
            result->subtests = *(int *)cut_FragmentGet(&message, 0, NULL);
            if (result->status == cut_RESULT_UNKNOWN)
                result->status = cut_RESULT_OK;
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
        case cut_MESSAGE_TIMEOUT:
            result->status = cut_RESULT_TIMED_OUT;
            break;
        case cut_MESSAGE_CHECK:
            message.sliceCount == 3 || cut_FatalExit("invalid check:message format");
            cut_AddInfo(
                &result->check,
                *(size_t *)cut_FragmentGet(&message, 0, NULL),
                cut_FragmentGet(&message, 1, NULL),
                cut_FragmentGet(&message, 2, NULL)
            ) || cut_FatalExit("cannot add check");
            result->status = cut_RESULT_FAILED;
            repeat = 1;
            break;
        }
        cut_FragmentClear(&message);
    } while (repeat);
    return NULL;
}

CUT_PRIVATE int cut_SetSubtestName(struct cut_UnitResult *result, int number, const char *name) {
    result->name = (char *)malloc(strlen(name) + 1);
    if (!result->name)
        return 0;
    result->number = number;
    strcpy(result->name, name);
    return 1;
}

CUT_PRIVATE int cut_AddInfo(struct cut_Info **info,
                             size_t line, const char *file, const char *text) {
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

CUT_PRIVATE int cut_SetFailResult(struct cut_UnitResult *result,
                                  size_t line, const char *file, const char *text) {
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