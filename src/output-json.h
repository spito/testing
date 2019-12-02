#ifndef CUT_OUTPUT_JSON_H
#define CUT_OUTPUT_JSON_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif

struct cut_TestAttributes_json {
    int subtests;
    int subtestsFailed;
};

struct cut_OutputData_json {
    struct cut_TestAttributes_json *attributes;
};

CUT_PRIVATE void cut_PrintInfo_json(struct cut_Shepherd *shepherd, const struct cut_UnitResult *result) {
    enum cut_Colors color;
    fprintf(shepherd->output, ", \"status\": \"%s\"", cut_GetStatus(result, &color));
    if (result->check) {
        fprintf(shepherd->output, ", \"checks\": [");
        int item = 0;
        for (const struct cut_Info *current = result->check; current; current = current->next) {
            if (item++)
                fputc(',', shepherd->output);
            fprintf(shepherd->output, "\"%s (%s:%d)\"", current->message,
                    current->file, current->line);
        }
        fprintf(shepherd->output, "]");
    }
    switch (result->status) {
    case cut_RESULT_TIMED_OUT:
        fprintf(shepherd->output, ", \"timeout\": %d", shepherd->arguments->timeout);
        break;
    case cut_RESULT_SIGNALLED:
        fprintf(shepherd->output, ", \"signal\": \"%s\"", cut_Signal(result->signal));
        break;
    case cut_RESULT_RETURNED_NON_ZERO:
        fprintf(shepherd->output, ", \"return\": \"%s\"", cut_ReturnCode(result->returnCode));
        break;
    }
    if (result->statement && result->file && result->line) {
        fprintf(shepherd->output, ", \"assert\": \"%s (%s:%d)\"", result->statement, result->file, result->line);
    }
    if (result->exceptionType && result->exceptionMessage) {
        fprintf(shepherd->output, ", \"exception\": \"%s: %s\"", result->exceptionType, result->exceptionMessage);
    }
    if (result->debug) {
        fprintf(shepherd->output, ", \"debug\": [");
        int item = 0;
        for (const struct cut_Info *current = result->debug; current; current = current->next) {
            if (item++)
                fputc(',', shepherd->output);
            fprintf(shepherd->output, "\"%s (%s:%d)\"", current->message, current->file, current->line);
        }
        fprintf(shepherd->output, "]");
    }
}

CUT_PRIVATE void cut_StartTest_json(struct cut_Shepherd *shepherd, int testId) {
    fputc(testId ? ',' : '[', shepherd->output);
    fprintf(shepherd->output, "{\"name\": \"%s\"", cut_unitTests.tests[testId].name);
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_StartSubTests_json(struct cut_Shepherd *shepherd, int testId, int subtests) {
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    data->attributes[testId].subtests = subtests;

    fprintf(shepherd->output, ", \"subtests\": [");
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_EndSubTest_json(struct cut_Shepherd *shepherd, int testId, int subtest, const struct cut_UnitResult *result) {
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    if (result->status != cut_RESULT_OK)
        ++data->attributes[testId].subtestsFailed;

    enum cut_Colors color;
    const char *status = cut_GetStatus(result, &color);
    if (1 < subtest)
        fputc(',', shepherd->output);
    fprintf(shepherd->output, "{\"name\": \"%s\", \"iteration\": %d", result->name, result->number);
    cut_PrintInfo_json(shepherd, result);
    fputc('}', shepherd->output);
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_EndTest_json(struct cut_Shepherd *shepherd, int testId, const struct cut_UnitResult *result) {
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    struct cut_UnitResult dummy;

    if (data->attributes[testId].subtests) {
        memset(&dummy, 0, sizeof(dummy));
        dummy.status = data->attributes[testId].subtestsFailed ? cut_RESULT_FAILED : cut_RESULT_OK;
        result = &dummy;
        fprintf(shepherd->output, "]");
    }
    cut_PrintInfo_json(shepherd, result);
    fprintf(shepherd->output, "}");
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_Finalize_json(struct cut_Shepherd *shepherd) {
    fprintf(shepherd->output, "]");
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_Clear_json(struct cut_Shepherd *shepherd) {
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    free(data->attributes);
    free(data);
}


CUT_PRIVATE void cut_InitOutput_json(struct cut_Shepherd *shepherd) {

    shepherd->data = malloc(sizeof(struct cut_OutputData_json));
    if (!shepherd->data)
        cut_FatalExit("cannot allocate memory for output");

    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    
    data->attributes = (struct cut_TestAttributes_json *) malloc(sizeof(struct cut_TestAttributes_json) * cut_unitTests.size);
    if (!data->attributes)
        cut_FatalExit("cannot allocate memory for output");
    memset(data->attributes, 0, sizeof(struct cut_TestAttributes_json) * cut_unitTests.size);

    shepherd->startTest = cut_StartTest_json;
    shepherd->startSubTests = cut_StartSubTests_json;
    shepherd->endSubTest = cut_EndSubTest_json;
    shepherd->endTest = cut_EndTest_json;
    shepherd->finalize = cut_Finalize_json;
    shepherd->clear = cut_Clear_json;
}

#endif
