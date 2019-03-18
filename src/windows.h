#ifndef CUT_WINDOWS_H
#define CUT_WINDOWS_H

# include <Windows.h>
# include <io.h>
# include <fcntl.h>

CUT_PRIVATE HANDLE cut_jobGroup;

CUT_PRIVATE int cut_IsDebugger() {
    return IsDebuggerPresent();
}

CUT_PRIVATE int cut_IsTerminalOutput() {
    return _isatty(_fileno(stdout));
}

CUT_PRIVATE void cut_RedirectIO() {
    cut_outputsRedirected = 1;
    cut_stdout = tmpfile();
    cut_stderr = tmpfile();
    cut_originalStdOut = _dup(1);
    cut_originalStdErr = _dup(2);

    _dup2(_fileno(cut_stdout), 1);
    _dup2(_fileno(cut_stderr), 2);
}

CUT_PRIVATE void cut_ResumeIO() {
    fclose(cut_stdout) != -1 || cut_FatalExit("cannot close file");
    fclose(cut_stderr) != -1 || cut_FatalExit("cannot close file");
    _close(1) != -1 || cut_FatalExit("cannot close file");
    _close(2) != -1 || cut_FatalExit("cannot close file");
    _dup2(cut_originalStdOut, 1);
    _dup2(cut_originalStdErr, 2);

    cut_outputsRedirected = 0;
}

CUT_PRIVATE int64_t cut_Read(int fd, char *destination, size_t bytes) {
    return _read(fd, destination, bytes);
}

CUT_PRIVATE int64_t cut_Write(int fd, const char *source, size_t bytes) {
    return _write(fd, source, bytes);
}

CUT_PRIVATE void NTAPI cut_TimerCallback(CUT_UNUSED(void *param), CUT_UNUSED(BOOLEAN timerEvent)) {
    cut_Timeouted();
    ExitProcess(cut_NORMAL_EXIT);
}

CUT_PRIVATE int cut_PreRun() {
    if (!cut_arguments.noFork && cut_arguments.testId < 0) {
        // create a group of processes to be able to kill unit when parent dies
		cut_jobGroup = CreateJobObject(NULL, NULL);
        if (!cut_jobGroup)
            cut_FatalExit("cannot create jobGroup object");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(cut_jobGroup, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)) ||cut_FatalExit("cannot SetInformationJobObject");
    }
    if (cut_arguments.testId < 0)
        return 0;

    SetErrorMode(SEM_NOGPFAULTERRORBOX);

    HANDLE timer = NULL;
    if (cut_arguments.timeout) {
        CreateTimerQueueTimer(&timer, NULL, cut_TimerCallback, NULL, 
                              cut_arguments.timeout * 1000, 0, WT_EXECUTEONLYONCE) || cut_FatalExit("cannot create timer");
    }

    cut_pipeWrite = _dup(1);
    _setmode(cut_pipeWrite, _O_BINARY);

    cut_ExceptionBypass(cut_arguments.testId, cut_arguments.subtestId);

    _close(cut_pipeWrite) != -1 || cut_FatalExit("cannot close file");

    if (timer) {
        DeleteTimerQueueTimer(NULL, timer, NULL) || cut_FatalExit("cannot delete timer");
    }

    return 1;
}


CUT_PRIVATE void cut_RunUnit(int testId, int subtest, struct cut_UnitResult *result) {

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE childOutWrite, childOutRead;

    CreatePipe(&childOutRead, &childOutWrite, &saAttr, 0);
    SetHandleInformation(childOutRead, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION procInfo;
    STARTUPINFOA startInfo;

    ZeroMemory(&procInfo, sizeof(procInfo));
    ZeroMemory(&startInfo, sizeof(startInfo));

    startInfo.cb = sizeof(startInfo);
    startInfo.dwFlags |= STARTF_USESTDHANDLES;
    startInfo.hStdOutput = childOutWrite;

    const char *fmtString = "\"%s\" --test %i --subtest %i --timeout %i";
    int length = snprintf(NULL, 0, fmtString, cut_arguments.selfName, testId, subtest,
                          cut_arguments.timeout);
    char *command = (char *)malloc(length + 1);
    sprintf(command, fmtString, cut_arguments.selfName, testId, subtest, cut_arguments.timeout);
            
    CreateProcessA(cut_arguments.selfName,
                   command,
                   NULL,
                   NULL,
                   TRUE,
                   CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED,
                   NULL,
                   NULL,
                   &startInfo,
                   &procInfo) || cut_FatalExit("cannot create process");
    free(command);

    AssignProcessToJobObject(cut_jobGroup, procInfo.hProcess) || cut_FatalExit("cannot assign process to job object");
    ResumeThread(procInfo.hThread) == 1 || cut_FatalExit("cannot resume thread");
	CloseHandle(childOutWrite) || cut_FatalExit("cannot close handle");

    cut_pipeRead = _open_osfhandle((intptr_t)childOutRead, 0);
    cut_PipeReader(result);

    WaitForSingleObject(procInfo.hProcess, INFINITE) == WAIT_OBJECT_0 || cut_FatalExit("cannot wait for single object");
    DWORD childResult;
    GetExitCodeProcess(procInfo.hProcess, &childResult) || cut_FatalExit("cannot get exit code");
    CloseHandle(procInfo.hProcess) || cut_FatalExit("cannot close handle");
    CloseHandle(procInfo.hThread) || cut_FatalExit("cannot close handle");

    result->returnCode = childResult;
    result->signal = 0;
    result->failed |= result->returnCode;

    _close(cut_pipeRead) != -1 || cut_FatalExit("cannot close file");
}

CUT_PRIVATE int cut_ReadWholeFile(int fd, char *buffer, size_t length) {
    while (length) {
        int64_t rv = _read(fd, buffer, length);
        if (rv < 0)
            return -1;
        buffer += rv;
        length -= (size_t)rv;
    }
    return 0;
}

int cut_File(FILE *f, const char *content) {
    int result = 0;
    size_t length = strlen(content);
    _flushall();
    int fd = _fileno(f);
    char *buf = NULL;

    long offset = _lseek(fd, 0, SEEK_CUR);
    if ((size_t) _lseek(fd, 0, SEEK_END) != length)
        goto cleanup;

    _lseek(fd, 0, SEEK_SET);
    buf = (char*)malloc(length);
    if (!buf)
        cut_FatalExit("cannot allocate memory for file");
    if (cut_ReadWholeFile(fd, buf, length))
        cut_FatalExit("cannot read whole file");

    result = memcmp(content, buf, length) == 0;
cleanup:
    _lseek(fd, offset, SEEK_SET);
    free(buf);
    return result;
}

CUT_PRIVATE int cut_PrintColorized(enum cut_Colors color, const char *text) {
    HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    WORD attributes = 0;

    GetConsoleScreenBufferInfo(stdOut, &info);
    switch (color) {
    case cut_YELLOW_COLOR:
        attributes = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
        break;
    case cut_RED_COLOR:
        attributes = FOREGROUND_INTENSITY | FOREGROUND_RED;
        break;
    case cut_GREEN_COLOR:
        attributes = FOREGROUND_INTENSITY | FOREGROUND_GREEN;
        break;
    default:
        break;
    }
    if (attributes)
        SetConsoleTextAttributes(stdOut, attributes);
    int rv = fprintf(cut_output, "%s", text);
    if (attributes)
        SetConsoleTextAttributes(stdOut, info.wAttributes);
    return rv;
}

#endif // CUT_WINDOWS_H
