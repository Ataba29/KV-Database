#ifndef KV_DATABASE_EVENTLOOP_H
#define KV_DATABASE_EVENTLOOP_H

#include <vector>
#include "NetworkTypes.h"

/**
 * @file EventLoop.h
 * @brief Platform-independent event loop interface.
 *
 * Defines the contract that both EpollEventLoop (Linux) and IocpEventLoop
 * (Windows) implement, so Server only ever talks to IEventLoop and never
 * needs to know which concrete implementation is underneath.
 */

/// The kind of readiness event that occurred on a socket.
enum class IOEvent
{
    Readable,
    Writable,
    HangUp,
    Error
};

/**
 * @brief A single ready-event reported by the event loop.
 */
struct EventLoopEntry
{
    /// The socket the event occurred on.
    SocketType socket;

    /// What kind of event occurred.
    IOEvent event;
};

/**
 * @brief Abstract interface for a readiness-based event loop.
 *
 * Concrete implementations wrap a platform's native mechanism
 * (epoll on Linux, IOCP on Windows) behind this common API.
 */
class IEventLoop
{
public:
    virtual ~IEventLoop() = default;

    /**
     * @brief Starts watching a socket for events.
     * @param sock The socket to watch.
     */
    virtual void add(SocketType sock) = 0;

    /**
     * @brief Stops watching a socket.
     * @param sock The socket to stop watching.
     */
    virtual void remove(SocketType sock) = 0;

    /**
     * @brief Blocks (up to an internal timeout) until at least one watched
     *        socket is ready, or the timeout elapses with nothing ready.
     * @param out Filled with the sockets that are ready and what happened to them.
     * @return The number of ready events, 0 if the wait timed out with
     *         nothing ready, or -1 on error.
     */
    virtual int wait(std::vector<EventLoopEntry> &out) = 0;

    // Explained inside the Server.h
    virtual bool rearm(SocketType sock) = 0;
};

#endif // KV_DATABASE_EVENTLOOP_H