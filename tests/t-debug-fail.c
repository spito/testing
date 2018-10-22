#include <cut.h>

TEST(debugFail) {
    DEBUG_MSG("Line: %d", __LINE__);
    ASSERT(0);
    DEBUG_MSG("no fear i am here");
}
