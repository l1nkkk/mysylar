

# thread
## 关于pthread和std::thread
- c++11之前一般都用pthread_xx
- c++11之后自带线程，实际实现也是通过pthread来做的，实际链接还是需要pthread。std::thread没有分互斥量没有分读写锁，在高并发时，可能不是很适合

## 线程局部变量thread_local
相当于每个线程都会有一个实例
- https://murphypei.github.io/blog/2020/02/thread-local
- Thread的构造函数中，我为什么不直接去调用用户提供的函数呢？？？
  - 原因：因为我需要再创建线程后先做一些初始化好环境的操作，如果交给用户提供的代码则办不到。

# 锁
- spinlock： 
  - 如果没获得锁，他在cpu里不会马上陷入内核态，只是再cpu上空跑一段时间
  - 所以CPU消耗要高点，但是锁的等待可能少很多
  - 往往针对冲突多、冲突时间比较短，效率可能好一些.
  
# 协程模块
- 协程需要自己**主动**让出CPU。
- 一个线程同一时刻只能有一个协程在跑
- 函数------>协程。（感觉有时候可以想成，协程就是一个可以切出来的函数）
- 每个线程里有一个main协程 `Thead--->main_fiber`，所有的协程交换都跟该main_fiber协程打交道。（这样写起来感觉好实现点）
- 在协程栈里面，不要最后遗留自己的智能指针，否则程序退出时，无法正常释放
```cpp
class Fiber : public std::enable_shared_from_this<Fiber> {
    friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> sptr;
    // 协程状态
    enum State {
        INIT,   // 只出现刚初始化完成
        HOLD,   // 让出执行,还没执行完
        EXEC,   // 正在执行
        TERM,   // 结束
        READY,  // 准备好被调度了
        EXCEPT  // 出错了
    };
private:
    // 注意，这里是私有的
    Fiber();
public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    // 当协程执行完或者有问题的时候，可以将状态或者函数重置。利用原有协程的栈上内存做另外一个协程。
    // 重置协程函数，并设置状态
    // INIT， TERM
    void reset(std::function<void()> cb);

    
    void swapIn();  // 获得执行权（schedule）
    void swapOut(); // 让出执行权（schedule）
    void call();    // 获得执行权
    void back();    // 让出执行权
    uint64_t getID(){return m_id;}
    State getState(){ return  m_state; }

public:
    // 设置当前协程，方便协程函数将自己切出
    static void SetThis(Fiber* f);

    /**
    * 返回当前执行点协程
    * 如果发现没有的话，表明当前线程还没有使用协程，则创建一个主协程
    */
    static Fiber::sptr GetThis();
    static void YieldToReady(); // 协程切换到主协程，并且设置为Ready状态
    static void YieldToHold(); //  协程切换到主协程，并且设置为Hold状态
    static uint64_t TotalFibers(); // 总协程数
    static void MainFunc();
    static void CallerMainFunc();
    static uint64_t GetFiberId();
private:
    uint64_t m_id = 0;          // 协程id
    uint64_t m_stacksize = 0;   // 栈大小
    State m_state = INIT;       // 协程状态
    ucontext_t m_ctx;           // 上下文
    void *m_stack = nullptr;    // 协程栈
    std::function<void()> m_cb; // 协程调用的函数
};
```



# 协程调度



- 区分协程与函数（本质同理）
```
        1   -   N   1  -   M  
 scheduler --> thread --> fiber
 N ： M # n个线程对应m个协程
```
- scheduler相当于线程池，一个scheduler对应N个线程
- 一个线程对应M个协程
- 线程和协程是N对M的结构
- 协程可以再线程间自由切换

### schedule
1. 是一个线程池，分配一组线程
2. 是一个协程调度器，将协程指定到相应的线程上去执行
   - 两种方式：
     - 一种是随机选择一个空闲的线程
     - 一种是协程指定绑定特定线程执行

> 主要三个函数
```
start()
# 如果要实现比较优雅的话，stop要等到所有协程执行完才退出。那么就要确定所有协程执行完是哪些状态。
stop()


run()
1. 设置当前线程的scheduler
2. 设置当前线程的run，fiber（执行哪个fiber）
3. 协程调度循环while(true)
    1. 协程消息队里里面是否有任务
    2. 无任务执行，执行idle
```


# IO协程点都
- 基于schedule重载Idle的方法
```
IOManager(epoll)---->Schedule
    |
    |
    |
    v
    (epollwait)idle
```

# 问题

- thread中需要log，如何在cmake中指定