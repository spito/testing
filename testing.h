#ifndef TESTING_H
#define TESTING_H

#ifndef TESTING

# define ASSERT( e ) (void)0
# define ASSERT_FILE( f, content ) (void)0
# define TEST( name ) static void unitTest_ ## name()
# define TEST_FAILING( name ) static void unitTest_ ## name()
# define DEBUG_MSG( ... ) (void)0

#else

#if defined(__GNUC__) || defined(__clang)
# define NORETURN __attribute__((noreturn))
# define INLINE inline
#elif defined(_MSC_VER)
# define NORETURN __declspec(noreturn)
# define INLINE
#else
# define NORETURN
# define INLINE
#endif

#define _POSIX_C_SOURCE 1

#include <stdio.h>

typedef void(*TestRunner)();

struct TestInfo;

void testRegister( TestRunner run, const char *name, int failing );
NORETURN void testFinish();
int testFile( FILE *f, const char *content );
void testFillInfo( const char *stm, const char *file, int line );
struct TestInfo *testInfo();
void testDebugMsg( const char *file, size_t line, const char *fmt, ... );

#if defined(__GNUC__) || defined(__clang)

# define CONSTRUCTOR(name) __attribute__((constructor)) static void name()

#elif defined(_MSC_VER)

# pragma section(".CRT$XCU",read)
# define CONSTRUCTOR(name)                                                      \
    static void __cdecl name( void );                                           \
    __declspec(allocate(".CRT$XCU")) void (__cdecl * name ## _)(void) = name;   \
    static void __cdecl name( void )

#else
# error "unsupported compiler"
#endif

# define ASSERT( e ) do { if ( !(e) ) {                                         \
        testFillInfo( #e, __FILE__, __LINE__ );                                 \
        testFinish();                                                           \
    } } while( 0 )

# define ASSERT_FILE( f, content ) do {                                         \
    if ( !testFile( f, content ) ) {                                            \
        testFillInfo( "content of file is not equal", __FILE__, __LINE__ );     \
        testFinish();                                                           \
    } } while( 0 )

# define TEST( name )                                                           \
    void unitTest_ ## name();                                                   \
    CONSTRUCTOR( testRegister ## name ) {                                       \
        testRegister( unitTest_ ## name, #name, 0 );                            \
    }                                                                           \
    void unitTest_ ## name()

# define TEST_FAILING( name )                                                   \
    void unitTest_ ## name();                                                   \
    CONSTRUCTOR( testRegister ## name ) {                                       \
        testRegister( unitTest_ ## name, #name, 1 );                            \
    }                                                                           \
    void unitTest_ ## name()

# define DEBUG_MSG(...) testDebugMsg( __FILE__, __LINE__, __VA_ARGS__ )

#if defined(TESTING_MAIN)

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#if defined( _WIN32 )
# include <Windows.h>
# include <io.h>
# include <fcntl.h>
#elif defined( __unix ) || defined(__APPLE__)
# include <unistd.h>
# include <sys/wait.h>
# include <sys/types.h>
#else
# error "unsupported platform"
#endif

#ifdef __linux__
# include <sys/prctl.h>
#endif

NORETURN void testInternalError() {
#if defined(__unix) || defined(__APPLE__)
    perror( "Internal error occurred inside testing framework" );
#elif defined(_WIN32)
    char *msg;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (LPSTR)&msg,
        0, NULL );

    fprintf( stderr, "Internal error occurred inside testing framework: %s\n", msg );
    LocalFree( msg );
#endif
    exit( -1 );
}
NORETURN void testExplicitError(int eCode) {
#if defined(__unix) || defined(__APPLE__)
    errno = eCode;
#elif defined(_WIN32)
    SetLastError( eCode );
#endif
    testInternalError();
}

enum TestResult {
    testSuccess = 0,
    testFail = 1,
    testAbnormal = 2
};

struct TestInfo {
    const char *name;
    int order;
    int id;
    int failing;
    enum TestResult result;
    int signal;
    int returnCode;
    int pipeEnd;
    int debugPipe;
    const char *errStatement;
    const char *errFile;
    int errLine;
};

struct TestItem {
    TestRunner run;
    const char *name;
    int failing;
};

static int testIndex = 0;
static int testCapacity = 0;
static struct TestItem *testAll = NULL;
static struct TestInfo info;
static FILE *testTmpOut = NULL;
static FILE *testTmpErr = NULL;
static char *testDebug = NULL;
#if defined( _WIN32 )
static const char *testProgramName = NULL;
static const char *testSwitch = "--test-id";
static HANDLE jobGroup;
#endif

struct TestInfo *testInfo() {
    return &info;
}

void testRegister( TestRunner run, const char *name, int failing ) {
    if ( testIndex == testCapacity ) {
        testCapacity += 16;
        void *tmp = realloc( testAll, sizeof( struct TestItem ) * testCapacity );
        if ( !tmp ) {
            free( testAll );
            testExplicitError( ENOMEM );
        }
        testAll = tmp;
    }

    testAll[ testIndex ].run = run;
    testAll[ testIndex ].name = name;
    testAll[ testIndex ].failing = failing;
    ++testIndex;
}
static int testReadWholeFile( int fd, char *buffer, size_t length ) {
    while (length) {
#if defined(_WIN32)
        int rv = _read( fd, buffer, length );
#elif defined(__unix) || defined(__APPLE__)
        int rv = read( fd, buffer, length );
#endif
        if ( rv < 0 )
            return -1;
        buffer += rv;
        length -= rv;
    }
    return 0;
}

#if defined(_WIN32)
int testFile( FILE *f, const char *content ) {
    int result = 0;
    long length = strlen( content );
    _flushall();
    int fd = _fileno( f );
    char *buf = NULL;

    long offset = _lseek( fd, 0, SEEK_CUR );
    if ( _lseek( fd, 0, SEEK_END ) != length )
        goto leave;

    _lseek( fd, 0, SEEK_SET );
    buf = (char*)malloc( length );
    if ( !buf )
        testExplicitError( ENOMEM );
    if ( testReadWholeFile( fd, buf, length ) )
        testInternalError();

    result = memcmp( content, buf, length ) == 0;
leave:
    _lseek( fd, offset, SEEK_SET );
    free( buf );
    return result;
}
#elif defined(__unix) || defined(__APPLE__)
int testFile( FILE *f, const char *content ) {
    int result = 0;
    size_t length = strlen( content );
    int fd = fileno( f );
    fflush(f);
    char *buf = NULL;

    long offset = lseek( fd, 0, SEEK_CUR );
    if ( (size_t) lseek( fd, 0, SEEK_END ) != length )
        goto leave;

    lseek( fd, 0, SEEK_SET );
    buf = (char*)malloc( length );
    if ( !buf )
        testExplicitError( ENOMEM );
    if ( testReadWholeFile( fd, buf, length ) )
        testInternalError();

    result = memcmp( content, buf, length ) == 0;
leave:
    lseek( fd, offset, SEEK_SET );
    free( buf );
    return result;
}
#endif

#if defined(_WIN32)
NORETURN void testFinish() {
    int r;
    r = _write( testInfo()->pipeEnd, testInfo(), sizeof( struct TestInfo ) );
    if ( r != sizeof( struct TestInfo ) )
        testInternalError();
    _close( 1 );
    _close( 2 );
    _close( testInfo()->pipeEnd );
    _close( testInfo()->debugPipe );

    free( testAll );
    if ( testTmpOut )
        fclose( testTmpOut );
    if ( testTmpErr )
        fclose( testTmpErr );
    ExitProcess( 0 );
}
#elif defined(__unix) || defined(__APPLE__)
NORETURN void testFinish() {
    int r;
    r = write( testInfo()->pipeEnd, testInfo(), sizeof( struct TestInfo ) );
    if ( r != sizeof( struct TestInfo ) )
        testInternalError();
    r = close( 1 );
    if ( r == -1 )
        testInternalError();
    r = close( 2 );
    if ( r == -1 )
        testInternalError();
    r = close( testInfo()->pipeEnd );
    if ( r == -1 )
        testInternalError();
    r = close( testInfo()->debugPipe );
    if ( r == -1 )
        testInternalError();

    free( testAll );
    if ( testTmpOut )
        fclose( testTmpOut );
    if ( testTmpErr )
        fclose( testTmpErr );
    exit( 0 );
}
#endif

void testFillInfo( const char *stm, const char *file, int line ) {
    testInfo()->result = testFail;
    testInfo()->errStatement = stm;
    testInfo()->errFile = file;
    testInfo()->errLine = line;
}

static int argMatch( const char *name, int argc, const char **argv ) {
    for ( int i = 1; i < argc; ++i ) {
        if ( strstr( name, argv[ i ] ) )
            return 1;
    }
    return 0;
}

static INLINE void fill( FILE *output, int n ) {
    for ( int i = 0; i < n; ++i )
        putc( '.', output );
}

static int printDefail( FILE *output, int base ) {

    int failed = 0;
    const char *msg = "OK";

    if ( testInfo()->result == testAbnormal ) {
        msg = "FAIL";
        failed = 1;
    }
    else if ( testInfo()->failing && testInfo()->result == testSuccess ) {
        msg = "FAIL - should failed";
        failed = 1;
    }
    else if ( !testInfo()->failing && testInfo()->result == testFail ) {
        msg = "FAIL";
        failed = 1;
    }

    fill( output, 80 - 1 - base - strlen( msg ) );
    fprintf( output, "%s\n", msg );

    if ( failed ) {
        if ( testInfo()->result == testAbnormal ) {
            fprintf( output, "\ttest exited abnormally\n" );
            if ( testInfo()->returnCode )
                fprintf( output, "\treturn code: %i\n", testInfo()->returnCode );
            if ( testInfo()->signal )
                fprintf( output, "\tsignal code: %i\n", testInfo()->signal );
        }
        else if ( testInfo()->result == testFail )
            fprintf( output, "\tassert '%s' on %s : %i\n", testInfo()->errStatement, testInfo()->errFile, testInfo()->errLine );
    }

    if ( testDebug && *testDebug )
        fprintf( output, "\tdebug messages:\n%s", testDebug );
    if ( testDebug ) {
        free( testDebug );
        testDebug = NULL;
    }

    return failed;
}

void testDebugMsg( const char *file, size_t line, const char *fmt, ... ) {
    va_list argsPrepare;
    va_list args;
    va_start( argsPrepare, fmt );
    va_copy( args, argsPrepare );
    size_t textLength = 1 + vsnprintf( NULL, 0, fmt, argsPrepare );

    char *text = malloc( textLength );
    char *message = NULL;
    if ( !text )
        goto leave;

    vsnprintf( text, textLength, fmt, args );

    size_t messageLength = 1 + snprintf( NULL, 0, "\t\t%s (%s:%lu)\n", text, file, line );
    message = malloc( messageLength );
    if ( !message )
        goto leave;

    snprintf( message, messageLength, "\t\t%s (%s:%lu)\n", text, file, line );

#if defined(__unix) || defined(__APPLE__)
    write( testInfo()->debugPipe, message, messageLength - 1 );
#elif defined(_WIN32)
    _write( testInfo()->debugPipe, message, messageLength - 1 );
#endif

leave:
    va_end( args );
    va_end( argsPrepare );
    free( text );
    free( message );
}

#if defined(__unix) || defined(__APPLE__)
static void testExecute( TestRunner run ) {

    int r;
    int pipefd[ 2 ];
    r = pipe( pipefd );
    if ( r != 0 )
        testInternalError();

    int debugPipe[ 2 ];
    r = pipe( debugPipe );
    if ( r != 0 )
        testInternalError();

    int parentPid = getpid();
    int pid = fork();
    if ( r == -1 )
        testInternalError();

    if ( pid ) { // parent
        r = close( pipefd[ 1 ] ); // disable writing
        if ( r == -1 )
            testInternalError();

        r = close( debugPipe[ 1 ] ); // disable writing;
        if ( r == -1 )
            testInternalError();

        int status = 0;
#if defined(__APPLE__)
        do {
            r = waitpid( pid, &status, 0 );
        } while ( r == -1 && errno == EINTR );
#else
        r = waitpid( pid, &status, 0 );
#endif
        if ( r == -1 )
            testInternalError();

        if ( !WIFEXITED( status ) || ( WIFEXITED( status ) && WEXITSTATUS( status ) ) ) {
            testInfo()->result = testAbnormal;
            testInfo()->returnCode = WIFEXITED( status ) ? WEXITSTATUS( status ) : 0;
            testInfo()->signal = WIFSIGNALED( status ) ? WTERMSIG( status ) : 0;
        }
        else {
            r = read( pipefd[ 0 ], testInfo(), sizeof( struct TestInfo ) );
            if ( r != sizeof( struct TestInfo ) )
                testInternalError();
        }
        int capacity = 1024;
        int size = 0;
        testDebug = malloc( capacity );
        while ( ( r = read( debugPipe[ 0 ], testDebug + size, capacity - size ) ) > 0 ) {
            size += r;
            if ( size + 1 >= capacity ) {
                capacity += 1024;
                char *resized = realloc( testDebug, capacity );
                if ( !resized )
                    break;
                testDebug = resized;
            }
        }
        testDebug[ size ] = '\0';

        r = close( debugPipe[ 0 ] );
        if ( r == -1 )
            testInternalError();

        r = close( pipefd[ 0 ] );
        if ( r == -1 )
            testInternalError();
    }
    else { // child
// TODO: check parent in other *NIX platforms
#if defined(__linux__)
        r = prctl( PR_SET_PDEATHSIG, SIGTERM );
        if ( r == -1 )
            testInternalError();
        if ( getppid() != parentPid )
            testExplicitError( EPERM );
#endif
        r = close( pipefd[ 0 ] ); // disable reading
        if ( r == -1 )
            testInternalError();

        r = close( debugPipe[ 0 ] ); // disable reading
        if ( r == -1 )
            testInternalError();

        testInfo()->pipeEnd = pipefd[ 1 ];
        testInfo()->debugPipe = debugPipe[ 1 ];
        testTmpOut = tmpfile();
        testTmpErr = tmpfile();
        dup2( fileno( testTmpOut ), 1 ); // redirect stdout
        dup2( fileno( testTmpErr ), 2 ); // redirect stderr
        run();

        testFinish();
    }
}
#endif

#if defined(_WIN32)

NORETURN static void testExecuteUnit( int id, int order ) {
    SetErrorMode( SEM_NOGPFAULTERRORBOX );
    testInfo()->name = testAll[ id ].name;
    testInfo()->order = order;
    testInfo()->id = id;
    testInfo()->failing = testAll[ id ].failing;
    testInfo()->result = testSuccess;
    testInfo()->errStatement = "";
    testInfo()->errFile = "";
    testInfo()->errLine = 0;

    testInfo()->pipeEnd = _dup( 1 );
    _setmode( testInfo()->pipeEnd, _O_BINARY );

    testInfo()->debugPipe = _dup( 2 );
    _setmode( testInfo()->debugPipe, _O_BINARY );

    testTmpOut = tmpfile();
    testTmpErr = tmpfile();
    _dup2( _fileno( testTmpOut ), 1 ); // redirect stdout
    _dup2( _fileno( testTmpErr ), 2 ); // redirect stderr
    testAll[ id ].run();

    testFinish();
}

static void testExecute( TestRunner run ) {
    (void)run;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof( saAttr );
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE childOutWrite, childOutRead;
    HANDLE childDebugWrite, childDebugRead;

    CreatePipe( &childOutRead, &childOutWrite, &saAttr, 0 );
    CreatePipe( &childDebugRead, &childDebugWrite, &saAttr, 0 );
    SetHandleInformation( childOutRead, HANDLE_FLAG_INHERIT, 0 );
    SetHandleInformation( childDebugRead, HANDLE_FLAG_INHERIT, 0 );

    PROCESS_INFORMATION procInfo;
    STARTUPINFOA startInfo;

    ZeroMemory( &procInfo, sizeof( procInfo ) );
    ZeroMemory( &startInfo, sizeof( startInfo ) );

    startInfo.cb = sizeof( startInfo );
    startInfo.dwFlags |= STARTF_USESTDHANDLES;
    startInfo.hStdOutput = childOutWrite;
    startInfo.hStdError = childDebugWrite;

    char *command = malloc( strlen( testProgramName ) + strlen( testSwitch ) + 16 );
    sprintf( command, "\"%s\" %s %i %i", testProgramName, testSwitch, testInfo()->id, testInfo()->order );

    if ( !CreateProcessA( testProgramName,
                          command,
                          NULL,
                          NULL,
                          TRUE,
                          CREATE_BREAKAWAY_FROM_JOB | CREATE_SUSPENDED,
                          NULL,
                          NULL,
                          &startInfo,
                          &procInfo ) ) {
        testInternalError();
    }
    if ( !AssignProcessToJobObject( jobGroup, procInfo.hProcess ) )
        testInternalError();
    if ( ResumeThread( procInfo.hThread ) != 1 )
        testInternalError();

    WaitForSingleObject( procInfo.hProcess, INFINITE );
    DWORD childResult;
    GetExitCodeProcess( procInfo.hProcess, &childResult );
    CloseHandle( procInfo.hProcess );
    CloseHandle( procInfo.hThread );

    if ( childResult ) {
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
    CloseHandle( childDebugWrite );
}
#endif

static int testRun( FILE *output, int argc, const char **argv ) {
    int failed = 0;
    int executed = 0;

#if defined(_WIN32)
    testProgramName = argv[ 0 ];

    for ( int i = 0; i < argc; ++i ) {
        if ( strcmp( argv[ i ], testSwitch ) == 0 && i + 2 < argc ) {
            int id = atoi( argv[ i + 1 ] );
            if ( id >= 0 && id < testIndex )
                testExecuteUnit( id, atoi( argv[ i + 2 ] ) );// noreturn
        }
    }

    // create a group of processes to be able to kill unit when parent dies
    jobGroup = CreateJobObject( NULL, NULL );
    if ( !jobGroup )
        testInternalError();
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if ( !SetInformationJobObject( jobGroup, JobObjectExtendedLimitInformation, &jeli, sizeof( jeli ) ) )
        testInternalError();

#endif

    testInfo()->order = 0;
    for ( int  i = 0; i < testIndex; ++i ) {

        if ( argc > 1 && !argMatch( testAll[ i ].name, argc, argv ) )
            continue;

        ++executed;

        testInfo()->name = testAll[ i ].name;
        testInfo()->order = executed;
        testInfo()->id = i;
        testInfo()->failing = testAll[ i ].failing;
        testInfo()->result = testSuccess;
        testInfo()->errStatement = "";
        testInfo()->errFile = "";
        testInfo()->errLine = 0;


        int base = fprintf( output, "[%3i] %s", testInfo()->order, testInfo()->name );
        fflush( output );
        testExecute( testAll[ i ].run );
        failed += printDefail( output, base );
    }

    fprintf( output,
            "\nSummary:\n"
            "  tests:     %3i\n"
            "  succeeded: %3i\n"
            "  skipped:   %3i\n"
            "  failed:    %3i\n",
            testIndex,
            executed - failed,
            testIndex - executed,
            failed
        );

    testCapacity = 0;
    testIndex = 0;
    free( testAll );
    return failed;
}

int main( int argc, const char **argv ) {
    return testRun( stdout, argc, argv );
}

#endif

#endif

#endif

