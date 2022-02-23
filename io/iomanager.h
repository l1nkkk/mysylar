//
// Created by l1nkkk on 2021/9/13.
//

#ifndef MYSYLAR_IOMANAGER_H
#define MYSYLAR_IOMANAGER_H

#include "../thread/scheduler.h"
#include "netbase.h"


namespace l1nkkk{
class IOManager : public Scheduler{
public:
    typedef std::shared_ptr<IOManager> sptr;
    typedef RWMutex RWMutexType;

    enum Event{
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x2
    };
private:
    struct FdContext{
        typedef Mutex MutexType;
        struct EventContext{
            Scheduler* scheduler = nullptr;
            Fiber::sptr fiber;
            std::function<void()> cb;
        };
        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event ctx);
        EventContext read;
        EventContext write;
        int fd;
        Event events = NONE;
        MutexType mutex;
    };
public:
    IOManager(size_t threadNum = 1, bool use_caller = true, const std::string& name  = "");
    ~IOManager();

    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);
    static IOManager* GetThis();
protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    void contextResize(size_t size);

private:
    int m_epfd;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

}


#endif //MYSYLAR_IOMANAGER_H
