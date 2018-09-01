#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <ucontext.h>
#include <unordered_map>

#define STACK_SIZE (1024*1024)
//#define DEFAULT_COROUTINE 16

struct coroutine;

struct schedule {
#ifdef SHARED_STACK
    char stack[STACK_SIZE];
#endif
    ucontext_t main;
    //int nco;
    //int cap;
    int64_t running;
    int64_t next_id;
    //coroutine **co;
    std::unordered_map<int64_t, coroutine *> cos;
    coroutine *dead_co;
};

struct coroutine {
    coroutine_func func;
    void *ud;
    ucontext_t ctx;
    schedule *sch;
    ptrdiff_t cap;
    ptrdiff_t size;
    int status;
    char *stack;
};

coroutine *_co_new(schedule *S, coroutine_func func, void *ud) {
    coroutine *co = new coroutine();
    co->func = func;
    co->ud = ud;
    co->sch = S;
    co->cap = 0;
    co->size = 0;
    co->status = COROUTINE_READY;
#ifdef SHARED_STACK
    co->stack = NULL;
#else
    co->stack = (char *) malloc(STACK_SIZE);
#endif
    return co;
}

void _co_delete(coroutine *co) {
    free(co->stack);
    free(co);
}

schedule *coroutine_open(void) {
    schedule *S = new schedule();
    //S->nco = 0;
    //S->cap = DEFAULT_COROUTINE;
    S->running = -1;
    S->next_id = 0;
    //S->co = (coroutine **) malloc(sizeof(coroutine *) * S->cap);
    //memset(S->co, 0, sizeof(coroutine *) * S->cap);
    S->dead_co = NULL;
    return S;
}

void coroutine_close(schedule *S) {
#if 0
    int i;
    for (i = 0; i < S->cap; i++) {
        coroutine *co = S->co[i];
        if (co) {
            _co_delete(co);
        }
    }
#endif

    for (auto it = S->cos.begin(); it != S->cos.end(); /* NULL */) {
        coroutine *co = it->second;
        if (co) {
            _co_delete(co);
            it = S->cos.erase(it);
        }
    }

    //free(S->co);
    //S->co = NULL;
    free(S);
}

int64_t coroutine_new(schedule *S, coroutine_func func, void *ud) {
    int64_t id = S->next_id++;
    assert(S->cos.find(id) == S->cos.end());
    coroutine *co = _co_new(S, func, ud);
    S->cos.insert(std::make_pair(id, co));
    return id;
#if 0
    if (S->nco >= S->cap) {
        int id = S->cap;
        S->co = (coroutine **) realloc(S->co, S->cap * 2 * sizeof(coroutine *));
        memset(S->co + S->cap, 0, sizeof(coroutine *) * S->cap);
        S->co[S->cap] = co;
        S->cap *= 2;
        ++S->nco;
        return id;
    } else {
        int i;
        for (i = 0; i < S->cap; i++) {
            int id = (i + S->nco) % S->cap;
            if (S->co[id] == NULL) {
                S->co[id] = co;
                ++S->nco;
                return id;
            }
        }
    }
    assert(0);
    return -1;
#endif
}

static void mainfunc(uint32_t low32, uint32_t hi32) {
    uintptr_t ptr = (uintptr_t) low32 | ((uintptr_t) hi32 << 32);
    coroutine *C = (coroutine *) ptr;
    schedule *S = C->sch;
    int64_t id = S->running;
#if 0
    schedule *S = (schedule *)ptr;
    int id = S->running;
    coroutine *C = S->co[id];
#endif
    C->func(S, C->ud);
    //_co_delete(C);
    //S->co[id] = NULL;
    //--S->nco;
    S->dead_co = C;
    S->cos.erase(id);
    S->running = -1;
}

void coroutine_resume(schedule *S, int64_t id) {
    assert(S->running == -1);
    //assert(id >= 0 && id < S->cap);
    //coroutine *C = S->co[id];
    auto it = S->cos.find(id);
    assert(it != S->cos.end());
    coroutine *C = it->second;
    if (C == NULL)
        return;
    int status = C->status;
    switch (status) {
        case COROUTINE_READY: {
            getcontext(&C->ctx);
#ifdef SHARED_STACK
            C->ctx.uc_stack.ss_sp = S->stack;
#else
            C->ctx.uc_stack.ss_sp = C->stack;
#endif
            C->ctx.uc_stack.ss_size = STACK_SIZE;
            C->ctx.uc_link = &S->main;
            S->running = id;
            C->status = COROUTINE_RUNNING;
            //uintptr_t ptr = (uintptr_t)S;
            uintptr_t ptr = (uintptr_t) C;
            makecontext(&C->ctx, (void (*)(void)) mainfunc, 2, (uint32_t) ptr, (uint32_t)(ptr >> 32));
            swapcontext(&S->main, &C->ctx);
            break;
        }
        case COROUTINE_SUSPEND: {
#ifdef SHARED_STACK
            memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);
#endif
            S->running = id;
            C->status = COROUTINE_RUNNING;
            swapcontext(&S->main, &C->ctx);
            break;
        }
        default:
            assert(0);
    }

    if (S->dead_co) {
        _co_delete(S->dead_co);
        S->dead_co = NULL;
    }
}

#ifdef SHARED_STACK
static void _save_stack(coroutine *C, char *top) {
    char dummy = 0;
    assert(top - &dummy <= STACK_SIZE);
    if (C->cap < top - &dummy) {
        free(C->stack);
        C->cap = top-&dummy;
        C->stack = (char *)malloc(C->cap);
    }
    C->size = top - &dummy;
    memcpy(C->stack, &dummy, C->size);
}
#endif

void coroutine_yield(schedule *S) {
    int64_t id = S->running;
    assert(id >= 0);
    //coroutine *C = S->co[id];

    auto it = S->cos.find(id);
    assert(it != S->cos.end());
    coroutine *C = it->second;
#ifdef SHARED_STACK
    assert((char *)&C > S->stack);
    _save_stack(C,S->stack + STACK_SIZE);
#endif
    C->status = COROUTINE_SUSPEND;
    S->running = -1;
    swapcontext(&C->ctx, &S->main);
}

int coroutine_status(schedule *S, int64_t id) {
#if 0
    assert(id >= 0 && id < S->cap);
    if (S->co[id] == NULL) {
        return COROUTINE_DEAD;
    }

    return S->co[id]->status;
#endif
    auto it = S->cos.find(id);
    if (it == S->cos.end()) {
        return COROUTINE_DEAD;
    }

    return it->second->status;
}

int coroutine_running(schedule *S) {
    return S->running;
}
