//
// Created by l1nkkk on 2021/5/28.
//

#include "mutex.h"
namespace l1nkkk{
Semaphore::Semaphore(uint32_t count){
    if(sem_init(&m_semaphore, 0, count)){
        throw std::logic_error("sem_init error");
    }
}
Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);
}

void Semaphore::wait(){
//    while(true){
    // while 防止假唤醒，比如被信号中断了。
    if(sem_wait(&m_semaphore)){
        throw std::logic_error("sem_wait error");
    }
//    }
}
void Semaphore::notify(){
    if(sem_post(&m_semaphore)){
        throw std::logic_error("sem_post error");
    }
}
}