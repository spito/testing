#ifndef CUT_DISABLED_H
#define CUT_DISABLED_H

#define ASSERT(e) (void)0
#define ASSERT_FILE(f, content) (void)0
#define ASSERT_FILE_BINARY(f, content) (void)0
#define CHECK(e) (void)0
#define CHECK_FILE(f, content) (void)0
#define CHECK_FILE_BINARY(f, content) (void)0
#define INPUT(content) (void)0
#define TEST(name, ...) static void unitTest_ ## name()
#define SUBTEST(name) if (0)
#define REPEATED_SUBTEST(name, count) if (0)
#define SUBTEST_NO 0
#define DEBUG_MSG(...) (void)0
#define TEST_POINTS(n)
#define TEST_TIMEOUT(n)
#define TEST_SUPPRESS
#define TEST_NEEDS(...)

#endif