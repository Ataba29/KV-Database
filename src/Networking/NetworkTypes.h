//
// Created by razi on 7/3/2026.
//

#ifndef KV_DATABASE_NETWORKTYPES_H
#define KV_DATABASE_NETWORKTYPES_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

// Use type aliases to match native Windows paradigms cleanly
using SocketType = SOCKET;
inline int CloseSocket(SocketType s) { return closesocket(s); }
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Define standard POSIX elements to match the unified implementation API
using SocketType = int;
inline int CloseSocket(SocketType s) { return close(s); }

const SocketType INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
#endif


#endif //KV_DATABASE_NETWORKTYPES_H
