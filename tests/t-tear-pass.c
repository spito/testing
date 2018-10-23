#include <cut.h>

int g = 0;

GLOBAL_TEAR_UP() {
    g = 10;
}

TEST(g) {
    ASSERT(g == 10);
}
