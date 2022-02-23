//
// Created by l1nkkk on 2021/5/30.
//

#include "scheduler.h"
#include <glog/logging.h>
#include "../common/util.h"
namespace l1nkkk {

void testSchedule() {
    LLOG(INFO) << "main================";
    google::InstallFailureSignalHandler();
    l1nkkk::Scheduler sc;
    sc.start();
    sc.stop();
    LLOG(INFO) << "main over===================";
}


void testFiber(){

    static int  s_count = 5;
    LLOG(INFO) << "test in fiber count " << s_count;
    sleep(1);
    if(--s_count >= 0){
        l1nkkk::Scheduler::GetThis()->schedule(&testFiber);
    }

}

void testSchedule2() {
    LLOG(INFO) << "main";
    google::InstallFailureSignalHandler();
    l1nkkk::Scheduler sc;

    sc.start();
    sc.schedule(&testFiber);
    sc.stop();
    LLOG(INFO) << "main over";
}


void testSchedule3() {
    LLOG(INFO) << "main";
    google::InstallFailureSignalHandler();
    l1nkkk::Scheduler sc(3, false, "test");

    sc.start();
    sc.schedule(&testFiber);
    sc.stop();
    LLOG(INFO) << "main over";
}

}