#ifndef CUT_UNIX_H
#define CUT_UNIX_H

# include <unistd.h>
# include <sys/wait.h>
# include <sys/types.h>
# include <sys/prctl.h>

CUT_PRIVATE int cut_SendMessage(const struct cut_Fragment *message) {
    size_t remaining = message->serializedLength;
    size_t position = 0;

    ssize_t r;
    while ((r = write(cut_pipeWrite, message->serialized + position, remaining)) > 0) {
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
            message->serialized = realloc(message->serialized, message->serializedLength);
            if (!message->serialized)
                cut_FatalExit("cannot allocate memory for reading a message");
        }
        r = read(cut_pipeRead, message->serialized + processed, toRead);
    }
    return toRead != -1;
}


CUT_PRIVATE void cut_PreRun(void *data) {
}

CUT_PRIVATE void cut_RunUnit(int testId, int subtest, int timeout, int noFork, struct cut_UnitResult *result) {
    int r;
    int pipefd[2];
    r = pipe(pipefd);
    if (r == -1)
        cut_FatalExit("cannot establish communication pipe");

    cut_pipeRead = pipefd[0];
    cut_pipeWrite = pipefd[1];

    int pid = getpid();
    int parentPid = getpid();
    if (!noFork) {
        pid = fork();
        if (pid == -1)
            cut_FatalExit("cannot fork");
    }
    if (!pid || noFork) {
        if (!noFork) {
            cut_spawnChild = 1;
            r = prctl(PR_SET_PDEATHSIG, SIGTERM);
            if (r == -1)
                cut_FatalExit("cannot set child death signal");
            if (getppid() != parentPid)
                exit(cut_ERROR_EXIT);
            close(cut_pipeRead) != -1 || cut_FatalExit("cannot close file");
        }
        cut_stdout = tmpfile();
        cut_stderr = tmpfile();
        int originalStdOut = dup(1);
        int originalStdErr = dup(2);

        dup2(fileno(cut_stdout), 1);
        dup2(fileno(cut_stderr), 2);

        if (!noFork && timeout)
            alarm(timeout);
        cut_ExceptionBypass(testId, subtest);

        fclose(cut_stdout) != -1 || cut_FatalExit("cannot close file");
        fclose(cut_stderr) != -1 || cut_FatalExit("cannot close file");
        close(1) != -1 || cut_FatalExit("cannot close file");
        close(2) != -1 || cut_FatalExit("cannot close file");
        close(cut_pipeWrite) != -1 || cut_FatalExit("cannot close file");
        if (!noFork) {
            free(cut_unitTests.tests);
            exit(cut_OK_EXIT);
        } else {
            dup2(originalStdOut, 1);
            dup2(originalStdErr, 2);
        }
    }
    if (pid || noFork) {
        int status = 0;
        if (!noFork) {
            close(cut_pipeWrite) != -1 || cut_FatalExit("cannot close file");
        }
        cut_PipeReader(result);
        if (!noFork) {
            waitpid(pid, &status, 0) != -1 || cut_FatalExit("cannot wait for unit");
        }
        if (!noFork) {
            result->returnCode = WIFEXITED(status) ? WEXITSTATUS(status) : 0;
            result->signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
            result->failed |= result->returnCode ||  result->signal;
        } else {
            result->returnCode = 0;
            result->signal = 0;
        }
        close(cut_pipeRead) != -1 || cut_FatalExit("cannot close file");
    }
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
