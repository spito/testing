[![Build Status](https://travis-ci.org/spito/testing.svg?branch=master)](https://travis-ci.org/spito/testing)

# _CUT_ - C Unit Tests

_CUT_ - C unit testing is a singles header testing framework. By default each unit test is run as a separate process to avoid unwanted corrupted memory and to make every unit test independent to other tests. Furthemore, it provides timeout functionality to avoid infinite loops in tests. It can be compiled as C++, in such case it can also catch exceptions.

It is multiplatform. It works on Linux, OSX, and Windows and it can be compiled by GCC, clang, and MSVC.

## How to use _CUT_

First of all, `#include <cut.h>` into the place where you want to write unit tests. These are created using `TEST` macro. Use `ASSERT`/`CHECK` to test values and `ASSERT_FILE`/`CHECK_FILE` to test a content of a specified file (such as `stdout`). The difference between then is that `ASSERT` stops a test when it fails while `CHECK` remembers the failure and continues on.

```c
#include <cut.h>

TEST(plus) {
    ASSERT(1 + 1 == 2);
}

TEST(output) {
    printf("value: %d", 6);
    ASSERT_FILE(stdout, "value: 6");
}
```

To actually enable tests, you have to either `#define CUT` or `#define DEBUG` to turn on testing features. There also need to generate `main` function which si done by defining `CUT_MAIN` macro (before `#include`). If no such enabling macro exists, every macro defined by _CUT_ is turned into harmless no-op.

```c
#define CUT_MAIN
#include <cut.h>
```

> Note: when defining `CUT_MAIN`, place `#include <cut.h>` before any other standard `#include`. _CUT_ needs to `#define _POSIX_C_SOURCE 199309L` and `#define _XOPEN_SOURCE 500` to handle signals properly.


### Configuration options

Compile time macro directives:

 *  `CUT` - Turn on the framework.
 *  `DEBUG` - Turn on the framework.
 *  `CUT_MAIN` - Turn on the framework and generates `main` function.
 *  `CUT_TIMEOUT` - Set timeout in seconds to a different value (default: 3).
 *  `CUT_NO_FORK` - Disable fork by default.
 *  `CUT_NO_COLOR` - Turn off colors.

Runtime configuration is done via command line arguments. Arguments are used to filter unit tests by their names. For example if arguments are _"ab"_ and _"c"_, only test whose names contains these substrings are executed while the rest of the tests are skipped. Additionally, there are few arguments that have different meaning:

 * `--help` - Print a help.
 * `--timeout <N>` - Set timeout of each test in seconds. 0 for no timeout. Overrides `CUT_TIMEOUT` value.
 * `--no-fork` - Disable forking. Timeout is turned off.
 * `--fork` - Force forking. Usefull during debugging with fork enabled. Overrides `CUT_NO_FORK`.
 * `--no-color` - Turn off colors.
 * `--output <file>` - Redirect output to the file.
 * `--short-path <N>` - Make filenames in the output (reporting checks, asserts, debug messages) shorter.

### Provided macros

 * `TEST(name)` - Defines test and its name.
 * `SUBTEST(name)` - Defines subtest within the test. Each subtest is executed separately and eventualy in its own process.
 * `REPEATED_SUBTEST(name, count)` - Defines subtest which is run `count`-times. Do not mix with the `SUBTEST()` in the same `TEST()`.
 * `SUBTEST_NO` - A number of current subtest iteration in the `REPEATED_SUBTEST()`.
 * `ASSERT(condition)` - Check if the condition is non-zero. If not, aborts the test.
 * `CHECK(condition)` - Check if the condition is non-zero. If not, reports it and continues.
 * `ASSERT_FILE(file, content)` - Check if the content of the `file` equals to the `content`. If not, aborts the test. The type of `file` should be `FILE *` and such file has to be opened for reading. It is possible to check even `stdout` and `stderr`. 
 * `CHECK_FILE(file, content)` - Same as the previous except it does not aborts the test.
 * `DEBUG_MSG(fmt, ...)` - Write a debug message. Use printf-like formatting.
 * `GLOBAL_TEAR_UP()` - Defines a function executed before each test/subtest.
 * `GLOBAL_TEAR_DOWN()` - Defines a function executed after each test/subtest even in case of assert failure or uncaught exception. The function is not executed in case of abnormal termination of test.

When `SUBTEST(name)` or `REPEATED_SUBTEST(name, count)` is used, the whole test is run several times. The first run does not execute any of subtest, its purpose is to figure out how many subtests are in the test. The subsequent executions will run subtests one by one, eventualy each in its own process.

Note: all asserting and checking macros can be used deeper in the stack, not just in the top level test function.

 ### Example

```c
TEST(first) {
    int value = magicFunction(0);
    CHECK(value > 0);
    int value2 = magicFunction(value);
    ASSERT(value2 > value);
}

TEST(second) {
    prepareMagic();
    REPEATED_SUBTEST(magic, 6) {
        int value = magicFunction(SUBTEST_NO);
        DEBUG_MSG("The magic value is %d", value);
        ASSERT(value > SUBTEST_NO);
    }
    cleanMagic();
}

TEST(third) {
    prepareMagic();
    SUBTEST(dark) {
        int value = darkMagic();
        ASSERT(value == -1);
    }
    SUBTEST(light) {
        int value = lightMagic();
        ASSERT(value == 1);
    }
    cleanMagic();
}

TEST(forth) {
    saySpell(stdout);
    CHECK_FILE(stdout, "secret spell");
    CHECK_FILE(stderr, "");
}
```
