#ifndef C_COROUTINE_H
#define C_COROUTINE_H

#include <stdint.h>
#include <ucontext.h>
#include <unordered_map>
#include <stddef.h>
#include <functional>

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

#define STACK_SIZE (1024*1024)

struct schedule;

struct coroutine {
    coroutine(std::function<void(void*)> func, void *arg);

    std::function<void(void*)> _callback;
    void *_arg;
    ucontext_t ctx;
    schedule *sch;
    ptrdiff_t cap;
    ptrdiff_t size;
    int status;
    char *stack;
};

struct schedule {
    schedule();

#ifdef SHARED_STACK
    char stack[STACK_SIZE];
#endif
    ucontext_t main;
    int64_t running;
    int64_t next_id;    // coroutine id generator
    std::unordered_map<int64_t, coroutine *> cos;
    coroutine *dead_co;
};

void coroutine_open();
void coroutine_close();
void coroutine_yield();
int64_t coroutine_running_id();
void coroutine_resume(int64_t id);
int coroutine_status(int64_t id);
int64_t coroutine_new(std::function<void(void*)>, void *ud);

#endif
