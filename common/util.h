//
// Created by l1nkkk on 3/22/21.
//

#ifndef LOGGER_UTIL_H
#define LOGGER_UTIL_H
#include <sys/syscall.h>
#include <zconf.h>
#include <glog/logging.h>
#include <iostream>

# define LLOG(x) LOG(x) << " " << l1nkkk::GetFiberId() << " " << l1nkkk::Thread::GetName() << " --> "
# define LDLOG(x) DLOG(x) << " " << l1nkkk::GetFiberId() << " " << l1nkkk::Thread::GetName() << " --> "
# define LCHECK(x) CHECK(x) << " " << l1nkkk::GetFiberId() << " " << l1nkkk::Thread::GetName() << " --> "
namespace l1nkkk {
    pid_t GetThreadId();
    int64_t GetFiberId();
    class L1nkkkInfoLogger : public google::base::Logger {
    public:
        virtual void Write(bool force_flush,
                           time_t timestamp,
                           const char* message,
                           int message_len) {

            std::cout << timestamp << "\t" << GetFiberId() << '\t'
                      << message ;
        }

        virtual void Flush() {
            std::cout << "MyInfoLogger::Flush" << std::endl;
        }

        virtual google::uint32 LogSize() {
            return 0;
        }
    };
}
#endif //LOGGER_UTIL_H
