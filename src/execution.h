#ifndef CUT_EXECUTION_H
#define CUT_EXECUTION_H

#include "declarations.h"
#include "globals.h"

#if defined(__cplusplus)

#  include <stdexcept>
#  include <typeinfo>
#  include <string>

CUT_PRIVATE void cut_ExceptionBypass(int testId, int subtest) {
    cut_RedirectIO();
    if (setjmp(cut_executionPoint))
        goto cleanup;
    if (cut_globalTearUp)
        cut_globalTearUp();
    try {
        int counter = 0;
        cut_unitTests.tests[testId].instance(&counter, subtest);
        cut_SendOK(counter);
    } catch (const std::exception &e) {
        std::string name = typeid(e).name();
        cut_StopException(name.c_str(), e.what() ? e.what() : "(no reason)");
    } catch (...) {
        cut_StopException("unknown type", "(empty message)");
    }
cleanup:
    if (cut_globalTearDown)
        cut_globalTearDown();
    cut_ResumeIO();
}

# else

CUT_NS_BEGIN

CUT_PRIVATE void cut_ExceptionBypass(int testId, int subtest) {
    cut_RedirectIO();
    if (setjmp(cut_executionPoint))
        goto cleanup;
    if (cut_globalTearUp)
        cut_globalTearUp();
    int counter = 0;
    cut_unitTests.tests[testId].instance(&counter, subtest);
    cut_SendOK(counter);
cleanup:
    if (cut_globalTearDown)
        cut_globalTearDown();
    cut_ResumeIO();
}

CUT_NS_END

# endif

CUT_NS_BEGIN

CUT_PRIVATE void cut_RunUnitForkless(struct cut_Shepherd *shepherd, int testId, int subtest,
                                     struct cut_UnitResult *result) {
    cut_ExceptionBypass(testId, subtest);
    cut_PipeReader(-1, result);
    cut_ResetLocalMessage();
    result->returnCode = 0;
    result->signal = 0;
}

CUT_PRIVATE void cut_RunUnitTest(struct cut_Shepherd *shepherd, struct cut_UnitResult *result, int testId) {
    shepherd->unitRunner(shepherd, testId, 0, result);

    if (result->subtests > 0 && result->status == cut_RESULT_OK)
        cut_RunUnitSubTests(shepherd, testId, result->subtests);
}

CUT_PRIVATE void cut_SkipDependingUnitTests(struct cut_Shepherd *shepherd, struct cut_QueueItem *item, enum cut_SkipReason reason) {
    struct cut_QueueItem *cursor = item->depending.first;
    for (; cursor; cursor = cursor->next) {
        cut_unitTests.tests[cursor->testId].skipReason = reason;
    }
}

CUT_PRIVATE void cut_RunUnitTests(struct cut_Shepherd *shepherd) {

    struct cut_QueueItem *item = NULL;
    while ((item = cut_QueuePopTest(shepherd->queuedTests))) {
        struct cut_UnitResult result;
        memset(&result, 0, sizeof(result));

        shepherd->startTest(shepherd, item->testId);

        if (cut_unitTests.tests[item->testId].skipReason != cut_SKIP_REASON_NO_SKIP)
            result.status = (enum cut_ResultStatus) cut_unitTests.tests[item->testId].skipReason;
        else if (cut_unitTests.tests[item->testId].settings->suppress)
            result.status = cut_RESULT_SUPPRESSED;
        else if (cut_FilterOutUnit(shepherd->arguments, item->testId))
            result.status = cut_RESULT_FILTERED_OUT;
        else
            cut_RunUnitTest(shepherd, &result, item->testId);

        switch (result.status) {
        case cut_RESULT_OK:
            break;
        case cut_RESULT_FILTERED_OUT:
            cut_SkipDependingUnitTests(shepherd, item, cut_SKIP_REASON_FILTERED_OUT);
            break;
        case cut_RESULT_SUPPRESSED:
            cut_SkipDependingUnitTests(shepherd, item, cut_SKIP_REASON_SUPPRESSED);
            break;
        default:
            cut_SkipDependingUnitTests(shepherd, item, cut_SKIP_REASON_FAILED);
            break;
        }
        cut_QueueMeltTest(shepherd->queuedTests, item);

        shepherd->endTest(shepherd, item->testId, &result);
        cut_ClearUnitResult(&result);
    }
}

CUT_PRIVATE void cut_RunUnitSubTest(struct cut_Shepherd *shepherd, int testId, int subtest) {
    struct cut_UnitResult result;
    memset(&result, 0, sizeof(result));

    shepherd->startSubTest(shepherd, testId, subtest);
    shepherd->unitRunner(shepherd, testId, subtest, &result);
    shepherd->endSubTest(shepherd, testId, subtest, &result);

    cut_ClearUnitResult(&result);
}

CUT_PRIVATE void cut_RunUnitSubTests(struct cut_Shepherd *shepherd, int testId, int subtests) {

    shepherd->startSubTests(shepherd, testId, subtests);

    if (shepherd->arguments->subtestId >= 0) {
        if (subtests <= shepherd->arguments->subtestId) {
            cut_ErrorExit("Invalid argument - requested to run subtest %d, only avaliable %d subtests are for %s test",
                shepherd->arguments->subtestId, subtests, cut_unitTests.tests[testId].name);
        }
        cut_RunUnitSubTest(shepherd, testId, shepherd->arguments->subtestId);
        return;
    }

    for (int subtest = 1; subtest <= subtests; ++subtest) {
        cut_RunUnitSubTest(shepherd, testId, subtest);
    }
}


CUT_PRIVATE int cut_FilterOutUnit(const struct cut_Arguments *arguments, int testId) {
    if (arguments->testId >= 0)
        return testId != arguments->testId;
    if (!arguments->matchSize)
        return 0;
    const char *name = cut_unitTests.tests[testId].name;
    for (int i = 0; i < arguments->matchSize; ++i) {
        if (strstr(name, arguments->match[i]))
            return 0;
    }
    return 1;
}

CUT_PRIVATE int cut_TestComparator(const void *_lhs, const void *_rhs) {
    struct cut_UnitTest *lhs = (struct cut_UnitTest *)_lhs;
    struct cut_UnitTest *rhs = (struct cut_UnitTest *)_rhs;

    int result = strcmp(lhs->file, rhs->file);
    if (!result)
        result = lhs->line <= rhs->line ? -1 : 1;
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

    struct cut_SortedTestItem *sortedTests = (struct cut_SortedTestItem *) malloc(sizeof(struct cut_SortedTestItem) * cut_unitTests.size);
    memset(sortedTests, 0, sizeof(struct cut_SortedTestItem) * cut_unitTests.size);

    for (int testId = 0; testId < cut_unitTests.size; ++testId) {
        sortedTests[testId].name = cut_unitTests.tests[testId].name;
        sortedTests[testId].testId = testId;
    }

    qsort(sortedTests, cut_unitTests.size, sizeof(struct cut_SortedTestItem), cut_SortTestsByName);

    for (int testId = 0; testId < cut_unitTests.size; ++testId) {
        if (shepherd->arguments->timeoutDefined || !cut_unitTests.tests[testId].settings->timeoutDefined) {
            cut_unitTests.tests[testId].settings->timeout = shepherd->arguments->timeout;
        }

        if (cut_unitTests.tests[testId].settings->needSize == 1) {
            tests[testId].self = cut_QueuePushTest(shepherd->queuedTests, testId);
        }
        else {
            tests[testId].appliedNeeds = (int *) malloc(sizeof(int) * cut_unitTests.tests[testId].settings->needSize);
            memset(tests[testId].appliedNeeds, 0, sizeof(int) * cut_unitTests.tests[testId].settings->needSize);
            cut_QueuePushTest(&localQueue, testId);
        }
    }

    struct cut_QueueItem *current = cut_QueuePopTest(&localQueue);
    for (; current; current = cut_QueuePopTest(&localQueue)) {
        int testId = current->testId;
        const struct cut_Settings *settings = cut_unitTests.tests[testId].settings;
        int needSatisfaction = settings->needSize - 1;
        for (size_t n = 1; n < settings->needSize; ++n, --needSatisfaction) {
            if (tests[testId].appliedNeeds[n])
                continue;
            struct cut_SortedTestItem *need = (struct cut_SortedTestItem *) bsearch(
                settings->needs[n], sortedTests, cut_unitTests.size, sizeof(struct cut_SortedTestItem),
                cut_TestFinder);
            if (!need)
                cut_ErrorExit("Test %s depends on %s; such test does not exists, however.", cut_unitTests.tests[testId].name, settings->needs[n]);
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
        for (size_t n = 1; n < settings->needSize; ++n) {
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
    shepherd->listTests = (void (*)(const struct cut_Shepherd *)) cut_ShepherdNoop;
    shepherd->startTest = (void (*)(struct cut_Shepherd *, int)) cut_ShepherdNoop;
    shepherd->startSubTests = (void (*)(struct cut_Shepherd *, int, int)) cut_ShepherdNoop;
    shepherd->startSubTest = (void (*)(struct cut_Shepherd *, int, int)) cut_ShepherdNoop;
    shepherd->endSubTest = (void (*)(struct cut_Shepherd *, int, int, const struct cut_UnitResult *)) cut_ShepherdNoop;
    shepherd->endTest = (void (*)(struct cut_Shepherd *, int, const struct cut_UnitResult *)) cut_ShepherdNoop;
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
    free(cut_unitTests.tests);
}


CUT_PRIVATE int cut_Runner(int argc, char **argv) {
    int returnValue = 0;
    struct cut_Arguments arguments;
    struct cut_Queue queue;
    struct cut_Shepherd shepherd;

    cut_ParseArguments(&arguments, argc, argv);
    cut_InitQueue(&queue);
    cut_InitShepherd(&shepherd, &arguments, &queue);

    if (arguments.help) {
        returnValue = cut_Help(&arguments);
        goto cleanup;
    }

    qsort(cut_unitTests.tests, cut_unitTests.size, sizeof(struct cut_UnitTest), cut_TestComparator);
    cut_EnqueueTests(&shepherd);

    if (arguments.list) {
        returnValue = cut_List(&shepherd);
        goto cleanup;
    }

    if (cut_PreRun(&arguments)) {
        goto cleanup;
    }

    cut_RunUnitTests(&shepherd);

    shepherd.finalize(&shepherd);
    returnValue = shepherd.failed;

cleanup:
    cut_ClearShepherd(&shepherd);
    return returnValue;
}

CUT_NS_END

#endif // CUT_EXECUTION_H
