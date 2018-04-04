#include <cut.h>
#include <stdexcept>

TEST(throwFail) {
    throw std::runtime_error("expected to fail");
}