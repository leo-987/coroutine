#include "coroutine.h"
#include <stdio.h>

struct args {
    int n;
};

static void foo(void *ud) {
    args *arg = (args *) ud;
    int start = arg->n;
    for (int i = 0; i < 10000; i++) {
        printf("coroutine %d : %d\n", coroutine_running(), start + i);
        coroutine_yield();
    }
}

static void test() {
    args arg1 = {0};
    args arg2 = {100};
    args arg3 = {1000};
    args arg4 = {10000};

    int64_t co1 = coroutine_new(foo, &arg1);
    int64_t co2 = coroutine_new(foo, &arg2);
    int64_t co3 = coroutine_new(foo, &arg3);
    int64_t co4 = coroutine_new(foo, &arg4);

    printf("main start\n");
    while (coroutine_status(co1) && coroutine_status(co2) &&
           coroutine_status(co3) && coroutine_status(co4)) {
        coroutine_resume(co1);
        coroutine_resume(co2);
        coroutine_resume(co3);
        coroutine_resume(co4);
    }
    printf("main end\n");
}

int main() {
    coroutine_open();
    test();
    coroutine_close();
    return 0;
}

