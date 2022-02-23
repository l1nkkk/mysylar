//
// Created by l1nkkk on 4/19/21.
//
#include <vector>
#include <iostream>
#include <string>
#include <glog/logging.h>
#include "thread.h"
#include "mutex.h"
#include "../common/util.h"
int count = 0;
//l1nkkk::RWMutex s_mutex;
l1nkkk::Mutex s_mutex;

namespace l1nkkk{
    void fun1(){
        LOG(INFO) << "name:" << l1nkkk::Thread::GetName()
                << " this.name:" << l1nkkk::Thread::GetThis()->getName()
                << " id:" << l1nkkk::GetThreadId()
                << " this.id:" << l1nkkk::Thread::GetThis()->getID();
        for(int i = 0; i < 100000; ++i) {
            // 注释下一句将导致错误
            // l1nkkk::RWMutex::WriteLock lock(s_mutex);
            l1nkkk::Mutex::Lock lock(s_mutex);
            ++count;
        }
    }

    void fun2(){

    }
    void testThread(){
        google::InitGoogleLogging("testThread");
        FLAGS_logtostderr = 1;
        FLAGS_stderrthreshold = 0;
        LOG(INFO) << "thread test begin";
        std::vector<l1nkkk::Thread::sptr> thrs;
        for(int i = 0; i < 5; ++i){
            l1nkkk::Thread::sptr thr(new l1nkkk::Thread(&fun1, "name_" + std::to_string(i)));
            thrs.push_back(thr);
        }
        for(int i = 0; i < 5;++i){
            thrs[i]->join();
        }
        LOG(INFO) << "count = " << count;
        LOG(INFO) << "thread test end";
    }
}