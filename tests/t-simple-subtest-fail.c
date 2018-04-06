#include <cut.h>

TEST(withSubtest) {
    int i = 1;
    SUBTEST(S1) {
        CHECK(0);
        ASSERT(i == 1);
    }
    ++i;
    SUBTEST(S2 (failing)) {
        ASSERT(0);
    }
    ++i;
    SUBTEST(S3) {
        ASSERT(i == 3);
    }
    ++i;
    SUBTEST(S4) {
        int *n = NULL;
        *n = 4;
    }
    ++i;
    ASSERT(i == 5);
}
