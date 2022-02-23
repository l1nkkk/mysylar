//
// Created by l1nkkk on 2021/5/29.
//

#ifndef MYSYLAR_SCHEDULER_H
#define MYSYLAR_SCHEDULER_H
#include <memory>
#include <vector>
#include <list>
#include <string>
#include <map>
#include "fiber.h"
#include "mutex.h"
#include "thread.h"

namespace l1nkkk {
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> sptr;
    typedef Mutex MutexType;

    /**
     * @brief 情况1：将当前线程一起被调度。创建并设置主协程，设置主线程，设置当前线程的调度器；
     * @brief 情况2：不将当前线程一起被调度。
     * @param thsNum
     * @param use_caller
     * @param name
     */
    Scheduler(size_t thsNum = 1, bool use_caller = true, const std::string name = "MainThread");
    virtual ~Scheduler();

    /**
     * @brief 获取调度器名称
     * @return
     */
    const std::string & getName() const { return m_name;}
    /**
     * @brief 获取当前调度器
     * @return
     */
    static Scheduler* GetThis();    // 获取当前的调度器

    /**
     * @brief 获取调度器的主协程
     * @return
     */
    static Fiber* GetMainFiber();   // 获取调度器的主协程。跟fiber.h中线程的主协程不太一样。

    /**
     * @brief 创建线程池,创建多个线程。每个线程执行Scheduler::run
     */
    void start();   // 启动线程池
    void stop();    // 停止线程池

    /**
     * @brief 把 协程/函数对象 添加到待调度队列里
     * @tparam FiberOrCb
     * @param fc
     * @param thread
     */
    template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_tickle = false;
        {
            MutexType::Lock  lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if(need_tickle){
            tickle();
        }
    }

    /**
     * @brief 批量添加到待调度队列里
     * @tparam InputIterator
     * @param begin
     * @param end
     */
    template <class InputIterator>
    void schedule(InputIterator begin, InputIterator end){
        bool need_tickle = false;
        {
            MutexType::Lock  lock(m_mutex);
            while(begin != end){
                // 注：调用的是swap的那个FiberAndThread构造
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
            }
        }
        if(need_tickle){
            // 相当于通知
            tickle();
        }
    }

private:
    /**
     * 无锁的schedule
     * 返回true相当于以前没有任何任务，可能所有的线程都陷入内核态，比如wait某个信号量
     * 当放进去一个，就需要唤醒线程，通知任务来了，比如从协程队列中取出要执行的协程
     */
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int tid){
        bool need_tckle = m_fibers.empty();
        FiberAndThread ft(fc, tid);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        return need_tckle;
    }
protected:
    virtual void tickle();
    virtual bool stopping();
    virtual void idle(); // idle 协程。没任务做了就该执行idle
    void run();

    void setThis();
    bool hasIdelThreads(){ return m_idelThreadCount > 0;}  // 是否有闲置的线程

private:
    // m_fibers并不是一直都是Fiber对象，下面自定义一个类型，另其作为执行的参数（被调度执行的对象）
    // 如协程Fiber和Functional
    struct FiberAndThread{
        Fiber::sptr fiber;          // 协程
        std::function<void()> cb;   // 回调的function
        int threadID;               // 线程ID，可以指定这个对象给哪个线程执行

        FiberAndThread(Fiber::sptr f, int thr): fiber(f),threadID(thr){}
        // 为什么要有这个智能指针的指针这种构造.因为可能要将其swap。
        // 好处：将用户代码中的Fiber引用释放了，避免在析构的时候，释放不掉
        FiberAndThread(Fiber::sptr *f, int thr): threadID(thr){
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr): cb(f),threadID(thr){}
        FiberAndThread(std::function<void()> *f, int thr): threadID(thr){
            cb.swap(*f);
        }

        // 置于容器中时需要默认构造函数
        FiberAndThread(): threadID(-1){
        }

        void reset(){
            fiber = nullptr;
            cb = nullptr;
            threadID = -1;
        }
    };
private:
    MutexType m_mutex;                      // 锁
    std::vector<Thread::sptr> m_threads;    // 线程池
    std::list<FiberAndThread> m_fibers;     // 等待被调度的协程
//    std::map<int, std::list<FiberAndThread> >
    Fiber::sptr m_rootFiber;                // 主协程

protected:  // 可能被继承类使用
    std::vector<int> m_threadIds;                         // 所有的线程ID
    size_t m_threadCount = 0;                             // 所有线程数
    std::atomic<size_t> m_activeThreadCount = {0};      // 活跃线程数
    std::atomic<size_t> m_idelThreadCount = {0};        // 空闲线程数
    bool m_stopping = true;                               // 执行状态，是否停止
    bool m_autoStop = false;                              // 是否主动停止
    int64_t m_rootThread = 0;                             // 主线程的ID，use_caller的ID
    std::string m_name;                                   // 调度器名称
};
}

#endif //MYSYLAR_SCHEDULER_H
