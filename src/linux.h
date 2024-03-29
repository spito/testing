#ifndef CUT_LINUX_H
#define CUT_LINUX_H

# include <unistd.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <sys/types.h>
# include <sys/prctl.h>
# include <fcntl.h>
# include <signal.h>

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
        r = prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (r == -1)
            cut_FatalExit("cannot set child death signal");
        if (getppid() != parentPid)
            exit(cut_ERROR_EXIT);
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
    waitpid(pid, &status, 0) != -1 || cut_FatalExit("cannot wait for unit");
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
    const char *desired = "TracerPid:";
    const char *found = NULL;
    int tracerPid;
    int result = 0;
    int status = open("/proc/self/status", O_RDONLY);
    if (status < 0)
        return 0;

    char *buffer;
    size_t length;
    
    if (!cut_ReadWholeFile(status, &buffer, &length))
        goto cleanup;

    found = strstr(buffer, desired);
    if (!found)
        goto cleanup;

    if (!sscanf(found + strlen(desired), "%i", &tracerPid))
        goto cleanup;

    if (tracerPid)
        result = 1;
cleanup:
    free(buffer);
    close(status);
    return result;
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
#endif // CUT_LINUX_H
