#ifndef TESTING_H
#define TESTING_H

#ifndef TESTING

# define ASSERT( e ) (void)0
# define TEST( name ) static void unitTest_ ## name()
# define TEST_FAILING( name ) static void unitTest_ ## name()

#else

typedef void(*TestRunner)();

struct TestInfo;

void testRegister( TestRunner run, const char *name, int failing );
void testFinish();
void testFillInfo( const char *stm, const char *file, int line );
struct TestInfo *testInfo();

# define ASSERT( e ) do { if ( !(e) ) {                                 \
        testFillInfo( #e, __FILE__, __LINE__ );                         \
        testFinish();                                                   \
    } } while( 0 )

# define TEST( name )                                                   \
    void unitTest_ ## name();                                           \
    __attribute__((constructor)) static void testRegister ## name() {   \
        testRegister( unitTest_ ## name, #name, 0 );                    \
    }                                                                   \
    void unitTest_ ## name()                            

# define TEST_FAILING( name )                                           \
    void unitTest_ ## name();                                           \
    __attribute__((constructor)) static void testRegister ## name() {   \
        testRegister( unitTest_ ## name, #name, 1 );                    \
    }                                                                   \
    void unitTest_ ## name()                            

#if defined( TESTING_MAIN ) && defined( TESTING_NO_MAIN )
# error "Cannot specify both TESTING_MAIN and TESTING_NO_MAIN."
#elif defined( TESTING_MAIN ) || defined( TESTING_NO_MAIN )

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

void testInternalError() {
    perror( "Internal error occurred inside testing framework." );
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
    int failing;
    enum TestResult result;
    int signal;
    int returnCode;
    int pipeEnd;
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

__attribute__((noreturn)) void testFinish() {
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

static inline void fill( FILE *output, int n ) {
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

static int testRun( FILE *output, int argc, const char **argv ) {
    int failed = 0;
    int executed = 0;

    testInfo()->order = 0;
    for ( int  i = 0; i < testIndex; ++i ) {

        if ( argc > 1 && !argMatch( testAll[ i ].name, argc, argv ) )
            continue;

        ++executed;

        testInfo()->name = testAll[ i ].name;
        testInfo()->order = executed;
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

#endif

#if defined( TESTING_MAIN )
int main( int argc, const char **argv ) {
    return testRun( stdout, argc, argv );
}
#endif

#endif

#endif

