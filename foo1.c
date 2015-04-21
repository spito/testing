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

TEST_FAILING( test3 ) {
    foo();
}
