#ifndef CUT_UNIX_H
#define CUT_UNIX_H

# include <unistd.h>
# include <sys/wait.h>
# include <sys/types.h>
# include <sys/prctl.h>
# include <signal.h>

CUT_PRIVATE void cut_RedirectIO() {
    cut_stdout = tmpfile();
    cut_stderr = tmpfile();
    cut_originalStdOut = dup(1);
    cut_originalStdErr = dup(2);

    dup2(fileno(cut_stdout), 1);
    dup2(fileno(cut_stderr), 2);
}

CUT_PRIVATE void cut_ResumeIO() {
    fclose(cut_stdout) != -1 || cut_FatalExit("cannot close file");
    fclose(cut_stderr) != -1 || cut_FatalExit("cannot close file");
    close(1) != -1 || cut_FatalExit("cannot close file");
    close(2) != -1 || cut_FatalExit("cannot close file");
    dup2(cut_originalStdOut, 1);
    dup2(cut_originalStdErr, 2);
}

CUT_PRIVATE int cut_SendMessage(const struct cut_Fragment *message) {
    size_t remaining = message->serializedLength;
    size_t position = 0;

    ssize_t r;
    while (remaining && (r = write(cut_pipeWrite, message->serialized + position, remaining)) > 0) {
        position += r;
        remaining -= r;
    }
    return r != -1;
}

CUT_PRIVATE int cut_ReadMessage(struct cut_Fragment *message) {
    cut_FragmentReceiveStatus status = CUT_FRAGMENT_RECEIVE_STATUS;

    message->serialized = NULL;
    message->serializedLength = 0;
    ssize_t r = 0;
    ssize_t toRead = 0;
    while ((toRead = cut_FragmentReceiveContinue(&status, message->serialized, r)) > 0) {
        size_t processed = cut_FragmentReceiveProcessed(&status);

        if (message->serializedLength < processed + toRead) {
            message->serializedLength = processed + toRead;
            message->serialized = (char *)realloc(message->serialized, message->serializedLength);
            if (!message->serialized)
                cut_FatalExit("cannot allocate memory for reading a message");
        }
        r = read(cut_pipeRead, message->serialized + processed, toRead);
    }
    return toRead != -1;
}

CUT_PRIVATE void cut_PreRun() {
}

CUT_PRIVATE void cut_SigAlrm(CUT_UNUSED(int signum)) {
    cut_Timeouted();
    _exit(cut_NORMAL_EXIT);
}

CUT_PRIVATE void cut_RunUnit(int testId, int subtest, struct cut_UnitResult *result) {
    int r;
    int pipefd[2];
    r = pipe(pipefd);
    if (r == -1)
        cut_FatalExit("cannot establish communication pipe");

    cut_pipeRead = pipefd[0];
    cut_pipeWrite = pipefd[1];

    int pid = getpid();
    int parentPid = getpid();
    pid = fork();
    if (pid == -1)
        cut_FatalExit("cannot fork");
    if (!pid) {
        cut_spawnChild = 1;
        r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1)
            cut_FatalExit("cannot set child death signal");
        if (getppid() != parentPid)
            exit(cut_ERROR_EXIT);
        close(cut_pipeRead) != -1 || cut_FatalExit("cannot close file");

        if (cut_arguments.timeout) {
            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = cut_SigAlrm;
            sigaction(SIGALRM, &sa, NULL);
            alarm(cut_arguments.timeout);
        }
        cut_ExceptionBypass(testId, subtest);

        close(cut_pipeWrite) != -1 || cut_FatalExit("cannot close file");
        free(cut_unitTests.tests);
        free(cut_arguments.match);
        if (cut_arguments.output)
            fclose(cut_output);
        exit(cut_NORMAL_EXIT);
    }
    // parent process only
    int status = 0;
    close(cut_pipeWrite) != -1 || cut_FatalExit("cannot close file");
    cut_PipeReader(result);
    waitpid(pid, &status, 0) != -1 || cut_FatalExit("cannot wait for unit");
    result->returnCode = WIFEXITED(status) ? WEXITSTATUS(status) : 0;
    result->signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
    result->failed |= result->returnCode ||  result->signal;
    close(cut_pipeRead) != -1 || cut_FatalExit("cannot close file");
}

CUT_PRIVATE int testReadWholeFile(int fd, char *buffer, size_t length) {
    while (length) {
        int rv = read(fd, buffer, length);
        if (rv < 0)
            return -1;
        buffer += rv;
        length -= rv;
    }
    return 0;
}

int cut_File(FILE *f, const char *content) {
    int result = 0;
    size_t length = strlen(content);
    int fd = fileno(f);
    fflush(f);
    char *buf = NULL;

    long offset = lseek(fd, 0, SEEK_CUR);
    if ((size_t) lseek(fd, 0, SEEK_END) != length)
        goto leave;

    lseek(fd, 0, SEEK_SET);
    buf = (char*)malloc(length);
    if (!buf)
        cut_FatalExit("cannot allocate memory for file");
    if (testReadWholeFile(fd, buf, length))
        cut_FatalExit("cannot read whole file");

    result = memcmp(content, buf, length) == 0;
leave:
    lseek(fd, offset, SEEK_SET);
    free(buf);
    return result;
}
#endif
