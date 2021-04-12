#ifndef CUT_OUTPUT_STD_H
#define CUT_OUTPUT_STD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "declarations.h"
#include "globals.h"

CUT_NS_BEGIN

struct cut_TestAttributes_std {
    int subtests;
    int subtestsFailed;
    int repeatedSubtest;
    int base;
};

struct cut_OutputData_std {
    struct cut_TestAttributes_std *attributes;
};


CUT_PRIVATE void cut_StartTest_std(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;

    ++shepherd->executed;

    int base = fprintf(shepherd->output, "[%3i] %s", shepherd->executed, test->setup->name);
    fflush(shepherd->output);
    data->attributes[test->id].base = base;
    data->attributes[test->id].subtests = 0;
    data->attributes[test->id].subtestsFailed = 0;
}

CUT_PRIVATE void cut_StartSubTests_std(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    data->attributes[test->id].subtests = test->resultSize - 1;
    data->attributes[test->id].repeatedSubtest = 1;

    for (int i = 2; i < test->resultSize; ++i) {
        if (strcmp(test->results[i - 1].name, test->results[i].name)) {
            data->attributes[test->id].repeatedSubtest = 0;
            break;
        }
    }

    fprintf(shepherd->output, ": %d subtests\n", data->attributes[test->id].subtests);
    fflush(shepherd->output);
}


CUT_PRIVATE int cut_PrintDetailResult(struct cut_Shepherd *shepherd, const char *indent, const struct cut_UnitTest *test) {
    int extended = 0;
    const struct cut_UnitResult *result = test->currentResult;
    for (const struct cut_Info *current = result->check; current; current = current->next) {
        fprintf(shepherd->output, "%scheck '%s' (%s:%d)\n", indent, current->message,
                cut_ShortPath(shepherd->arguments, current->file), current->line);
        extended = 1;
    }
    switch (result->status) {
    case cut_RESULT_TIMED_OUT:
        fprintf(shepherd->output, "%stimed out (%d s)\n", indent, test->setup->timeout);
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

CUT_PRIVATE void cut_EndSubTest_std(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    static const char *shortIndent = "    ";
    static const char *longIndent = "        ";
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    enum cut_Colors color;
    const char *status = cut_GetStatus(test->currentResult, &color);
    int lastPosition = 80 - 1 - strlen(status);
    if (test->currentResult->status != cut_RESULT_OK)
        ++data->attributes[test->id].subtestsFailed;

    const char *indent = shortIndent;
    if (!data->attributes[test->id].repeatedSubtest || test->currentResult->id <= 1)
        lastPosition -= fprintf(shepherd->output, "%s%s", indent, test->currentResult->name);
    else {
        lastPosition -= fprintf(shepherd->output, "%s", indent);
        int length = strlen(test->currentResult->name);
        for (int i = 0; i < length; ++i)
            putc(' ', shepherd->output);
        lastPosition -= length;
    }
    if (data->attributes[test->id].repeatedSubtest)
        lastPosition -= fprintf(shepherd->output, " #%d", test->currentResult->id);
    indent = longIndent;

    for (int i = 0; i < lastPosition; ++i) {
        putc('.', shepherd->output);
    }
    if (shepherd->arguments->noColor)
        fprintf(shepherd->output, status);
    else
        cut_PrintColorized(shepherd->output, color, status);

    putc('\n', shepherd->output);
    if (cut_PrintDetailResult(shepherd, indent, test))
        putc('\n', shepherd->output);
    fflush(shepherd->output);


}

CUT_PRIVATE void cut_EndSingleTest_std(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    static const char *indent = "    ";
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;
    enum cut_Colors color;
    const char *status = cut_GetStatus(test->results, &color);
    int lastPosition = 80 - 1 - strlen(status) - data->attributes[test->id].base;
    int extended = 0;

    for (int i = 0; i < lastPosition; ++i) {
        putc('.', shepherd->output);
    }
    if (shepherd->arguments->noColor)
        fprintf(shepherd->output, status);
    else
        cut_PrintColorized(shepherd->output, color, status);

    putc('\n', shepherd->output);
    if (!data->attributes[test->id].subtests) {
        extended = cut_PrintDetailResult(shepherd, indent, test);
        if (extended)
            putc('\n', shepherd->output);
    }
    fflush(shepherd->output);

    shepherd->maxPoints += test->setup->points;
    switch (test->results->status) {
    case cut_RESULT_OK:
        ++shepherd->succeeded;
        shepherd->points += test->setup->points;
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

CUT_PRIVATE void cut_EndSubtests_std(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;

    data->attributes[test->id].base = fprintf(shepherd->output, "[%3i] %s (overall)", shepherd->executed, test->setup->name);
    cut_EndSingleTest_std(shepherd, test);
}

CUT_PRIVATE void cut_EndTest_std(struct cut_Shepherd *shepherd, const struct cut_UnitTest *test) {
    struct cut_OutputData_std *data = (struct cut_OutputData_std *)shepherd->data;

    if (data->attributes[test->id].subtests)
        cut_EndSubtests_std(shepherd, test);
    else
        cut_EndSingleTest_std(shepherd, test);
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