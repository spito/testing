#ifndef CUT_LINUX_H
#define CUT_LINUX_H

#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "declarations.h"
#include "file-operations.h"

#include "common-nix.h"

CUT_NS_BEGIN

CUT_PRIVATE void cut_RunUnit(struct cut_Shepherd *shepherd, struct cut_UnitTest *test) {
    int r;
    int pipefd[2];
    r = pipe(pipefd);
    if (r == -1)
        CUT_DIE("cannot establish communication pipe");

    int pipeRead = pipefd[0];
    int pipeWrite = pipefd[1];

    int pid = getpid();
    int parentPid = getpid();
    pid = fork();
    if (pid == -1)
        CUT_DIE("cannot fork");
    if (!pid) {
        r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1)
            CUT_DIE("cannot set child death signal");
        if (getppid() != parentPid)
            exit(cut_ERROR_EXIT);
        close(pipeRead) != -1 || CUT_DIE("cannot close file");
        cut_pipeWrite = pipeWrite;

        int timeout = test->setup->timeout;
        if (timeout) {
            signal(SIGALRM, cut_SigAlrm);
            alarm(timeout);
        }
        cut_ExceptionBypass(test);

        close(cut_pipeWrite) != -1 || CUT_DIE("cannot close file");
        cut_ClearShepherd(shepherd);
        exit(cut_NORMAL_EXIT);
    }
    // parent process only
    int status = 0;
    close(pipeWrite) != -1 || CUT_DIE("cannot close file");
    cut_PipeReader(pipeRead, test);
    do {
        r = waitpid(pid, &status, 0);
    } while (r == -1 && errno == EINTR);
    r != -1 || CUT_DIE("cannot wait for unit");
    test->currentResult->returnCode = WIFEXITED(status) ? WEXITSTATUS(status) : 0;
    test->currentResult->signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
    if (test->currentResult->signal)
        test->currentResult->status = cut_RESULT_SIGNALLED;
    else if (test->currentResult->returnCode)
        test->currentResult->status = cut_RESULT_RETURNED_NON_ZERO;
    close(pipeRead) != -1 || CUT_DIE("cannot close file");
}

CUT_PRIVATE int cut_IsDebugger() {
    const char *desired = "TracerPid:";
    const size_t desiredLength = strlen(desired);
    const char *found = NULL;
    int tracerPid = 0;
    int result = 0;
    FILE *status = fopen("/proc/self/status", "r");
    if (!status)
        return 0;

    size_t length;
    char *buffer = cut_ReadWholeFile(status, &length);
    fclose(status);

    if (!buffer)
        return 0;

    char *content = (char *) realloc(buffer, length + 1);
    if (content) {
        buffer = content;
        buffer[length] = '\0';
        found = (char *) memmem(buffer, length, desired, desiredLength);
    }

    if (found && desiredLength + 2 <= found - buffer) {
        sscanf(found + desiredLength, "%i", &tracerPid);
    }

    free(buffer);
    return !!tracerPid;
}

CUT_NS_END

#endif // CUT_LINUX_H
