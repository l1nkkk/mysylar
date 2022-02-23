//
// Created by l1nkkk on 3/23/21.
//
#include "util.h"
#include "../thread/fiber.h"
namespace l1nkkk {
pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

int64_t GetFiberId(){
    return l1nkkk::Fiber::GetFiberId();
}

}