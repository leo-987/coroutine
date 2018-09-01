#ifndef C_COROUTINE_H
#define C_COROUTINE_H

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct schedule;

typedef void (*coroutine_func)(schedule *, void *ud);

schedule *coroutine_open(void);
void coroutine_close(schedule *);

int coroutine_new(schedule *, coroutine_func, void *ud);
void coroutine_resume(schedule *, int id);
int coroutine_status(schedule *, int id);
int coroutine_running(schedule *);
void coroutine_yield(schedule *);

#endif
