#include "EpollEventLoop.h"

#ifndef _WIN32

#include <unistd.h>
#include <iostream>

EpollEventLoop::EpollEventLoop()
{
    epollFd = epoll_create1(0);
}

EpollEventLoop::~EpollEventLoop()
{
    close(epollFd);
}

void EpollEventLoop::add(SocketType sock)
{
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLONESHOT; // watch for "readable", (intentionally "mutes" the socket after the first notification)
    ev.data.fd = sock;

    epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &ev);
}

void EpollEventLoop::remove(SocketType sock)
{
    epoll_ctl(epollFd, EPOLL_CTL_DEL, sock, nullptr);
}

int EpollEventLoop::wait(std::vector<EventLoopEntry> &out)
{
    epoll_event events[MAX_EVENTS];

    int numReady = epoll_wait(epollFd, events, MAX_EVENTS, 1000); // 1s timeout so shutdown can be noticed
    if (numReady == -1)
    {
        return -1;
    }

    out.clear();
    for (int i = 0; i < numReady; ++i)
    {
        IOEvent ioEvent;

        if (events[i].events & (EPOLLHUP | EPOLLRDHUP))
        {
            ioEvent = IOEvent::HangUp;
        }
        else if (events[i].events & EPOLLERR)
        {
            ioEvent = IOEvent::Error;
        }
        else if (events[i].events & EPOLLIN)
        {
            ioEvent = IOEvent::Readable;
        }
        else
        {
            ioEvent = IOEvent::Writable;
        }

        out.push_back(EventLoopEntry{static_cast<SocketType>(events[i].data.fd), ioEvent});
    }

    return numReady;
}

bool EpollEventLoop::rearm(SocketType sock)
{
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLONESHOT;
    ev.data.fd = sock;

    return epoll_ctl(epollFd, EPOLL_CTL_MOD, sock, &ev) == 0;
}

#endif //_WIN32