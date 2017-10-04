#define CUT
#define CUT_MAIN

#include "core.h"
#include <stdexcept>

TEST(foo) {
    throw std::runtime_error("bla bla");
}
