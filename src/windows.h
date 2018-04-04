#ifndef CUT_WINDOWS_H
#define CUT_WINDOWS_H

# include <Windows.h>
# include <io.h>

CUT_PRIVATE HANDLE cut_jobGroup;

CUT_PRIVATE int cut_SendMessage(const struct cut_Fragment *message) {
    size_t remaining = message->serializedLength;
    size_t position = 0;

    ssize_t r;
    while (remaining && (r = _write(cut_pipeWrite, message->serialized + position, remaining)) > 0) {
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
    if (cut_arguments.testId < 0)
        return;
    // TODO setup timer (threads+exit?)
    cut_ExceptionBypass(cut_arguments.testId, cut_arguments.subtestId);
    // TODO check if something is needed to send (everything should be handled in bypass)
}


CUT_PRIVATE void cut_RunUnit(int testId, int subtest, struct cut_UnitResult *result) {

   // create a group of processes to be able to kill unit when parent dies
    jobGroup = CreateJobObject( NULL, NULL );
    if ( !jobGroup )
        cut_FatalExit("cannot create jobGroup object");
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(jobGroup, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
        cut_FatalExit("cannot SetInformationJobObject");

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
    startInfo.hStdError = childDebugWrite;

    const char *fmtString = "\"%s\" --test %i --subtest %i";
    int length = snprintf(NULL, 0, fmtString, cut_arguments.selfName, testId.name, subtest);
    char *command = malloc(length + 1);
    sprintf(command, fmtString, cut_arguments.selfName, testId, subtest);
            
    if (!CreateProcessA(cut_arguments.selfName,
                        command,
                        NULL,
                        NULL,
                        TRUE,
                        CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED,
                        NULL,
                        NULL,
                        &startInfo,
                        &procInfo)) {
        cut_FatalExit("cannot create process");
    }
    if (!AssignProcessToJobObject(cut_jobGroup, procInfo.hProcess))
        cut_FatalExit("cannot assign process to job object");
    if (ResumeThread(procInfo.hThread) != 1)
        cut_FatalExit("cannot resume thread");

    WaitForSingleObject(procInfo.hProcess, INFINITE);
    DWORD childResult;
    GetExitCodeProcess( procInfo.hProcess, &childResult );
    CloseHandle( procInfo.hProcess );
    CloseHandle( procInfo.hThread );

    if (childResult) {
        testInfo()->result = testAbnormal;
        testInfo()->returnCode = childResult;
        testInfo()->signal = 0;
    }
    else {
        DWORD read;
        ReadFile( childOutRead, testInfo(), sizeof( struct TestInfo ), &read, NULL );
    }

    int capacity = 1024;
    int size = 0;
    testDebug = malloc( capacity + 1 );
    while ( 1 ) {
        DWORD read;
        ReadFile( childDebugRead, testDebug + size, capacity - size, &read, NULL );
        size += read;
        if ( size != capacity )
            break;
        capacity += 1024;
        char *resized = realloc( testDebug, capacity + 1 );
        if ( !resized )
            break;
        testDebug = resized;
    }
    testDebug[ size ] = '\0';

    CloseHandle( childOutRead );
    CloseHandle( childOutWrite );
    CloseHandle( childDebugRead );
    CloseHandle( childDebugWrite );}

CUT_PRIVATE int testReadWholeFile(int fd, char *buffer, size_t length) {
    while (length) {
        int rv = _read(fd, buffer, length);
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
    _flushall();
    int fd = _fileno(f);
    char *buf = NULL;

    long offset = _lseek(fd, 0, SEEK_CUR);
    if ((size_t) _lseek(fd, 0, SEEK_END) != length)
        goto leave;

    _lseek(fd, 0, SEEK_SET);
    buf = (char*)malloc(length);
    if (!buf)
        cut_FatalExit("cannot allocate memory for file");
    if (testReadWholeFile(fd, buf, length))
        cut_FatalExit("cannot read whole file");

    result = memcmp(content, buf, length) == 0;
leave:
    _lseek(fd, offset, SEEK_SET);
    free(buf);
    return result;
}

#endif // CUT_WINDOWS_H
