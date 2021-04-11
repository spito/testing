#ifndef CUT_WINDOWS_H
#define CUT_WINDOWS_H

#include <Windows.h>
#include <io.h>
#include <fcntl.h>

#include "declarations.h"
#include "globals.h"

CUT_NS_BEGIN

CUT_PRIVATE HANDLE cut_jobGroup;

CUT_PRIVATE int cut_IsDebugger() {
    return IsDebuggerPresent();
}

CUT_PRIVATE int cut_IsTerminalOutput() {
    return _isatty(_fileno(stdout));
}

CUT_PRIVATE int cut_CreateTemporaryFile(FILE **file) {
    const char *name = tmpnam(NULL);
    while (*name == '/' || *name == '\\')
        ++name;
    *file = fopen(name, "w+TD");
    return !!*file;
}

CUT_PRIVATE void cut_RedirectIO() {
    cut_outputsRedirected = 1;
    cut_CreateTemporaryFile(&cut_stdin) || CUT_DIE("cannot open temporary file");
    cut_CreateTemporaryFile(&cut_stdout) || CUT_DIE("cannot open temporary file");
    cut_CreateTemporaryFile(&cut_stderr) || CUT_DIE("cannot open temporary file");
    cut_originalStdIn = _dup(0);
    cut_originalStdOut = _dup(1);
    cut_originalStdErr = _dup(2);
    _dup2(_fileno(cut_stdin), 0);
    _dup2(_fileno(cut_stdout), 1);
    _dup2(_fileno(cut_stderr), 2);
}

CUT_PRIVATE void cut_ResumeIO() {
    fclose(cut_stdin) != -1 || CUT_DIE("cannot close file");
    fclose(cut_stdout) != -1 || CUT_DIE("cannot close file");
    fclose(cut_stderr) != -1 || CUT_DIE("cannot close file");
    _close(0) != -1 || CUT_DIE("cannot close file");
    _close(1) != -1 || CUT_DIE("cannot close file");
    _close(2) != -1 || CUT_DIE("cannot close file");
    _dup2(cut_originalStdIn, 0);
    _dup2(cut_originalStdOut, 1);
    _dup2(cut_originalStdErr, 2);

    cut_outputsRedirected = 0;
}

CUT_PRIVATE int cut_ReopenFile(FILE *file) {
    int fd = _dup(_fileno(file));
    _lseek(fd, 0, SEEK_SET);
    return fd;
}

CUT_PRIVATE void cut_CloseFile(int fd) {
    _close(fd);
}

CUT_PRIVATE int64_t cut_Read(int fd, char *destination, size_t bytes) {
    return _read(fd, destination, bytes);
}

CUT_PRIVATE int64_t cut_Write(int fd, const char *source, size_t bytes) {
    return _write(fd, source, bytes);
}

CUT_PRIVATE void NTAPI cut_TimerCallback(CUT_UNUSED(void *param), CUT_UNUSED(BOOLEAN timerEvent)) {
    cut_Timedout();
    ExitProcess(cut_NORMAL_EXIT);
}

CUT_PRIVATE int cut_PreRun(const struct cut_Arguments *arguments) {
    if (!arguments->noFork && arguments->testId < 0) {
        // create a group of processes to be able to kill unit when parent dies
		cut_jobGroup = CreateJobObject(NULL, NULL);
        if (!cut_jobGroup)
            CUT_DIE("cannot create jobGroup object");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(cut_jobGroup, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)) ||CUT_DIE("cannot SetInformationJobObject");
    }
    if (arguments->testId < 0)
        return 0;

    SetErrorMode(SEM_NOGPFAULTERRORBOX);

    HANDLE timer = NULL;
    if (arguments->timeout) {
        CreateTimerQueueTimer(&timer, NULL, cut_TimerCallback, NULL, 
                              arguments->timeout * 1000, 0, WT_EXECUTEONLYONCE) || CUT_DIE("cannot create timer");
    }

    cut_pipeWrite = _dup(1);
    _setmode(cut_pipeWrite, _O_BINARY);

    cut_ExceptionBypass(arguments->testId, arguments->subtestId);

    _close(cut_pipeWrite) != -1 || CUT_DIE("cannot close file");

    if (timer) {
        DeleteTimerQueueTimer(NULL, timer, NULL) || CUT_DIE("cannot delete timer");
    }

    return 1;
}


CUT_PRIVATE void cut_RunUnit(struct cut_Shepherd *shepherd, int testId, int subtest, struct cut_UnitResult *result) {

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
    int length = snprintf(NULL, 0, fmtString, shepherd->arguments->selfName, testId, subtest,
                          shepherd->arguments->timeout);
    char *command = (char *)malloc(length + 1);
    sprintf(command, fmtString, shepherd->arguments->selfName, testId, subtest,
            shepherd->arguments->timeout);
            
    CreateProcessA(shepherd->arguments->selfName,
                   command,
                   NULL,
                   NULL,
                   TRUE,
                   CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED,
                   NULL,
                   NULL,
                   &startInfo,
                   &procInfo) || CUT_DIE("cannot create process");
    free(command);

    AssignProcessToJobObject(cut_jobGroup, procInfo.hProcess) || CUT_DIE("cannot assign process to job object");
    ResumeThread(procInfo.hThread) == 1 || CUT_DIE("cannot resume thread");
	CloseHandle(childOutWrite) || CUT_DIE("cannot close handle");

    int pipeRead = _open_osfhandle((intptr_t)childOutRead, 0);
    cut_PipeReader(pipeRead, result);

    WaitForSingleObject(procInfo.hProcess, INFINITE) == WAIT_OBJECT_0 || CUT_DIE("cannot wait for single object");
    DWORD childResult;
    GetExitCodeProcess(procInfo.hProcess, &childResult) || CUT_DIE("cannot get exit code");
    CloseHandle(procInfo.hProcess) || CUT_DIE("cannot close handle");
    CloseHandle(procInfo.hThread) || CUT_DIE("cannot close handle");

    result->returnCode = childResult;
    result->signal = 0;
    if (result->returnCode)
        result->status = cut_RESULT_RETURNED_NON_ZERO;

    _close(pipeRead) != -1 || CUT_DIE("cannot close file");
}

CUT_PRIVATE int cut_PrintColorized(FILE *output, enum cut_Colors color, const char *text) {
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
        SetConsoleTextAttribute(stdOut, attributes);
    int rv = fprintf(output, "%s", text);
    if (attributes)
        SetConsoleTextAttribute(stdOut, info.wAttributes);
    return rv;
}

CUT_NS_END

#endif // CUT_WINDOWS_H
