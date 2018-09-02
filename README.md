在云风的[coroutine库](https://github.com/cloudwu/coroutine)的基础上做了一些改进。
1. 支持共享stack和非共享stack两种模式，根据编译条件进行选择。
2. 修复了若干BUG，这些BUG包括：
    1. 在coroutine执行完成回到`uc_link`指向的上下文之前，将整个`ucontext_t`对象空间释放掉了，进程有可能会发生core。
    2. 旧的coroutine执行完毕，新的coroutine创建并复用了旧的`id`，那么调用`coroutine_status`将会产生一个不明确的语义。