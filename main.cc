#include "coroutine.h"
#include <stdio.h>

struct args {
    int n;
};

static void foo(schedule *S, void *ud) {
    args *arg = (args *) ud;
    int start = arg->n;
    int i;
    for (i = 0; i < 10000; i++) {
        printf("coroutine %d : %d\n", coroutine_running(S), start + i);
        coroutine_yield(S);
    }
}

static void test(schedule *S) {
    args arg1 = {0};
    args arg2 = {100};
    args arg3 = {1000};
    args arg4 = {10000};

    int co1 = coroutine_new(S, foo, &arg1);
    int co2 = coroutine_new(S, foo, &arg2);
    int co3 = coroutine_new(S, foo, &arg3);
    int co4 = coroutine_new(S, foo, &arg4);
    printf("main start\n");
    while (coroutine_status(S, co1) && coroutine_status(S, co2) &&
           coroutine_status(S, co3) && coroutine_status(S, co4)) {
        coroutine_resume(S, co1);
        coroutine_resume(S, co2);
        coroutine_resume(S, co3);
        coroutine_resume(S, co4);
    }
    printf("main end\n");
}

int main() {
    schedule *S = coroutine_open();
    test(S);
    coroutine_close(S);

    return 0;
}

