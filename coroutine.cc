#include <string.h>
#include <assert.h>

#include "coroutine.h"

thread_local static schedule *S;


coroutine::coroutine(std::function<void(void*)> func, void *ud) {
    _callback = func;
    _arg = ud;
    sch = S;
    cap = 0;
    size = 0;
    status = COROUTINE_READY;
#ifdef SHARED_STACK
    stack = NULL;
#else
    stack = (char *) malloc(STACK_SIZE);
#endif
}


schedule::schedule() {
    running = -1;
    next_id = 0;
    dead_co = NULL;
}


coroutine *_co_new(std::function<void(void*)> func, void *ud) {
    coroutine *co = new coroutine(func, ud);
    return co;
}


void _co_delete(coroutine *co) {
    free(co->stack);
    delete co;
}


void coroutine_open(void) {
    S = new schedule();
}


void coroutine_close() {
    for (auto it = S->cos.begin(); it != S->cos.end(); /* NULL */) {
        coroutine *co = it->second;
        if (co) {
            _co_delete(co);
            it = S->cos.erase(it);
        }
    }
    delete S;
}


int64_t coroutine_new(std::function<void(void*)> func, void *ud) {
    int64_t id = S->next_id++;
    assert(S->cos.find(id) == S->cos.end());
    coroutine *co = _co_new(func, ud);
    S->cos.insert(std::make_pair(id, co));
    return id;
}


static void mainfunc(uint32_t low32, uint32_t hi32) {
    uintptr_t ptr = (uintptr_t) low32 | ((uintptr_t) hi32 << 32);
    coroutine *C = (coroutine *) ptr;
    schedule *S = C->sch;
    int64_t id = S->running;

    C->_callback(C->_arg);

    S->dead_co = C;
    S->cos.erase(id);
    S->running = -1;
}


void coroutine_resume(int64_t id) {
    assert(S->running == -1);
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


void coroutine_yield() {
    auto it = S->cos.find(S->running);
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


int coroutine_status(int64_t id) {
    assert(id >= 0);

    auto it = S->cos.find(id);
    if (it == S->cos.end()) {
        return COROUTINE_DEAD;
    }

    return it->second->status;
}


int64_t coroutine_running_id() {
    return S->running;
}

