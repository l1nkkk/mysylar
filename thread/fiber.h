//
// Created by l1nkkk on 2021/5/28.
//

#ifndef MYSYLAR_FIBER_H
#define MYSYLAR_FIBER_H
#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

class Scheduler;
namespace l1nkkk {
class Fiber : public std::enable_shared_from_this<Fiber> {
    friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> sptr;
    // 协程状态
    enum State {
        INIT,   // 初始化（只出现再刚初始化完成）
        HOLD,   // 让出执行（还没执行完）
        EXEC,   // 执行中
        TERM,   // 结束
        READY,  // 准备好被调度
        EXCEPT  // 出错
    };
private:
    /**
     * @brief 主要用于在线程中创建主协程的过程中，被GetThis调用
     */
    Fiber();
public:
    /**
     * @brief 将函数包装成一个协程
     * @param cb
     * @param stacksize
     * @param use_caller
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    // 当协程执行完或者有问题的时候，可以将状态或者函数重置。利用原有协程的栈上内存做另外一个协程。
    // 重置协程函数，并设置状态
    // INIT， TERM
    void reset(std::function<void()> cb);


    void swapIn();  // 获得执行权（schedule），切出调度协程（线程主协程）
    void swapOut(); // 让出执行权（schedule），切到调度协程（线程主协程）
    void call();    // 获得执行权，切出主线程的主协程，use_caller时才会用
    void back();    // 让出执行权，切到主线程的主协程，use_caller时才会用
    uint64_t getID(){return m_id;}          // 获取协程ID
    State getState(){ return  m_state; }    // 获取协程状态

public:
    // 设置当前协程，方便协程函数将自己切出
    static void SetThis(Fiber* f);

    /**
     * @brief 获取当前协程；或者在当前的上下文中建立起一个主协程
     * @return 返回当前执行点协程
     */
    static Fiber::sptr GetThis();
    static void YieldToReady();     // 协程切换到主协程，并且设置为Ready状态
    static void YieldToHold();      // 协程切换到主协程，并且设置为Hold状态
    static uint64_t TotalFibers();  // 返回总协程数
    static void MainFunc();         // 核心：调度函数，非主线程调用
    static void CallerMainFunc();   // 核心：调度函数，主线程调用
    static uint64_t GetFiberId();   // 获取当前协程ID
private:
    uint64_t m_id = 0;          // 协程id
    uint64_t m_stacksize = 0;   // 栈大小
    State m_state = INIT;       // 协程状态
    ucontext_t m_ctx;           // 上下文
    void *m_stack = nullptr;    // 协程栈
    std::function<void()> m_cb; // 协程调用的函数
};
}
#endif //MYSYLAR_FIBER_H
