//
// Created by l1nkkk on 2021/5/29.
//

#include "scheduler.h"
#include "../common/util.h"
#include <glog/logging.h>


namespace l1nkkk{

static thread_local Scheduler* t_scheduler = nullptr;   // 当前线程调度器指针——多个线程对应一个
static thread_local Fiber* st_fiber = nullptr;          // 主协程（调度协程）——每个线程对应一个


Scheduler::Scheduler(size_t thsNum, bool use_caller, const std::string name) : m_name(name){
    CHECK(thsNum > 0);
    if(use_caller) {
        /// 1.创建主协程
        l1nkkk::Fiber::GetThis();   // 注：如果这个线程没有协程，会初始化主协程
        --thsNum;                   // 注：因为使用了当前线程，就可以少分配一个线程了

        /// 2.创建调度器
        CHECK(GetThis() == nullptr);// 注：防止一个线程存在多个协程调度器，防止在一个在一个线程中多次创建调度器
        t_scheduler = this;

        /// 3.设置一些标志（主线程以及主线程中的主协程）。标志是use_caller为true的一些特征。
        // 注：创建一个协程来做调度（主协程）,但是还没被执行起来。
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        st_fiber = m_rootFiber.get(); // 注：设置当前线程的主协程（负责调度其他协程，后面被call切入）

        l1nkkk::Thread::SetName(m_name);        // 体现SetName的必要性，因为主线程不是我们创建的，只能通过这种方式命名
        m_rootThread = l1nkkk::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else{
        // 注：代表use_caller为false
        m_rootThread = -1;
    }
    m_threadCount = thsNum;
}

Scheduler::~Scheduler() {
    CHECK(m_stopping);
    if(GetThis() == this){
        t_scheduler == nullptr;
    }
}


Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}
Fiber * Scheduler::GetMainFiber() {
    return st_fiber;
}


void Scheduler::start() {
    MutexType::Lock  lock(m_mutex);
    if(!m_stopping){  // 注：说明已经启动
        return;
    }
    m_stopping  = false;

    CHECK(m_threads.empty()); // 注：还没开始，线程池肯定为空

    m_threads.resize(m_threadCount);
    for(auto i = 0; i <m_threadCount; ++i){
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                      m_name + "_" + std::to_string(i) ));
        m_threadIds.push_back(m_threads[i]->getID()); // 注：体现线程类加信号量的目的，不然id可能都没初始化好就返回了。
    }
    lock.unlock();

//    if(m_rootFiber){ // 注：如果当前线程也加进去调度的话，是调用call而不是swapIn。swapIn用于调度中的切换
//        m_rootFiber->call();
//        LLOG(INFO) << "call out " << m_rootFiber->getState();
//    }
}

/**
 * 作用：等待协程调度器所有任务完成后退出
 **/
void Scheduler::stop() {
    m_autoStop = true;

    /// 情况1：没使用了usercaller
    // 用了usercaller（m_rootFiber != -1），并且只有一个线程的情况(m_threadCount == 0)
    if(m_rootFiber  // 问题2：没有用usercaller的时候不是m_rootFiber = -1的吗，这里为什么用0去判断
            && m_threadCount == 0   // 问题1：m_threadCount没有--的地方，永远不会触发吧?
            && ((m_rootFiber->getState() == Fiber::TERM
                ||m_rootFiber->getState() == Fiber::INIT ))){
        LLOG(INFO) << this << " stopped";
        m_stopping = true;

        // 交给子类去定义stopping
        if(stopping()){
            return;
        }
    }

    /// 情况2：使用了usecaller
    // 当schedule把创建它的线程也使用上去的时候（即用了usecaller）
    // stop必须在那个创建它的线程上执行
    if(m_rootThread != -1){
        // 说明是user_caller的线程
        CHECK(GetThis() == this);
        // 当Scheduler要把创建它的线程也使用的时候，stop也得再创建它的线程执行
    } else{
        // 在非usecall的线程执行stop,可以在任何非它的线程里，执行stop
        CHECK(GetThis() != this);
    }

    // 有多少个run就需要多少个tickle
    // 每个线程tickle 主线程tickle
    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i){
        // 类似于信号量，tickle一下就可以唤醒，使线程唤醒，让他们结束
        // 为什么不用唤醒user_call
        tickle();
    }
    if(m_rootFiber) {
        tickle();
    }

    if(m_rootFiber) {
        if(!stopping()){
            m_rootFiber->call();
        }
    }

    // 注：下面处理多线程情况
    std::vector<Thread::sptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    LLOG(INFO) << "run";

    // 初始化当前线程的schedule
    setThis();
    if(l1nkkk::GetThreadId() != m_rootThread){
        st_fiber = Fiber::GetThis().get();
    }

    // 空闲的时候执行，协程调度器没有任务，又不能使协程终止的时候
    // 具体怎么做让实现子类来做
    Fiber::sptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::sptr cb_fiber;   // 回调函数的协程

    FiberAndThread ft;
    while(true){
        ft.reset();
        bool tickle_me = false;
        bool is_active = false; // 注：说明是否有任务可做

        /** 寻找一个可以被执行的协程：start*/
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            // 协程消息队列
            while (it != m_fibers.end()) {
                if(it->threadID != -1 && it->threadID != l1nkkk::GetThreadId()){
                    // 该协程指定了执行线程，且当前线程不是其所指定的
                    ++it;
                    // 自己处理不了，通知其他线程处理
                    tickle_me = true;
                    continue;
                }
                CHECK(it->fiber || it->cb);
                // 如果正在处理中（感觉不存在这种情况，因为每次拿了都erase掉）
                if(it->fiber && it->fiber->getState() == Fiber::EXEC){
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it);
                ++m_idelThreadCount;
                is_active = true;
                break;
            }
        }
        /** 寻找一个可以被执行的协程 ：end*/

        // 唤醒其他线程
        if(tickle_me)
            tickle();

        /** 执行线程*/
        // 如果是协程有值，且还没有运行结束或者出错
        if(ft.fiber && ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT){

            /** 主协程让出线程：start*/
            ft.fiber->swapIn();
            --m_activeThreadCount;
            /** 主协程让出线程：end*/

            if(ft.fiber->getState() == Fiber::READY){
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT){
                ft.fiber->m_state = Fiber::HOLD;
            }
            // 智能指针释放
            ft.reset();
        } else if(ft.cb){
            // 如果是callback有值，下面两个reset：一个是fiber本身的成员，一个是智能指针的方法
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();

            // 切入，执行调度
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY){
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM){
                cb_fiber->reset(nullptr);
            } else {//if(cb_fiber->getState() != Fiber::TERM){
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else{
            if(is_active){
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                LLOG(INFO) << "idle fiber term";
                break;  // idel执行完结束
            }
            // 注：原子操作
            idle_fiber->swapIn();
            --m_idelThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
               && idle_fiber->getState() != Fiber::EXCEPT){
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle(){
    LLOG(INFO) <<"tickle";
}

// 判断是否没有任务了
bool Scheduler::stopping(){
//    LLOG(INFO) <<"stopping";
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}
void Scheduler::idle(){
    LLOG(INFO) <<"idle";
    while(!stopping()) {
        l1nkkk::Fiber::YieldToHold();
    }
}

}
