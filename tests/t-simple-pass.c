#include <cut.h>

TEST(success,
    TEST_POINTS(1))
{
    ASSERT(1);
}

TEST(suppressed, TEST_SUPPRESS) {
    ASSERT(0);
}