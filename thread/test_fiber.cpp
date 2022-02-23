//
// Created by l1nkkk on 2021/5/28.
//

#include "fiber.h"
#include "../common/util.h"
#include "glog/logging.h"

namespace l1nkkk {
    void runInFiber() {
        LLOG(INFO) << "runInFiber begin";
        l1nkkk::Fiber::YieldToHold();
        LLOG(INFO) << "runInFiber end";
    }

    // 单线程多协程
    void test_fiber() {
//        google::InitGoogleLogging("testThread");
//        l1nkkk::L1nkkkInfoLogger my_info_logger;
//        google::base::SetLogger(google::GLOG_INFO, &my_info_logger);
        FLAGS_stderrthreshold = 0;
        FLAGS_alsologtostderr = 1;

        l1nkkk::Fiber::GetThis();   // 将当前的函数变为一个协程，作为主协程
        LLOG(INFO) << "main begin";
        l1nkkk::Fiber::sptr fiber(new l1nkkk::Fiber(runInFiber));
        fiber->call();
        LLOG(INFO) << "main after swapIn";
        fiber->call();
        LLOG(INFO) << "main after end";
        return;
    }

    // 多线程多协程
    // 创建三个线程，每个线程有两个协程，其中一个是主协程。每个线程都有一个主协程
    void test_fiberThread(){
        l1nkkk::Thread::SetName("main");
        std::vector<l1nkkk::Thread::sptr> thrs;
        for(int i = 0; i <3; ++i){
            thrs.push_back(l1nkkk::Thread::sptr(new l1nkkk::Thread(&test_fiber, "thread_" + std::to_string(i))));
        }
        for(int i = 0; i <3; ++i)
            thrs[i]->join();

    }
}