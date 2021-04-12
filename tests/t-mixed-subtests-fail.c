#include "cut.h"

TEST(prvni) {
    SUBTEST("trololo") {
        DEBUG_MSG("%d", SUBTEST_NO);
    }

    REPEATED_SUBTEST("x", 5) {
        DEBUG_MSG("%d", SUBTEST_NO);
    }

    SUBTEST("chacha") {
        ASSERT(0);
    }
}

TEST(druhy) {
    REPEATED_SUBTEST("x", 2) {
        DEBUG_MSG("%d", SUBTEST_NO);
    }
 
    SUBTEST("trololo") {
        DEBUG_MSG("%d", SUBTEST_NO);
    }

    REPEATED_SUBTEST("x", 5) {
        DEBUG_MSG("%d", SUBTEST_NO);
    }
}

TEST(treti) {
    REPEATED_SUBTEST("fail", -1) {
        DEBUG_MSG("upsi");
    }
}

TEST(ctvrty) {
    REPEATED_SUBTEST("x", 2) {
        DEBUG_MSG("alfa %d", SUBTEST_NO);
    }

    REPEATED_SUBTEST("x", 2) {
        DEBUG_MSG("%d beta", SUBTEST_NO);
    }
}