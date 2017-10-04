#include "fragments.h"
#include <stdio.h>

const char *hello = "Hello, friend";
const char *tractor = "Traktor";

static int bar();

int foo(int i) {
    return i + 1;
}

static int bar() {
    puts("xoxo");
    return 0;
}


int main() {

    struct cut_Fragment f;
    struct cut_Fragment other;
    cut_FragmentInit(&f, 14);
    cut_FragmentInit(&other, 0);


    cut_FragmentAddString(&f, hello);
    cut_FragmentAddString(&f, tractor);

    cut_FragmentSerialize(&f);

    other.serialized = malloc(f.serializedLength);
    memcpy(other.serialized, f.serialized, f.serializedLength);

    cut_FragmentDeserialize(&other);
    if (other.sliceCount != 2) {
        printf("invalid slice count: %d vs %d\n", other.sliceCount, f.sliceCount);
    }
    char *txt = cut_FragmentGet(&other, 0, NULL);
    if (txt) {
        printf("cmp 0: %s (%s)\n", strcmp(txt, hello) ? "no" : "yes", txt);
    }
    txt = cut_FragmentGet(&other, 1, NULL);
    if (txt) {
        printf("cmp 1: %s (%s)\n", strcmp(txt, tractor) ? "no" : "yes", txt);
    }

    cut_FragmentClean(&f);
    cut_FragmentClean(&other);

    cut_FragmentReceiveStatus status = CUT_FRAGMENT_RECEIVE_STATUS;

    foo(1) || bar();
    foo(-1) || bar();


    struct cut_Fragment empty;
    cut_FragmentInit(&empty, 10);
    cut_FragmentReserve(&empty, sizeof(int), NULL);
    cut_FragmentSerialize(&empty);

    char buf[500];
    memcpy(buf, empty.serialized, empty.serializedLength);

    cut_FragmentClean(&empty);

    return 0;
}
