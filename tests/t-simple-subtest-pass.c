#include <cut.h>

TEST(withSubtest) {
    int i = 1;
    SUBTEST("S1") {
        ASSERT(i == 1);
    }
    ++i;
    SUBTEST("S2") {
        ASSERT(i == 2);
    }
    ++i;
    ASSERT(i == 3);
}
