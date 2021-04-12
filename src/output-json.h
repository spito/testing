#ifndef CUT_OUTPUT_JSON_H
#define CUT_OUTPUT_JSON_H

#include "declarations.h"

CUT_NS_BEGIN

struct cut_TestAttributes_json {
    int subtests;
    int subtestsFailed;
};

struct cut_OutputData_json {
    struct cut_TestAttributes_json *attributes;
    int testNumber;
    char *buffer;
    size_t bufferSize;
};

CUT_PRIVATE const char *cut_SanitizeStr_json(struct cut_OutputData_json *data, const char *str) {
    size_t quots = 0;
    size_t size = 0;
    const char *c = str;
    for (; *c; ++c) {
        if (*c == '"')
            ++quots;
    }
    size = c - str + 1;

    if (data->bufferSize < size + quots) {
        data->buffer = (char *) realloc(data->buffer, size + quots);
        data->bufferSize = size + quots;
        if (!data->buffer)
            CUT_DIE("cannot allocate memory for json");
    }

    char *buffer = data->buffer;
    do {
        if (*str == '"' || *str == '\\') {
            *buffer = '\\';
            ++buffer;
        }
    } while (*buffer++ = *str++);
    return data->buffer;
}

CUT_PRIVATE void cut_PrintInfo_json(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    enum cut_Colors color;
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    fprintf(shepherd->output, ", \"status\": \"%s\"", cut_GetStatus(test->currentResult, &color));
    if (test->currentResult->check) {
        fprintf(shepherd->output, ", \"checks\": [");
        int item = 0;
        for (const struct cut_Info *current = test->currentResult->check; current; current = current->next) {
            if (item++)
                fputc(',', shepherd->output);
            fprintf(shepherd->output, "\"%s (%s:%d)\"", cut_SanitizeStr_json(data, current->message),
                    current->file, current->line);
        }
        fprintf(shepherd->output, "]");
    }
    switch (test->currentResult->status) {
    case cut_RESULT_TIMED_OUT:
        fprintf(shepherd->output, ", \"timeout\": %d", test->setup->timeout);
        break;
    case cut_RESULT_SIGNALLED:
        fprintf(shepherd->output, ", \"signal\": \"%s\"", cut_Signal(test->currentResult->signal));
        break;
    case cut_RESULT_RETURNED_NON_ZERO:
        fprintf(shepherd->output, ", \"return\": \"%s\"", cut_ReturnCode(test->currentResult->returnCode));
        break;
    }
    if (test->currentResult->statement && test->currentResult->file && test->currentResult->line) {
        fprintf(shepherd->output, ", \"assert\": \"%s (%s:%d)\"", cut_SanitizeStr_json(data, test->currentResult->statement),
                test->currentResult->file, test->currentResult->line);
    }
    if (test->currentResult->exceptionType && test->currentResult->exceptionMessage) {
        fprintf(shepherd->output, ", \"exception\": \"%s: %s\"", cut_SanitizeStr_json(data, test->currentResult->exceptionType),
                test->currentResult->exceptionMessage);
    }
    if (test->currentResult->debug) {
        fprintf(shepherd->output, ", \"debug\": [");
        int item = 0;
        for (const struct cut_Info *current = test->currentResult->debug; current; current = current->next) {
            if (item++)
                fputc(',', shepherd->output);
            fprintf(shepherd->output, "\"%s (%s:%d)\"", cut_SanitizeStr_json(data, current->message),
                    current->file, current->line);
        }
        fprintf(shepherd->output, "]");
    }
}

CUT_PRIVATE void cut_StartTest_json(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    fputc(data->testNumber++ ? ',' : '[', shepherd->output);
    fprintf(shepherd->output, "{\"name\": \"%s\"", test->setup->name);
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_StartSubTests_json(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    data->attributes[test->id].subtests = test->resultSize - 1;

    fprintf(shepherd->output, ", \"subtests\": [");
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_EndSubTest_json(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    // struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    // if (test->currentResult->status != cut_RESULT_OK)
    //     ++data->attributes[test->id].subtestsFailed;

    // enum cut_Colors color;
    // const char *status = cut_GetStatus(test->currentResult, &color);
    // if (1 < subtest)
    //     fputc(',', shepherd->output);
    // fprintf(shepherd->output, "{\"name\": \"%s\", \"iteration\": %d",
    //         cut_SanitizeStr_json(data, result->name),
    //         result->id);
    // cut_PrintInfo_json(shepherd, testId, result);
    // fputc('}', shepherd->output);
    // fflush(shepherd->output);
}

CUT_PRIVATE void cut_EndTest_json(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    // struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    // struct cut_UnitResult dummy;

    // if (data->attributes[testId].subtests) {
    //     memset(&dummy, 0, sizeof(dummy));
    //     dummy.status = data->attributes[testId].subtestsFailed ? cut_RESULT_FAILED : cut_RESULT_OK;
    //     result = &dummy;
    //     fprintf(shepherd->output, "]");
    // }
    // cut_PrintInfo_json(shepherd, testId, result);
    // fprintf(shepherd->output, ", \"points\":%0.2f}", result->status == cut_RESULT_OK
    //         ? cut_unitTests.tests[testId].setup->points
    //         : 0.0);
    // fflush(shepherd->output);
}

CUT_PRIVATE void cut_Finalize_json(struct cut_Shepherd *shepherd) {
    fprintf(shepherd->output, "]");
    fflush(shepherd->output);
}

CUT_PRIVATE void cut_Clear_json(struct cut_Shepherd *shepherd) {
    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    free(data->buffer);
    free(data->attributes);
    free(data);
}

CUT_PRIVATE void cut_InitOutput_json(struct cut_Shepherd *shepherd) {

    shepherd->data = malloc(sizeof(struct cut_OutputData_json));
    if (!shepherd->data)
        CUT_DIE("cannot allocate memory for output");

    struct cut_OutputData_json *data = (struct cut_OutputData_json *)shepherd->data;
    
    data->attributes = (struct cut_TestAttributes_json *) malloc(sizeof(struct cut_TestAttributes_json) * cut_unitTests.size);
    if (!data->attributes)
        CUT_DIE("cannot allocate memory for output");
    memset(data->attributes, 0, sizeof(struct cut_TestAttributes_json) * cut_unitTests.size);
    data->testNumber = 0;
    data->buffer = NULL;
    data->bufferSize = 0;

    shepherd->startTest = cut_StartTest_json;
    shepherd->startSubTests = cut_StartSubTests_json;
    shepherd->endSubTest = cut_EndSubTest_json;
    shepherd->endTest = cut_EndTest_json;
    shepherd->finalize = cut_Finalize_json;
    shepherd->clear = cut_Clear_json;
}

CUT_NS_END

#endif
