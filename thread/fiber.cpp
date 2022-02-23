//
// Created by l1nkkk on 2021/5/28.
//

#include "fiber.h"
#include "glog/logging.h"
#include "../common/util.h"
#include "scheduler.h"
namespace l1nkkk{


static std::atomic<uint64_t> s_fiber_id{0};               // 当前协程ID
static std::atomic<uint64_t> s_fiber_count{0};            // 当前协程数
static thread_local Fiber* t_fiber = nullptr;               // 当前协程
static thread_local Fiber::sptr t_threadFiber = nullptr;    // master协程

class MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }
    static void Dealloc(void *vp,size_t size){
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if(t_fiber){
        return t_fiber->getID();
    }
    return 0;
}


Fiber::Fiber(){
    m_state  =EXEC;
    SetThis(this);
// 注：下面的代码作者写了，但我觉得没必要
//    if(getcontext(&m_ctx)){
//        CHECK(false) << "getcontext";
//    }
    ++s_fiber_count;
    LDLOG(INFO) << "Fiber::Fiber";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) : m_id(++s_fiber_id), m_cb(cb) {
    ++s_fiber_count;
    // 默认1MB
    m_stacksize = stacksize ? stacksize: 1024*1024;

    m_stack = StackAllocator::Alloc(m_stacksize);
    // 下面getcontext感觉没什么用
    if(getcontext(&m_ctx)){
        CHECK(false) << "getcontext";
    }
    m_ctx.uc_link = nullptr;                // 该上下文结束时，要恢复到的上下文
    m_ctx.uc_stack.ss_sp = m_stack;         // 栈
    m_ctx.uc_stack.ss_size = m_stacksize;   // 栈大小

    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else{
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    LDLOG(INFO) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        CHECK(m_state == TERM || m_state == INIT || m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else{
        // 没有栈说明是主协程
        CHECK(!m_cb);
        CHECK(m_state == EXEC);

        // 当前线程是否等于自己
        Fiber * cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    LDLOG(INFO) << "Fiber;:~Fiber id = " << m_id;
}

// 重置协程函数和状态
void Fiber::reset(std::function<void()> cb) {
    CHECK(m_stack);
    CHECK(m_state == TERM ||m_state == INIT || m_state == EXCEPT);
    m_cb = cb;
    if(getcontext(&m_ctx))
        CHECK(false) << "getcontext";
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

// 切换到当前协程执行.    从主协程 swap到当前协程
void Fiber::swapIn() {
    SetThis(this);
    CHECK(m_state != EXEC);
    m_state = EXEC;
//    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)){
    // 将当前的context保存到&Scheduler::GetMainFiber()->m_ctx，跳转到&m_ctx
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)){
        CHECK(false) << "swapcontext";
    }
}

// 切换到后台执行
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)){
        CHECK(false) << "swapcontext";
    }
}

// 从当前协程置换成目标执行协程
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    LLOG(INFO) << getID();
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        LCHECK(false) << "swapcontext";
    }
}

void Fiber::back() {
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)){
        CHECK(false) << "swapcontext";
    }
}


void Fiber::SetThis(Fiber *f) {
    t_fiber = f;
}

// 注：我的理解是，让调用这个方法的函数，升级为协程。协程与函数
// 主要用在将当前函数作为主协程
Fiber::sptr Fiber::GetThis() {
    if(t_fiber){
        return t_fiber->shared_from_this();
    }

    // 注：在这调用了私有的构造函数
    Fiber::sptr  main_fiber(new Fiber);
    CHECK(main_fiber.get() == t_fiber);
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::sptr  cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::sptr  cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
//    cur->back();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    // 如果不做 1 处的处理，会让协程对象一直存在协程栈中
    Fiber::sptr cur = GetThis();
    CHECK(cur);
    try{
        cur->m_cb();    // 调用协程函数
        // 为什么用设置回nullptr，因为我们用的functional,有可能里面有一些智能指针参数，这样可以让引用计数-1
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }catch (std::exception& ex){
        cur->m_state = EXCEPT;
        LOG(ERROR) << "Fiber Except" << ex.what();
    }catch (...){
        LOG(ERROR) << "Fiber Except" ;
    }

    // 1. 注：下面这几行好好体会，为了可以让当前协程被释放
    // 切出去之前先释放掉只能指针的引用，让当前协程的引用技术-1，避免到时候释放不掉，因为再也不会回到该协程了。
    auto rawPtr = cur.get();
    cur.reset();
    rawPtr->swapOut();
//    rawPtr->back();
    LCHECK(false) << "Never Reach";
}

// 要把主线程要拿进去调度的话，调用该函数
void Fiber::CallerMainFunc() {
    // 如果不做 1 处的处理，会让协程对象一直存在协程栈中
    Fiber::sptr cur = GetThis();
    CHECK(cur);
    try{
        cur->m_cb();
        // 为什么用设置回nullptr，因为我们用的functional,有可能里面有一些智能指针参数，这样可以让引用计数-1
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }catch (std::exception& ex){
        cur->m_state = EXCEPT;
        LOG(ERROR) << "Fiber Except" << ex.what();
    }catch (...){
        LOG(ERROR) << "Fiber Except" ;
    }

    // 1. 注：下面这几行好好体会，为了可以让当前协程被释放
    auto rawPtr = cur.get();
    cur.reset();
    rawPtr->back(); // 2. 注：与上面函数的唯一区别
    LCHECK(false) << "Never Reach" << std::to_string(rawPtr->getID());
}

}