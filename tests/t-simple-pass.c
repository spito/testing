#include <cut.h>

TEST(success,
    TEST_SETTINGS(TEST_POINTS(1)))
{
    ASSERT(1);
}

TEST(suppressed, TEST_SETTINGS(TEST_SUPPRESS)) {
    ASSERT(0);
}