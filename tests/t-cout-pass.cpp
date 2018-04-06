#include <cut.h>

#include <iostream>

TEST(cppCout) {
    std::cout << "hello" << std::endl;
    CHECK_FILE(stdout, "hello\n");
}