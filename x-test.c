#define CUT
#define CUT_MAIN

#include "core.h"

TEST(prvni) {
    ASSERT(1);
}

TEST(druhy) {
    SUBTEST(sub1) {
        ASSERT(1);
    }
    SUBTEST(sub2) {
        ASSERT(0);
    }
    SUBTEST(sub3) {
        ASSERT(1);
    }
}

TEST(treti) {
    ASSERT(0);
}

TEST(ctvrty) {
    DEBUG_MSG("co to je? asi %d", 6);
    int *i = 0;
    *i = 6;
}

TEST(paty) {
    while(1);
}
