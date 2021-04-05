#ifndef CUT_UNIX_H
#define CUT_UNIX_H

# include <sys/types.h>
# include <sys/sysctl.h>
# include <unistd.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <fcntl.h>
# include <signal.h>
# include <errno.h>
# include <assert.h>

CUT_PRIVATE int cut_IsTerminalOutput() {
    return isatty(fileno(stdout));
}

CUT_PRIVATE void cut_RedirectIO() {
    cut_outputsRedirected = 1;
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
    cut_outputsRedirected = 0;
}

CUT_PRIVATE int64_t cut_Read(int fd, char *destination, size_t bytes) {
    return read(fd, destination, bytes);
}

CUT_PRIVATE int64_t cut_Write(int fd, const char *source, size_t bytes) {
    return write(fd, source, bytes);
}

CUT_PRIVATE int cut_PreRun() {
    return 0;
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
        /// TODO: missing feature - kill child when parent dies
        close(cut_pipeRead) != -1 || cut_FatalExit("cannot close file");

        if (cut_arguments.timeout) {
            signal(SIGALRM, cut_SigAlrm);
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
    do {
        r = waitpid( pid, &status, 0 );
    } while (r == -1 && errno == EINTR);
    r != -1 || cut_FatalExit("cannot wait for unit");
    result->returnCode = WIFEXITED(status) ? WEXITSTATUS(status) : 0;
    result->signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
    result->failed |= result->returnCode ||  result->signal;
    close(cut_pipeRead) != -1 || cut_FatalExit("cannot close file");
}

CUT_PRIVATE int cut_ReadWholeFile(int fd, char **buffer, size_t *length) {
    int result = 0;
    size_t capacity = 16;
    *length = 0;
    *buffer = (char *) malloc(capacity);
    if (!*buffer)
        return 0;

    long offset = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);

    int rv;
    while ((rv = read(fd, *buffer + *length, capacity - *length - 1)) > 0) {
        *length += rv;
        if (*length + 1 == capacity) {
            capacity *= 2;
            char *newBuffer = (char *)realloc(*buffer, capacity);
            if (!newBuffer)
                break;
            *buffer = newBuffer;
        }
    }

    if (!rv)
        result = 1;
    (*buffer)[*length] = '\0';
cleanup:
    lseek(fd, offset, SEEK_SET);
    return result;
}

int cut_File(FILE *f, const char *content) {
    int result = 0;
    size_t length = strlen(content);
    size_t fileLength;
    int fd = fileno(f);
    fflush(f);
    char *buf = NULL;

    if (!cut_ReadWholeFile(fd, &buf, &fileLength))
        cut_FatalExit("cannot read whole file");

    result = length == fileLength && memcmp(content, buf, length) == 0;
cleanup:
    free(buf);
    return result;
}

CUT_PRIVATE int cut_IsDebugger() {
    int                 junk;
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;

    info.kp_proc.p_flag = 0;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);


    return (info.kp_proc.p_flag & P_TRACED);
}

CUT_PRIVATE int cut_PrintColorized(enum cut_Colors color, const char *text) {
    const char *prefix;
    const char *suffix = "\x1B[0m";
    switch (color) {
    case cut_YELLOW_COLOR:
        prefix = "\x1B[1;33m";
        break;
    case cut_RED_COLOR:
        prefix = "\x1B[1;31m";
        break;
    case cut_GREEN_COLOR:
        prefix = "\x1B[1;32m";
        break;
    default:
        prefix = "";
        suffix = "";
        break;
    }
    return fprintf(cut_output, "%s%s%s", prefix, text, suffix);
}
#endif // CUT_UNIX_H
