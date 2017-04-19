#include <stdlib.h>
#include <stdio.h>
#include "testing.h"

void foo() {
    ASSERT( 0 );
}

TEST( test1_should_failed ) {
    DEBUG_MSG( "some debug text" );
    int a = 0;
    ASSERT( a + 1 == 0 );
}

TEST(sig1) {
    char *c = NULL;
    *c = 'c';
}

TEST_FAILING(sig2) {
    char *c = NULL;
    *c = 'c';
}

TEST( test2_should_failed ) {
    ASSERT( 0 );
}

TEST( test_output ) {
    printf("blabla");
    ASSERT_FILE(stdout, "blabla");
    printf("x");
    ASSERT_FILE(stdout, "blablax");
    DEBUG_MSG( "co to je? %d", 8 );
    DEBUG_MSG( "cislo to bylo" );
}

TEST( test_output_fprinf ) {
    fprintf(stdout, "blabla");
    ASSERT_FILE(stdout, "blabla");
    fprintf(stdout, "x");
    ASSERT_FILE(stdout, "blablax");
}

TEST( test_error ) {
    printf("blabla");
    ASSERT_FILE(stderr, "blabla");
    printf("x");
    ASSERT_FILE(stderr, "blablax");
}

TEST( test_error_fprinf ) {
    fprintf(stderr, "blabla");
    ASSERT_FILE(stderr, "blabla");
    fprintf(stderr, "x");
    ASSERT_FILE(stderr, "blablax");
}

TEST_FAILING( test3 ) {
    foo();
}
