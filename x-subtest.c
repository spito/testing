#include <stdio.h>

# define TEST(name)                                                             \
void name(int *cut_subtest, int cut_current)

# define SUBTEST(name)                                                          \
if (++*cut_subtest == cut_current)                                             \
    printf("running subtest %s\n", #name);                                                      \
if (*cut_subtest == cut_current)



TEST(foo) {

    puts("init");
    SUBTEST(jedna) {
        puts("xoxo");
    }
    SUBTEST(dva) {
        return;
        puts("chichi");
    }
    SUBTEST(tri) {
        puts("hoho");
    }


    puts("clearance");
}

int x(int a, int b) {
    return a > b ? a : b;
}


int main() {

    int subtests = 1;
    int counter = 0;
    for (int current = 1; current <= subtests; subtests = x(subtests, counter), ++current) {
        counter = 0;
        foo(&counter, current);
    }
    return 0;
}