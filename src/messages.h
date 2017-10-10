#ifndef CUT_MESSAGES_H
#define CUT_MESSAGES_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif

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

CUT_PRIVATE int cut_StopException(const char *type, const char *text) {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_EXCEPTION);
    cut_FragmentAddString(&message, type) || cut_FatalExit("cannot insert exception:fragment:type");
    cut_FragmentAddString(&message, text) || cut_FatalExit("cannot insert exception:fragment:text");
    cut_FragmentSerialize(&message) || cut_FatalExit("cannot serialize exception:fragment");

    cut_SendMessage(&message) || cut_FatalExit("cannot send exception:message");
    cut_FragmentClean(&message);
}

CUT_PRIVATE void cut_Timeouted() {
    struct cut_Fragment message;
    cut_FragmentInit(&message, cut_MESSAGE_TIMEOUT);
    cut_FragmentSignalSafeSerialize(&message) || cut_FatalExit("cannot serialize timeout:fragment");
    cut_SendMessage(&message) || cut_FatalExit("cannot send timeout:message");
}

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
        case cut_MESSAGE_TIMEOUT:
            result->timeouted = 1;
            result->failed = 1;
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

#endif // CUT_MESSAGES_H