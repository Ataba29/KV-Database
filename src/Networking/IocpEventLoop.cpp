#include "IocpEventLoop.h"

#ifdef _WIN32

IocpEventLoop::IocpEventLoop()
{
    iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
}

IocpEventLoop::~IocpEventLoop()
{
    CloseHandle(iocpHandle);
}

void IocpEventLoop::add(SocketType sock)
{
    // Associate this socket with the completion port. The socket itself
    // is used as the "completion key" so wait() can identify it later.
    CreateIoCompletionPort((HANDLE)sock, iocpHandle, (ULONG_PTR)sock, 0);

    auto context = std::make_unique<IocpContext>();
    context->socket = sock;
    ZeroMemory(&context->overlapped, sizeof(OVERLAPPED));
    context->buffer.buf = &context->dummy;
    context->buffer.len = 0; // zero-byte: completes on arrival, consumes nothing

    contexts[sock] = std::move(context);
    armRead(sock);
}

void IocpEventLoop::armRead(SocketType sock)
{
    auto it = contexts.find(sock);
    if (it == contexts.end())
        return;

    IocpContext *ctx = it->second.get();
    DWORD flags = 0;
    DWORD bytesRecvd = 0;

    WSARecv(sock, &ctx->buffer, 1, &bytesRecvd, &flags, &ctx->overlapped, nullptr);
    // WSA_IO_PENDING is the expected outcome here - it means "queued,
    // will complete later," not an error. Its eventual completion (or
    // failure) surfaces later through wait().
}

void IocpEventLoop::remove(SocketType sock)
{
    contexts.erase(sock);
    // IOCP has no direct "unregister" call. Once the caller closes the
    // socket itself (via CloseSocket), the OS tears down its association
    // with the port automatically.
}

int IocpEventLoop::wait(std::vector<EventLoopEntry> &out)
{
    DWORD bytesTransferred = 0;
    ULONG_PTR completionKey = 0;
    OVERLAPPED *overlapped = nullptr;

    BOOL success = GetQueuedCompletionStatus(
        iocpHandle, &bytesTransferred, &completionKey, &overlapped, 1000); // 1s timeout so shutdown can be noticed

    if (overlapped == nullptr)
    {
        return -1; // timed out with nothing ready, or a serious port-level error
    }

    SocketType sock = static_cast<SocketType>(completionKey);
    out.clear();

    if (!success)
    {
        // The posted operation itself failed (e.g. connection reset).
        out.push_back(EventLoopEntry{sock, IOEvent::Error});
    }
    else
    {
        // The zero-byte recv completed - data is available to read.
        // bytesTransferred will be 0 regardless (we asked for none), so
        // it does NOT mean the connection closed. The caller does a real
        // recv() next; if THAT returns 0, that's how a graceful close is
        // detected - same pattern as epoll's EPOLLIN + recv()==0 on Linux.
        out.push_back(EventLoopEntry{sock, IOEvent::Readable});
        armRead(sock); // re-arm so we're notified again for the next batch of data
    }

    return 1;
}

#endif //_WIN32