//
// Created by l1nkkk on 4/19/21.
//

#ifndef MYSYLAR_THREAD_H
#define MYSYLAR_THREAD_H

#include <pthread.h>
#include <functional>
#include <memory>
#include <string>
#include <semaphore.h>
#include "mutex.h"

namespace l1nkkk {

class Thread {
public:
    typedef std::shared_ptr<Thread> sptr;
    /**
     * @brief 创建线程（立即执行，内部调用 static void* run(void* arg)），并设置成员变量（线程id等）
     * @param cb
     * @param name
     */
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();
    pid_t getID() const { return m_id; };                   // 获取线程id
    const std::string& getName() const { return m_name;}    // 获取线程名
    void join();                                    // join封装
    static Thread* GetThis();                       // 获取当前进程
    static const std::string& GetName();            // 获取当前线程名

    /**
     * @brief 设置线程名（目的：比如main线程，不能够自己生成，即使不给他创建这个线程对象，他也是存在的。）
     * @param name
     */
    static void SetName(const std::string& name);

private:
    // 禁止拷贝
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;
    /**
     * @brief 线程调用函数的包装函数
     * @param arg 传入this
     * @return
     */
    static void* run(void* arg);
private:
    pid_t m_id = -1;                // 线程ID
    pthread_t m_thread = 0;         // 线程对象pthread
    std::function<void()> m_cb;     // 线程执行函数
    std::string m_name;             // 线程名
    Semaphore m_semaphore;          // 信号量，任意线程都可以解开
};
}

#endif //MYSYLAR_THREAD_H
