#ifndef KV_DATABASE_EVENTLOOPFACTORY_H
#define KV_DATABASE_EVENTLOOPFACTORY_H

#include <memory>
#include "EventLoop.h"

#ifdef _WIN32
#include "IocpEventLoop.h"
#else
#include "EpollEventLoop.h"
#endif

/**
 * @file EventLoopFactory.h
 * @brief Picks the correct IEventLoop implementation for the current platform.
 *
 * This is the only place in the codebase that needs to know whether
 * EpollEventLoop or IocpEventLoop exists - everything else (Server included)
 * only ever talks to the IEventLoop interface.
 */

/**
 * @brief Creates the platform-appropriate event loop.
 * @return A unique_ptr to an EpollEventLoop on Linux, or an IocpEventLoop on Windows.
 */
inline std::unique_ptr<IEventLoop> makeEventLoop()
{
#ifdef _WIN32
    return std::make_unique<IocpEventLoop>();
#else
    return std::make_unique<EpollEventLoop>();
#endif
}

#endif // KV_DATABASE_EVENTLOOPFACTORY_H