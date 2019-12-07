#include <cut.h>


TEST(A, TEST_SETTINGS(), TEST_NEEDS("Second", "Third")) {

}


TEST(First) {
    DEBUG_MSG("First");
}

TEST(Second, TEST_SETTINGS(), TEST_NEEDS("First")) {
    DEBUG_MSG("Second");
}

TEST(Third, TEST_SETTINGS(), TEST_NEEDS("Second")) {
    DEBUG_MSG("Th\"ird");
    CHECK(0);
}

TEST(B, TEST_SETTINGS(), TEST_NEEDS("A")) {

}
