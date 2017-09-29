#define CUT
#define CUT_MAIN

#include "core.h"

#include <fcntl.h>

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
    DEBUG_MSG("tohle jeste uvidim");
    *i = 6;
    DEBUG_MSG("ale tohle uz ne :(");
}

TEST(paty) {
    while(1);
}


TEST(sesty) {
    printf("cislo %d", 6);

    ASSERT_FILE(stdout, "cislo 6");
}

TEST(osm) {
    open("W", O_WRONLY | O_CREAT, 0666);
    exit(10);
}
