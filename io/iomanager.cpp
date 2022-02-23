//
// Created by l1nkkk on 2021/9/13.
//

#include "iomanager.h"

namespace l1nkkk{

void IOManager::tickle() {
    if(!hasIdelThreads())
        return;
    char data = 'T';
    if(netbase::writen(m_tickleFds[1], &data, 1)){
        LOG(FATAL) << "tickle error ";
    }
}

bool IOManager::stopping() {
    return Scheduler::stopping() &&
        m_pendingEventCount == 0;
}


void IOManager::idle() {
    epoll_event* evs = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_event(evs, [](epoll_event* ptr){
        delete []ptr;
    });

    while(true){
        if(stopping()){
            LOG(INFO) << "name=" << m_name << " idle stopping exit";
            break;
        }
        constexpr int MAX_TIMEOUT = 5000;
        auto rt = netbase::epoll_wait(m_epfd, evs, 64, MAX_TIMEOUT);

        for(int i = 0; i < rt; ++i){
            epoll_event& event = evs[i];
            if(event.data.fd == m_tickleFds[0]){
                uint8_t dummy;
                while(read(m_tickleFds[0], &dummy, 1) == 1);//由于是ET模式
                continue;
            }
            FdContext* fd_ctx = static_cast<FdContext*>(event.data.ptr);
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)){
                event.events |= EPOLLIN | EPOLLOUT;
            }

            int real_events = NONE;
            if(event.events & EPOLLIN){
                real_events |= READ;
            }

            if(event.events & EPOLLOUT){
                real_events |= WRITE;
            }

            if((fd_ctx->events & real_events) == NONE){
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            netbase::epoll_ctl(m_epfd, op, fd_ctx->fd, &event);

            if(real_events & READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }


        }
        // 让出协程
        Fiber::sptr  cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut(); // 回到调度器的主协程上（调度协程）
    }

}

IOManager::FdContext::EventContext & IOManager::FdContext::getContext(Event event) {
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            LOG(FATAL) << "getContext error";
    }
}

void IOManager::FdContext::resetContext(EventContext &ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event){
    CHECK(events & event);
    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb){
        ctx.scheduler->schedule(&ctx.cb);
    }else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threadNum, bool use_caller, const std::string &name) :
    Scheduler(threadNum, use_caller, name) {
    m_epfd = netbase::epoll_create(5);
    netbase::pipe(m_tickleFds);

    netbase::setNonblocking(m_tickleFds[0]);
    netbase::epollAddCtl(m_epfd, m_tickleFds[0], EPOLLIN | EPOLLET);

    m_fdContexts.resize(64);

    start();
}
IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);
    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(m_fdContexts[i])
            delete m_fdContexts[i];
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);
    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if(m_fdContexts.size() > fd){
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }else {
        lock.unlock();
        RWMutexType::WriteLock  lock2(m_mutex);
        contextResize(m_fdContexts.size()*1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(fd_ctx->events & event){
        LOG(FATAL) << "addEvent error fd=" << fd << " event=" << event
        << "fd_ctx.event=" << fd_ctx->events;
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;  // 重点1：
    if(auto rt = netbase::epoll_ctl(m_epfd, op, fd, &epevent)){
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);

    CHECK(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb){
        event_ctx.cb.swap(cb);
    }else{
        event_ctx.fiber = Fiber::GetThis();
        CHECK(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock  lock(m_mutex);
    if(m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event))
        return false;

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    if(auto rt = netbase::epoll_ctl(m_epfd, op, fd, &epevent)){
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock  lock(m_mutex);
    if(m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event))
        return false;

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    if(auto rt = netbase::epoll_ctl(m_epfd, op, fd, &epevent)){
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock  lock(m_mutex);
    if(m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events)
        return false;

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    if(auto rt = netbase::epoll_ctl(m_epfd, op, fd, &epevent)){
        return false;
    }

    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    CHECK(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*> (Scheduler::GetThis());
}

}