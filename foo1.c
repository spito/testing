#include <stdlib.h>
#include "testing.h"

void foo() {
    ASSERT( 0 );
}

TEST( test1 ) {
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

TEST( test2 ) {
    ASSERT( 0 );
}

TEST( test_output ) {
    printf("blabla");
    testFile(stdout, "blabla");
    printf("x");
    testFile(stdout, "blablax");
}

TEST( test_output_fprinf ) {
    fprintf(stdout, "blabla");
    testFile(stdout, "blabla");
    fprintf(stdout, "x");
    testFile(stdout, "blablax");
}

TEST( test_error ) {
    printf("blabla");
    testFile(stderr, "blabla");
    printf("x");
    testFile(stderr, "blablax");
}

TEST( test_error_fprinf ) {
    fprintf(stderr, "blabla");
    testFile(stderr, "blabla");
    fprintf(stderr, "x");
    testFile(stderr, "blablax");
}

TEST_FAILING( test3 ) {
    foo();
}
