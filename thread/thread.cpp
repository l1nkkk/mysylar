//
// Created by l1nkkk on 4/19/21.
//

#include "thread.h"
#include <glog/logging.h>
#include "../common/util.h"
namespace l1nkkk{

static thread_local Thread* t_thread = nullptr; // 指向当前线程

static thread_local std::string t_threadName = "UNKNOW"; // 指向当前线程名称

Thread* Thread::GetThis(){
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_threadName;
}


void Thread::SetName(const std::string &name) {
    // 注：if预防再主线程中SetName的情况
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_threadName = name;
}


Thread::Thread(std::function<void()> cb, const std::string& name): m_cb(cb), m_name(name){
    if(name.empty()){
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        LOG(FATAL) << "pthread_create thread fail，rt" << rt << " name=" << name;
    }
    m_semaphore.wait(); // 注：确保线程对象如果创建成功返回了，那么线程一定是跑起来的了
}

Thread::~Thread() {
    if(m_thread){ // 注：如果没有被join过
        pthread_detach(m_thread);
    }
}

void Thread::join(){
    if(m_thread){
        int rt = pthread_join(m_thread, nullptr);
        if(rt){
            LOG(FATAL) << "pthread_join thread fail";
        }
        m_thread = 0;   // 注：重要的一步，避免重复join出错
    }
}

// 有点包装器的感觉，对一些环境和变量进行初始化
// arg 传的是thread this
void* Thread::run(void* arg){
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_threadName = thread->m_name;
    thread->m_id = l1nkkk::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str()); // 最大只支持16字符

    std::function<void()> cb;
    cb.swap(thread->m_cb);       // 重点2：让智能指针少个引用
    thread->m_semaphore.notify();   // 重点1：同步
    cb();
    return 0;
}

}