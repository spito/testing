#include "cut.h"

#include <stdio.h>

TEST(input) {
    INPUT("6 5");
    int a;
    int b;

    ASSERT(2 == scanf("%d%d", &a, &b));
    ASSERT(a == 6);
    ASSERT(b == 5);
}