#ifndef CUT_EXECUTION_H
#define CUT_EXECUTION_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif

# ifdef __cplusplus
} // extern C

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

extern "C" {
# else
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
# endif


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

CUT_PRIVATE void cut_RunUnitTests(struct cut_Shepherd *shepherd) {

    struct cut_QueueItem *item = NULL;
    while ((item = cut_QueuePopTest(shepherd->queuedTests))) {
        struct cut_UnitResult result;
        memset(&result, 0, sizeof(result));

        shepherd->startTest(shepherd, item->testId);

        if (cut_unitTests.tests[item->testId].settings->suppress)
            result.status = cut_RESULT_SUPPRESSED;
        else if (cut_FilterOutUnit(shepherd->arguments, item->testId))
            result.status = cut_RESULT_FILTERED_OUT;
        else
            cut_RunUnitTest(shepherd, &result, item->testId);
        // TODO: handle dependent tests

        shepherd->endTest(shepherd, item->testId, &result);

        cut_MeltQueueItem(shepherd->queuedTests, item);
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

CUT_PRIVATE void cut_EnqueueTests(struct cut_Shepherd *shepherd) {
    for (int testId = 0; testId < cut_unitTests.size; testId++) {
        if (shepherd->arguments->timeoutDefined || !cut_unitTests.tests[testId].settings->timeoutDefined) {
            cut_unitTests.tests[testId].settings->timeout = shepherd->arguments->timeout;
        }

        cut_QueuePushTest(shepherd->queuedTests, testId);
    }
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

#endif // CUT_EXECUTION_H
