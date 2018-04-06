# cut - C Unit Tests

For now, it is just working version. It is missing support of Windows version (all compilers), documentation, automated tests, and much more. At least, the compiled program knows help using `--help` option.


## Targets

To work on platforms:

 * Unix
 * Mac
 * Windows

 To be compilable by

  * gcc
  * clang
  * msvc

## What it knows

It is a singles header c unit testing framework. By default each unit test is run as a separate process to avoid unwanted corrupted memory and to make every unit test independent to other tests. Furthemore it provides timeout functionality to avoid infinite loops in tests.

## How to use *cut*

First of all, `#include <cut.h>` into the place where you want to write unit tests. These are created using `TEST` macro. Use `ASSERT` to test values and `ASSERT_FILE` to test a content of a specified file (such as `stdout`).

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

To actually enable tests, you have to define either `CUT` or `DEBUG` to turn on testing features. There also need to be generated `main` function which si done when macro `CUT_MAIN` is defined (before `#include`).

```c
#define CUT_MAIN
#include <cut.h>
```

### Configuration options

Compile time macro directives:

 *  `CUT` -- turns on the framework
 *  `DEBUG` -- turns on the framework
 *  `CUT_MAIN` -- turns on the framework and generates `main` function
 *  `CUT_TIMEOUT` -- set timeout in seconds to a different value (default: 3)
 *  `CUT_NO_FORK` -- disable fork by default
 *  









