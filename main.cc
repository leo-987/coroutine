#include <stdio.h>
#include <deque>

#include "coroutine.h"


static void foo(void *ud) {
    int arg = *(int *)ud;

    for (int i = 0; i < arg; i++) {
        printf("coroutine %ld running\n", coroutine_running_id());
        coroutine_yield();
    }
}

static void test_1() {
    int arg1 = 1;
    int arg2 = 2;
    int arg3 = 3;
    int arg4 = 4;

    int64_t co1 = coroutine_new(foo, &arg1);
    int64_t co2 = coroutine_new(foo, &arg2);
    int64_t co3 = coroutine_new(foo, &arg3);
    int64_t co4 = coroutine_new(foo, &arg4);

    while (coroutine_status(co1) != COROUTINE_DEAD || coroutine_status(co2) != COROUTINE_DEAD ||
           coroutine_status(co3) != COROUTINE_DEAD || coroutine_status(co4) != COROUTINE_DEAD) {

        if (coroutine_status(co1) != COROUTINE_DEAD)
            coroutine_resume(co1);
        if (coroutine_status(co2) != COROUTINE_DEAD)
            coroutine_resume(co2);
        if (coroutine_status(co3) != COROUTINE_DEAD)
            coroutine_resume(co3);
        if (coroutine_status(co4) != COROUTINE_DEAD)
            coroutine_resume(co4);
    }
}

static void producer(void *arg) {
    std::deque<int> *q = (std::deque<int> *)arg;
    for (int i = 1; i < 10; i++) {
        q->push_back(i);
        printf("coroutine %ld, push data: %d\n", coroutine_running_id(), i);
        coroutine_yield();
    }
    q->push_back(0);
}

static void consumer(void *arg) {
    std::deque<int> *q = (std::deque<int> *)arg;
    while (true) {
        if (q->empty()) {
            coroutine_yield();
        }
        int data = q->front();
        if (data == 0)
            break;

        printf("coroutine %ld, pop data: %d\n", coroutine_running_id(), data);
        q->pop_front();

    }
}

static void test_2() {
    std::deque<int> q;
    int64_t co1 = coroutine_new(producer, &q);
    int64_t co2 = coroutine_new(consumer, &q);

    while (coroutine_status(co1) != COROUTINE_DEAD || coroutine_status(co2) != COROUTINE_DEAD) {
        if (coroutine_status(co1) != COROUTINE_DEAD)
            coroutine_resume(co1);
        if (coroutine_status(co2) != COROUTINE_DEAD)
            coroutine_resume(co2);
    }
}

int main() {
    coroutine_open();

    printf("-----main start\n");
    test_1();
    test_2();
    printf("-----main end\n");

    coroutine_close();
    return 0;
}

