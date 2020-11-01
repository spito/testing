#include <cut.h>

TEST(repeated) {
    ASSERT(1);

    REPEATED_SUBTEST("some name", 6) {
        ASSERT(SUBTEST_NO);
    }

    ASSERT(1);
}
