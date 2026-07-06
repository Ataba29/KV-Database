#ifndef KV_DATABASE_NETWORKTYPES_H
#define KV_DATABASE_NETWORKTYPES_H

/**
 * @file NetworkTypes.h
 * @brief Cross-platform networking type aliases and helper functions.
 *
 * Abstracts the differences between Winsock2 (Windows) and POSIX sockets
 * (Linux/macOS) behind a common set of types and functions, so the rest
 * of the codebase never needs to branch on platform directly.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

/// Native socket type on Windows (Winsock2).
using SocketType = SOCKET;

/**
 * @brief Closes a socket (Windows implementation).
 * @param s The socket to close.
 * @return 0 on success, SOCKET_ERROR on failure.
 */
inline int CloseSocket(SocketType s) { return closesocket(s); }

/**
 * @brief Sets a socket to non-blocking mode (Windows implementation).
 * @param s The socket to modify.
 * @return true on success, false on failure.
 */
inline bool setNonBlocking(SocketType s)
{
    u_long mode = 1; // 1 = non-blocking
    return ioctlsocket(s, FIONBIO, &mode) == 0;
}
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

/// Native socket type on POSIX systems (plain file descriptor).
using SocketType = int;

/**
 * @brief Closes a socket (POSIX implementation).
 * @param s The socket to close.
 * @return 0 on success, -1 on failure.
 */
inline int CloseSocket(SocketType s) { return close(s); }

/// Sentinel value representing an invalid/uninitialized socket (POSIX).
const SocketType INVALID_SOCKET = -1;

/// Sentinel value representing a generic socket error (POSIX).
const int SOCKET_ERROR = -1;

/**
 * @brief Sets a socket to non-blocking mode (POSIX implementation).
 * @param s The socket to modify.
 * @return true on success, false on failure.
 */
inline bool setNonBlocking(SocketType s)
{
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(s, F_SETFL, flags | O_NONBLOCK) != -1;
}
#endif

#endif // KV_DATABASE_NETWORKTYPES_H