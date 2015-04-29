#ifndef TESTING_H
#define TESTING_H

#ifndef TESTING

# define ASSERT( e ) (void)0
# define TEST( name ) static void unitTest_ ## name()
# define TEST_FAILING( name ) static void unitTest_ ## name()

#else

#if defined(__GNUC__) || defined(__clang)
# define NORETURN __attribute__((noreturn))
# define INLINE inline
#elif defined(_MSC_VER)
# define NORETURN __declspec(noreturn)
# define INLINE
#else
# define NORETURN
#endif

typedef void(*TestRunner)();

struct TestInfo;

void testRegister( TestRunner run, const char *name, int failing );
NORETURN void testFinish();
void testFillInfo( const char *stm, const char *file, int line );
struct TestInfo *testInfo();

#if defined(__GNUC__) || defined(__clang)

# define CONSTRUCTOR(name) __attribute__((constructor)) static void name()
#elif defined(_MSC_VER)
# pragma section(".CRT$XCU",read)
# define CONSTRUCTOR(name)                                                    \
    static void __cdecl name( void );                                         \
    __declspec(allocate(".CRT$XCU")) void (__cdecl * name ## _)(void) = name; \
    static void __cdecl name( void )

#else
# error "unsupported compiler"
#endif

# define ASSERT( e ) do { if ( !(e) ) {                                 \
        testFillInfo( #e, __FILE__, __LINE__ );                         \
        testFinish();                                                   \
    } } while( 0 )

# define TEST( name )                                                   \
    void unitTest_ ## name();                                           \
    CONSTRUCTOR( testRegister ## name ) {                               \
        testRegister( unitTest_ ## name, #name, 0 );                    \
    }                                                                   \
    void unitTest_ ## name()                            

# define TEST_FAILING( name )                                           \
    void unitTest_ ## name();                                           \
    CONSTRUCTOR( testRegister ## name ) {                               \
        testRegister( unitTest_ ## name, #name, 1 );                    \
    }                                                                   \
    void unitTest_ ## name()                            

#if defined( TESTING_MAIN )

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined( _WIN32 )
# include <Windows.h>
#elif defined( __unix ) || defined(__APPLE__)
# include <unistd.h>
# include <sys/wait.h>
# include <sys/types.h>
#else
# error "unsupported platform"
#endif

void testInternalError() {
#if defined(__unix) || defined(__APPLE__)
    perror( "Internal error occurred inside testing framework." );
#elif defined(_WIN32)
    char *msg;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (LPSTR)&msg,
        0, NULL );

    fprintf( stderr, "Internal error occurred inside testing framework.: %s\n", msg );
    LocalFree( msg );
#endif
    exit( -1 );
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
#if defined( _WIN32 )
    HANDLE pipeEnd;
#elif defined( __unix ) || defined(__APPLE__)
    int pipeEnd;
#endif
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
#if defined( _WIN32 )
static const char *testProgramName = NULL;
static const char *testSwitch = "--test-id";
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
            fprintf( stderr, "Internal error occurred inside testing framework." );
            exit( -1 );
        }
        testAll = tmp;
    }

    testAll[ testIndex ].run = run;
    testAll[ testIndex ].name = name;
    testAll[ testIndex ].failing = failing;
    ++testIndex;
}
#if defined(_WIN32)
NORETURN void testFinish() {
    int r;
    r = fwrite( testInfo(), sizeof( struct TestInfo ), 1, stdout );
    if ( r != 1 )
        testInternalError();
    free( testAll );
    ExitProcess( 0 );
}
#elif defined(__unix) || defined(__APPLE__)
NORETURN void testFinish() {
    int r;
    r = write( testInfo()->pipeEnd, testInfo(), sizeof( struct TestInfo ) );
    if ( r != sizeof( struct TestInfo ) )
        testInternalError();
    r = close( testInfo()->pipeEnd );
    if ( r == -1 )
        testInternalError();

    free( testAll );
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

static int printDefail( FILE *output ) {

    int failed = 0;
    int base = fprintf( output, "[%3i] %s", testInfo()->order, testInfo()->name );
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

    return failed;
}

#if defined( __unix) || defined(__APPLE__)
static void testExecute( TestRunner run ) {

    int r;
    int pipefd[ 2 ];
    r = pipe( pipefd );
    if ( r != 0 )
        testInternalError();
        
    int pid = fork();
    if ( r == -1 )
        testInternalError();

    if ( pid ) { // parent
        r = close( pipefd[ 1 ] ); // disable writing
        if ( r == -1 )
            testInternalError();

        int status = 0;
        r = waitpid( pid, &status, 0 );
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
        r = close( pipefd[ 0 ] );
        if ( r == -1 )
            testInternalError();
    }
    else { // child
        close( pipefd[ 0 ] ); // disable reading
        if ( r == -1 )
            testInternalError();

        testInfo()->pipeEnd = pipefd[ 1 ];
        run();

        testFinish();
    }
}
#endif

#if defined(_WIN32)

NORETURN static void testExecuteUnit( int id, int order ) {
    testInfo()->name = testAll[ id ].name;
    testInfo()->order = order;
    testInfo()->id = id;
    testInfo()->failing = testAll[ id ].failing;
    testInfo()->result = testSuccess;
    testInfo()->errStatement = "";
    testInfo()->errFile = "";
    testInfo()->errLine = 0;

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

    CreatePipe( &childOutRead, &childOutWrite, &saAttr, 0 );
    SetHandleInformation( childOutRead, HANDLE_FLAG_INHERIT, 0 );

    PROCESS_INFORMATION procInfo;
    STARTUPINFO startInfo;

    ZeroMemory( &procInfo, sizeof( procInfo ) );
    ZeroMemory( &startInfo, sizeof( startInfo ) );

    startInfo.cb = sizeof( startInfo );
    startInfo.dwFlags |= STARTF_USESTDHANDLES;
    startInfo.hStdOutput = childOutWrite;

    char *command = malloc( strlen( testProgramName ) + strlen( testSwitch ) + 16 );
    sprintf( command, "%s %s %i %i", testProgramName, testSwitch, testInfo()->id, testInfo()->order );

    if ( !CreateProcess(
        NULL,
        command,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &startInfo,
        &procInfo
        ) )
        testInternalError();

    WaitForSingleObject( procInfo.hProcess, INFINITE );
    unsigned long childResult;
    GetExitCodeProcess( procInfo.hProcess, &childResult );
    CloseHandle( procInfo.hProcess );
    CloseHandle( procInfo.hThread );

    if ( childResult ) {
        testInfo()->result = testAbnormal;
        testInfo()->returnCode = childResult;
        testInfo()->signal = 0;
    }
    else
        ReadFile( childOutRead, testInfo(), sizeof( struct TestInfo ), NULL, NULL );
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

        testExecute( testAll[ i ].run );
        failed += printDefail( output );
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

