#ifndef KV_DATABASE_IOCPEVENTLOOP_H
#define KV_DATABASE_IOCPEVENTLOOP_H

#ifdef _WIN32

#include "EventLoop.h"
#include <windows.h>
#include <unordered_map>
#include <memory>

/**
 * @file IocpEventLoop.h
 * @brief Windows implementation of IEventLoop, backed by IOCP.
 *
 * IOCP is completion-based rather than readiness-based: instead of
 * "this socket has data, go read it," it reports "an operation you
 * already posted has finished." To present the same readiness-style
 * interface as EpollEventLoop, this class posts a zero-byte WSARecv per
 * socket - a recv() with no buffer, which completes as soon as data
 * ARRIVES but consumes none of it. That completion is reported here as
 * Readable, and the caller still performs its own real recv() afterward,
 * exactly like on Linux.
 */

/**
 * @brief Per-socket state needed to keep a pending zero-byte WSARecv alive.
 *
 * OVERLAPPED and WSABUF must stay valid in memory for as long as the OS
 * has a pending I/O operation referencing them, so each socket gets its
 * own heap-allocated instance rather than a stack local.
 */
struct IocpContext
{
    OVERLAPPED overlapped;
    SocketType socket;
    WSABUF buffer;
    char dummy; // unused - zero-byte recv still needs a valid buffer pointer
};

/**
 * @brief Readiness-style event loop using Windows I/O Completion Ports.
 */
class IocpEventLoop : public IEventLoop
{
public:
    IocpEventLoop();
    ~IocpEventLoop() override;

    void add(SocketType sock) override;
    void remove(SocketType sock) override;
    int wait(std::vector<EventLoopEntry> &out) override;
    bool rearm(SocketType sock) override;

private:
    /// Posts (or re-posts) the zero-byte WSARecv that arms readiness notification for a socket.
    void armRead(SocketType sock);

    /// Handle to the completion port itself.
    HANDLE iocpHandle;

    /// Per-socket context objects, keyed by socket, kept alive while a recv is pending.
    std::unordered_map<SocketType, std::unique_ptr<IocpContext>> contexts;
};

#endif //_WIN32
#endif // KV_DATABASE_IOCPEVENTLOOP_H