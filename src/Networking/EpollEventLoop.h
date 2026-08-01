#ifndef KV_DATABASE_EPOLLEVENTLOOP_H
#define KV_DATABASE_EPOLLEVENTLOOP_H

#ifndef _WIN32

#include "EventLoop.h"
#include <sys/epoll.h>

/**
 * @file EpollEventLoop.h
 * @brief Linux implementation of IEventLoop, backed by epoll.
 *
 * Compiles to nothing on Windows (guarded by #ifndef _WIN32), since
 * <sys/epoll.h> only exists on Linux.
 */

/**
 * @brief Readiness-based event loop using the Linux epoll API.
 */
class EpollEventLoop : public IEventLoop
{
public:
    /**
     * @brief Creates a new epoll instance.
     */
    EpollEventLoop();

    /**
     * @brief Closes the underlying epoll file descriptor.
     */
    ~EpollEventLoop() override;

    void add(SocketType sock) override;
    void remove(SocketType sock) override;
    int wait(std::vector<EventLoopEntry> &out) override;
    bool rearm(SocketType sock) override;

private:
    /// File descriptor for the epoll instance itself.
    int epollFd;

    /// Maximum number of events epoll_wait() can report in a single call.
    static const int MAX_EVENTS = 64;
};

#endif //_WIN32
#endif // KV_DATABASE_EPOLLEVENTLOOP_H