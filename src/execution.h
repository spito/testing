#ifndef CUT_EXECUTION_H
#define CUT_EXECUTION_H

#include "declarations.h"
#include "globals.h"
#include "messages.h"
#include "output-json.h"
#include "output-standard.h"

#if defined(__cplusplus)

# include <stdexcept>
# include <typeinfo>
# include <string>

CUT_PRIVATE void cut_ExceptionBypass(struct cut_UnitTest *test) {
    cut_RedirectIO();
    if (!setjmp(cut_executionPoint)) {
        try {
            test->setup->test(0, test->currentResult->id);
            cut_SendOK();
        } catch (const std::exception &e) {
            std::string name = typeid(e).name();
            cut_StopException(name.c_str(), e.what() ? e.what() : "(no reason)");
        } catch (...) {
            cut_StopException("unknown type", "(empty message)");
        }
    }

    fflush(stdout);
    fflush(stderr);
    cut_ResumeIO();
}

#else

CUT_NS_BEGIN

CUT_PRIVATE void cut_ExceptionBypass(struct cut_UnitTest *test) {
    cut_RedirectIO();
    if (!setjmp(cut_executionPoint)) {
        test->setup->test(0, test->currentResult->id);
        cut_SendOK();
    }

    fflush(stdout);
    fflush(stderr);
    cut_ResumeIO();
}

CUT_NS_END

#endif

CUT_NS_BEGIN

CUT_PRIVATE void cut_RunUnitForkless(struct cut_Shepherd *shepherd, struct cut_UnitTest *test) {
    cut_ExceptionBypass(test);
    cut_PipeReader(-1, test);
    cut_ResetLocalMessage();
}

CUT_PRIVATE void cut_RunUnitTest(struct cut_Shepherd *shepherd, struct cut_UnitTest *test) {

    shepherd->unitRunner(shepherd, test);

    if (test->currentResult->status != cut_RESULT_OK || test->resultSize == 1)
        return;
    int start = 1;
    int stop = test->resultSize;
    shepherd->startSubTests(shepherd, test);

    if (0 < shepherd->arguments->subtestId) {
        if (test->resultSize <= shepherd->arguments->subtestId)
            cut_ErrorExit("Invalid argument - requested to run subtest %d, only avaliable %d subtests are for %s test",
                shepherd->arguments->subtestId, test->resultSize, test->setup->name);
        start = shepherd->arguments->subtestId;
        stop = shepherd->arguments->subtestId + 1;
    }

    for (int i = start; i < stop; ++i) {
        test->currentResult = &test->results[i];
        shepherd->startSubTest(shepherd, test);
        shepherd->unitRunner(shepherd, test);
        shepherd->endSubTest(shepherd, test);
    }
}

CUT_PRIVATE void cut_SkipDependingUnitTests(struct cut_Shepherd *shepherd, struct cut_QueueItem *item, enum cut_ResultStatus result) {
    struct cut_QueueItem *cursor = item->depending.first;
    for (; cursor; cursor = cursor->next) {
        cut_unitTests.tests[cursor->testId].results[0].status = result;
    }
}

CUT_PRIVATE void cut_RunUnitTests(struct cut_Shepherd *shepherd) {

    struct cut_QueueItem *item = NULL;
    while ((item = cut_QueuePopTest(shepherd->queuedTests))) {
        struct cut_UnitTest *test = &cut_unitTests.tests[item->testId];

        shepherd->startTest(shepherd, test);

        if (test->results[0].status == cut_RESULT_UNKNOWN) {
            if (test->setup->suppress)
                test->results[0].status = cut_RESULT_SUPPRESSED;
            else if (cut_FilterOutUnit(shepherd->arguments, test))
                test->results[0].status = cut_RESULT_FILTERED_OUT;
            else
                cut_RunUnitTest(shepherd, test);
        }

        for (int s = 1; s < test->resultSize; ++s) {
            if (test->results[s].status != cut_RESULT_OK) {
                test->results[0].status = cut_RESULT_FAILED;
            }
        }

        if (test->results[0].status != cut_RESULT_OK)
            cut_SkipDependingUnitTests(shepherd, item, cut_RESULT_SKIPPED);

        cut_QueueMeltTest(shepherd->queuedTests, item);

        shepherd->endTest(shepherd, test);
    }
}

CUT_PRIVATE int cut_FilterOutUnit(const struct cut_Arguments *arguments, const struct cut_UnitTest *test) {
    if (arguments->testId >= 0)
        return test->id != arguments->testId;
    if (!arguments->matchSize)
        return 0;

    for (int i = 0; i < arguments->matchSize; ++i) {
        if (strstr(test->setup->name, arguments->match[i]))
            return 0;
    }
    return 1;
}

CUT_PRIVATE int cut_TestComparator(const void *_lhs, const void *_rhs) {
    struct cut_UnitTest *lhs = (struct cut_UnitTest *)_lhs;
    struct cut_UnitTest *rhs = (struct cut_UnitTest *)_rhs;

    int result = strcmp(lhs->setup->file, rhs->setup->file);
    if (!result)
        result = lhs->setup->line <= rhs->setup->line ? -1 : 1;
    return result;
}

struct cut_SortedTestItem {
    const char *name;
    int testId;
};

CUT_PRIVATE int cut_TestFinder(const void *_name, const void *_test) {
    const char *name = (const char *)_name;
    const struct cut_SortedTestItem *test = (const struct cut_SortedTestItem *)_test;

    return strcmp(name, test->name);
}

CUT_PRIVATE int cut_SortTestsByName(const void *_lhs, const void *_rhs) {
    const struct cut_SortedTestItem *lhs = (const struct cut_SortedTestItem *) _lhs;
    const struct cut_SortedTestItem *rhs = (const struct cut_SortedTestItem *) _rhs;
    return strcmp(lhs->name, rhs->name);
}

CUT_PRIVATE void cut_EnqueueTests(struct cut_Shepherd *shepherd) {
    int cyclicDependency = 0;
    struct cut_EnqueuePair *tests = (struct cut_EnqueuePair *) malloc(sizeof(struct cut_EnqueuePair) * cut_unitTests.size);
    memset(tests, 0, sizeof(struct cut_EnqueuePair) * cut_unitTests.size);
    struct cut_Queue localQueue;
    cut_InitQueue(&localQueue);

    for (int testId = 0; testId < cut_unitTests.size; ++testId) {
        cut_unitTests.tests[testId].id = testId;
        if (shepherd->arguments->timeoutDefined || !cut_unitTests.tests[testId].setup->timeoutDefined) {
            cut_unitTests.tests[testId].setup->timeout = shepherd->arguments->timeout;
        }

        if (cut_unitTests.tests[testId].setup->needSize == 1) {
            tests[testId].self = cut_QueuePushTest(shepherd->queuedTests, testId);
        }
        else {
            tests[testId].appliedNeeds = (int *) malloc(sizeof(int) * cut_unitTests.tests[testId].setup->needSize);
            memset(tests[testId].appliedNeeds, 0, sizeof(int) * cut_unitTests.tests[testId].setup->needSize);
            cut_QueuePushTest(&localQueue, testId);
        }
    }

    struct cut_SortedTestItem *sortedTests = (struct cut_SortedTestItem *) malloc(sizeof(struct cut_SortedTestItem) * cut_unitTests.size);
    memset(sortedTests, 0, sizeof(struct cut_SortedTestItem) * cut_unitTests.size);

    for (int testId = 0; testId < cut_unitTests.size; ++testId) {
        sortedTests[testId].name = cut_unitTests.tests[testId].setup->name;
        sortedTests[testId].testId = testId;
    }

    qsort(sortedTests, cut_unitTests.size, sizeof(struct cut_SortedTestItem), cut_SortTestsByName);

    struct cut_QueueItem *current = cut_QueuePopTest(&localQueue);
    for (; current; current = cut_QueuePopTest(&localQueue)) {
        int testId = current->testId;
        const struct cut_Setup *setup = cut_unitTests.tests[testId].setup;
        int needSatisfaction = setup->needSize - 1;
        for (size_t n = 1; n < setup->needSize; ++n, --needSatisfaction) {
            if (tests[testId].appliedNeeds[n])
                continue;
            struct cut_SortedTestItem *need = (struct cut_SortedTestItem *) bsearch(
                setup->needs[n], sortedTests, cut_unitTests.size, sizeof(struct cut_SortedTestItem),
                cut_TestFinder);
            if (!need)
                cut_ErrorExit("Test %s depends on %s; such test does not exists, however.", setup->name, setup->needs[n]);
            if (!tests[need->testId].self) {
                cut_QueueRePushTest(&localQueue, current);
                if (cut_unitTests.size < ++tests[testId].retry) {
                    cyclicDependency = 1;
                    goto end;
                }
                break;
            }
            tests[testId].appliedNeeds[n] = need->testId + 1;
        }
        if (needSatisfaction)
            continue;
        for (size_t n = 1; n < setup->needSize; ++n) {
            int needId = tests[testId].appliedNeeds[n] - 1;
            if (!tests[testId].self)
                tests[testId].self = cut_QueuePushTest(&tests[needId].self->depending, testId);
            else
                tests[testId].self = cut_QueueAddTest(&tests[needId].self->depending, tests[testId].self);
        }
    }
end:
    for (int testId = 0; testId < cut_unitTests.size; ++testId)
        free(tests[testId].appliedNeeds);
    free(tests);
    free(sortedTests);
    cut_ClearQueue(&localQueue);

    if (cyclicDependency)
        cut_ErrorExit("There is a cyclic dependency between tests.");
}

CUT_PRIVATE void cut_InitShepherd(struct cut_Shepherd *shepherd, const struct cut_Arguments *arguments, struct cut_Queue *queue) {
    cut_noFork = arguments->noFork;
    shepherd->arguments = arguments;
    shepherd->queuedTests = queue;
    shepherd->executed = 0;
    shepherd->succeeded = 0;
    shepherd->suppressed = 0;
    shepherd->filteredOut = 0;
    shepherd->skipped = 0;
    shepherd->failed = 0;
    shepherd->points = 0;
    shepherd->maxPoints = 0;
    shepherd->startTest = (void (*)(struct cut_Shepherd *, const struct cut_UnitTest *test)) cut_ShepherdNoop;
    shepherd->startSubTests = (void (*)(struct cut_Shepherd *, const struct cut_UnitTest *test)) cut_ShepherdNoop;
    shepherd->startSubTest = (void (*)(struct cut_Shepherd *, const struct cut_UnitTest *test)) cut_ShepherdNoop;
    shepherd->endSubTest = (void (*)(struct cut_Shepherd *, const struct cut_UnitTest *test)) cut_ShepherdNoop;
    shepherd->endTest = (void (*)(struct cut_Shepherd *, const struct cut_UnitTest *test)) cut_ShepherdNoop;
    shepherd->finalize = (void (*)(struct cut_Shepherd *)) cut_ShepherdNoop;
    shepherd->clear = (void (*)(struct cut_Shepherd *)) cut_ShepherdNoop;

    shepherd->unitRunner = arguments->noFork ? cut_RunUnitForkless : cut_RunUnit;

    shepherd->output = stdout;
    if (arguments->output) {
        shepherd->output = fopen(arguments->output, "w");
        if (!shepherd->output)
            cut_ErrorExit("cannot open file %s for writing", arguments->output);
    }

    if (!strcmp(arguments->format, "json"))
        cut_InitOutput_json(shepherd);
    else
        cut_InitOutput_std(shepherd);
}

CUT_PRIVATE void cut_ClearShepherd(struct cut_Shepherd *shepherd) {
    cut_ClearQueue(shepherd->queuedTests);
    shepherd->clear(shepherd);

    if (shepherd->output != stdout)
        fclose(shepherd->output);
    free(shepherd->arguments->match);
    for (int i = 0; i < cut_unitTests.size; ++i) {
        for (int s = 1; s < cut_unitTests.tests[i].resultSize; ++s)
            free(cut_unitTests.tests[i].results[s].name);
        free(cut_unitTests.tests[i].results);
    }
    free(cut_unitTests.tests);
}


CUT_PRIVATE int cut_Runner(const struct cut_Arguments *arguments) {
    int returnValue = 0;
    struct cut_Queue queue;
    struct cut_Shepherd shepherd;

    cut_InitQueue(&queue);
    cut_InitShepherd(&shepherd, arguments, &queue);

    qsort(cut_unitTests.tests, cut_unitTests.size, sizeof(struct cut_UnitTest), cut_TestComparator);
    cut_EnqueueTests(&shepherd);

    if (!cut_PreRun(arguments)) {
        cut_RunUnitTests(&shepherd);

        shepherd.finalize(&shepherd);
        returnValue = shepherd.failed;
    }

    cut_ClearShepherd(&shepherd);
    return returnValue;
}

CUT_NS_END

#endif // CUT_EXECUTION_H
