#ifndef CUT_OUTPUT_STD_H
#define CUT_OUTPUT_STD_H

#include "declarations.h"

CUT_NS_BEGIN

struct cut_TestAttributes_std {
    int subtests;
    int subtestsFailed;
    int base;
};

struct cut_OutputData_std {
    struct cut_TestAttributes_std *attributes;
};


CUT_PRIVATE void cut_StartTest_std(struct cut_Shepherd *shepherd, int testId) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;

    ++shepherd->executed;

    int base = fprintf(shepherd->output, "[%3i] %s", shepherd->executed, cut_unitTests.tests[testId].setup->name);
    fflush(shepherd->output);
    data->attributes[testId].base = base;
    data->attributes[testId].subtests = 0;
    data->attributes[testId].subtestsFailed = 0;
}

CUT_PRIVATE void cut_StartSubTests_std(struct cut_Shepherd *shepherd, int testId, int subtests) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    data->attributes[testId].subtests = subtests;

    fprintf(shepherd->output, ": %d subtests\n", subtests);
    fflush(shepherd->output);
}


CUT_PRIVATE int cut_PrintDetailResult(struct cut_Shepherd *shepherd, const char *indent, int testId, const struct cut_UnitResult *result) {
    int extended = 0;
    for (const struct cut_Info *current = result->check; current; current = current->next) {
        fprintf(shepherd->output, "%scheck '%s' (%s:%d)\n", indent, current->message,
                cut_ShortPath(shepherd->arguments, current->file), current->line);
        extended = 1;
    }
    switch (result->status) {
    case cut_RESULT_TIMED_OUT:
        fprintf(shepherd->output, "%stimed out (%d s)\n", indent, cut_unitTests.tests[testId].setup->timeout);
        extended = 1;
        break;
    case cut_RESULT_SIGNALLED:
        fprintf(shepherd->output, "%ssignal: %s\n", indent, cut_Signal(result->signal));
        extended = 1;
        break;
    case cut_RESULT_RETURNED_NON_ZERO:
        fprintf(shepherd->output, "%sreturn code: %s\n", indent, cut_ReturnCode(result->returnCode));
        extended = 1;
        break;
    }
    if (result->statement && result->file && result->line) {
        fprintf(shepherd->output, "%sassert '%s' (%s:%d)\n", indent,
                result->statement, cut_ShortPath(shepherd->arguments, result->file), result->line);
        extended = 1;
    }
    if (result->exceptionType && result->exceptionMessage) {
        fprintf(shepherd->output, "%sexception %s: %s\n", indent,
                result->exceptionType, result->exceptionMessage);
        extended = 1;
    }
    if (result->debug) {
        fprintf(shepherd->output, "%sdebug messages:\n", indent);
        for (const struct cut_Info *current = result->debug; current; current = current->next) {
            fprintf(shepherd->output, "%s  %s (%s:%d)\n", indent, current->message,
                    cut_ShortPath(shepherd->arguments, current->file), current->line);
        }
        extended = 1;
    }
    return extended;
}

CUT_PRIVATE void cut_EndSubTest_std(struct cut_Shepherd *shepherd, int testId, int subtest, const struct cut_UnitResult *result) {
    static const char *shortIndent = "    ";
    static const char *longIndent = "        ";
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    enum cut_Colors color;
    const char *status = cut_GetStatus(result, &color);
    int lastPosition = 80 - 1 - strlen(status);
    if (result->status != cut_RESULT_OK)
        ++data->attributes[testId].subtestsFailed;

    const char *indent = shortIndent;
    if (result->number <= 1)
        lastPosition -= fprintf(shepherd->output, "%s%s", indent, result->name);
    else {
        lastPosition -= fprintf(shepherd->output, "%s", indent);
        int length = strlen(result->name);
        for (int i = 0; i < length; ++i)
            putc(' ', shepherd->output);
        lastPosition -= length;
    }
    if (result->number)
        lastPosition -= fprintf(shepherd->output, " #%d", result->number);
    indent = longIndent;

    for (int i = 0; i < lastPosition; ++i) {
        putc('.', shepherd->output);
    }
    if (shepherd->arguments->noColor)
        fprintf(shepherd->output, status);
    else
        cut_PrintColorized(shepherd->output, color, status);

    putc('\n', shepherd->output);
    if (cut_PrintDetailResult(shepherd, indent, testId, result))
        putc('\n', shepherd->output);
    fflush(shepherd->output);


}

CUT_PRIVATE void cut_EndSingleTest_std(struct cut_Shepherd *shepherd, int testId, const struct cut_UnitResult *result) {
    static const char *indent = "    ";
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    enum cut_Colors color;
    const char *status = cut_GetStatus(result, &color);
    int lastPosition = 80 - 1 - strlen(status) - data->attributes[testId].base;
    int extended = 0;

    for (int i = 0; i < lastPosition; ++i) {
        putc('.', shepherd->output);
    }
    if (shepherd->arguments->noColor)
        fprintf(shepherd->output, status);
    else
        cut_PrintColorized(shepherd->output, color, status);

    putc('\n', shepherd->output);
    extended = cut_PrintDetailResult(shepherd, indent, testId, result);
    if (extended)
        putc('\n', shepherd->output);
    fflush(shepherd->output);

    shepherd->maxPoints += cut_unitTests.tests[testId].setup->points;
    switch (result->status) {
    case cut_RESULT_OK:
        ++shepherd->succeeded;
        shepherd->points += cut_unitTests.tests[testId].setup->points;
        break;
    case cut_RESULT_SUPPRESSED:
        ++shepherd->suppressed;
        break;
    case cut_RESULT_SKIPPED:
        ++shepherd->skipped;
        break;
    case cut_RESULT_FILTERED_OUT:
        ++shepherd->filteredOut;
        break;
    default:
        ++shepherd->failed;
        break;
    }
}

CUT_PRIVATE void cut_EndSubtests_std(struct cut_Shepherd *shepherd, int testId) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;

    data->attributes[testId].base = fprintf(shepherd->output, "[%3i] %s (overall)", shepherd->executed, cut_unitTests.tests[testId].setup->name);

    struct cut_UnitResult result;
    memset(&result, 0, sizeof(result));
    result.status = data->attributes[testId].subtestsFailed ? cut_RESULT_FAILED : cut_RESULT_OK;

    cut_EndSingleTest_std(shepherd, testId, &result);
}

CUT_PRIVATE void cut_EndTest_std(struct cut_Shepherd *shepherd, int testId, const struct cut_UnitResult *result) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;

    if (data->attributes[testId].subtests)
        cut_EndSubtests_std(shepherd, testId);
    else
        cut_EndSingleTest_std(shepherd, testId, result);
}

CUT_PRIVATE void cut_Finalize_std(struct cut_Shepherd *shepherd) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;

    fprintf(shepherd->output,
            "\nSummary:\n"
            "  tests:        %3i\n"
            "  succeeded:    %3i\n"
            "  filtered out: %3i\n"
            "  suppressed:   %3i\n"
            "  skipped:      %3i\n"
            "  failed:       %3i\n",
            cut_unitTests.size,
            shepherd->succeeded,
            shepherd->filteredOut,
            shepherd->suppressed,
            shepherd->skipped,
            shepherd->failed);

    if (0.000000001 <= shepherd->maxPoints) {
        fprintf(shepherd->output,
            "\n"
            "Points:         %6.2f\n",
            shepherd->points);
    }
}

CUT_PRIVATE void cut_Clear_std(struct cut_Shepherd *shepherd) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    free(data->attributes);
    free(data);
}

CUT_PRIVATE void cut_InitOutput_std(struct cut_Shepherd *shepherd) {

    shepherd->data = malloc(sizeof(struct cut_OutputData_std));
    if (!shepherd->data)
        CUT_DIE("cannot allocate memory for output");

    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    
    data->attributes = (struct cut_TestAttributes_std *) malloc(sizeof(struct cut_TestAttributes_std) * cut_unitTests.size);
    if (!data->attributes)
        CUT_DIE("cannot allocate memory for output");
    memset(data->attributes, 0, sizeof(struct cut_TestAttributes_std) * cut_unitTests.size);

    shepherd->startTest = cut_StartTest_std;
    shepherd->startSubTests = cut_StartSubTests_std;
    shepherd->endSubTest = cut_EndSubTest_std;
    shepherd->endTest = cut_EndTest_std;
    shepherd->finalize = cut_Finalize_std;
    shepherd->clear = cut_Clear_std;
}


CUT_NS_END

#endif