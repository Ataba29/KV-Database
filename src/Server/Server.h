#ifndef SERVER_H
#define SERVER_H

// --- Cross-Platform Networking Layers ---
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

#include <stdexcept>
#include <string>
#include <atomic>
#include "../RAM/HashMap.h"
#include "../Storage/Persistence.h"
#include "../Worker/ThreadPool.h"
#include "../Worker/SnapshotScheduler.h"
#include "../Limit/Limiter.h"

/**
 * @brief TCP server that listens for client connections
 *        and dispatches commands to workers.
 */
class Server
{
private:
    SocketType serverSocket; /**< The main listening socket */
    sockaddr_in serverAddr;  /**< Server address structure */
    int port;                /**< Port the server listens on */

    std::atomic<bool> running; /**< Controls whether the server is running */

    HashMap hashMap;      /** Server owns an instance of the hashmap */
    Persistence pers;     /** Server owns an instance of persistance class */
    ThreadPool tpool;     /** Server owns an instance of ThreadPool class */
    SnapshotScheduler ss; /** Server owns an instance of SnapshotScheduler class */
    RateLimiter rt;       /** Server owns an instance of RateLimter class */

public:
    /**
     * @brief Initializes the server with a given port.
     * @param port The port number to listen on.
     */
    Server(int port);

    /**
     * @brief Cleans up and closes the server socket.
     */
    ~Server();

    /**
     * @brief Starts the server, binds and begins listening.
     * @throws std::runtime_error if socket setup fails.
     */
    void start();

    /**
     * @brief Accepts incoming client connections in a loop.
     */
    void acceptClients();

    /**
     * @brief Stops the server and closes the listening socket.
     */
    void stop();

    /**
     * @brief Handles communication with a connected client.
     * @param clientSocket The socket returned by accept().
     */
    void messageHandler(SocketType clientSocket);
};

#endif