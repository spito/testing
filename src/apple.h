#ifndef CUT_APPLE_H
#define CUT_APPLE_H

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "declarations.h"
#include "file-operations.h"

#include "common-nix.h" // force

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
        /// TODO: missing feature - kill child when parent dies
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
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;

    info.kp_proc.p_flag = 0;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    size = sizeof(info);
    if (sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0))
        return 0;

    return !!(info.kp_proc.p_flag & P_TRACED);
}

CUT_NS_END

#endif // CUT_APPLE_H
