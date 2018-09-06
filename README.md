在云风的[coroutine库](https://blog.codingnow.com/2012/07/c_coroutine.html)的基础上，做了一些改动。
* 支持共享stack和非共享stack两种模式，根据编译条件进行选择。
* 对外隐藏`schedule`对象，每个线程拥有一个单例`schedule`静态对象，用户只需要管理协程`ID`即可。
* 将对`_co_delete`函数的调用从`mainfunc`移到了`coroutine_resume`末尾。即协程执行完毕，切换到`uc_link`指向的上下文之后才free掉coroutine对象。防止过早的释放`coroutine->ctx`对象造成core。
* 改用`unordered_map`管理coroutine对象，并且协程`ID`单调递增，新的协程`ID`不会复用已结束的老的协程`ID`。防止复用后，调用`coroutine_status`函数产生混乱。
* 使用C++11特性。

该协程库使用非对称形式，协程之间的调度统一由`schedule`对象负责。协程函数通过调用`coroutine_yield`把控制权交给`schedule`，后续`schedule`再通过调用`coroutine_resume`恢复协程的执行。

参考：
1. https://github.com/cloudwu/coroutine
2. https://github.com/tonbit/coroutine/blob/master/coroutine.h
3. https://github.com/tonbit
4. http://tecdump.blogspot.com/2012/07/coroutine.html