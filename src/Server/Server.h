#ifndef SERVER_H
#define SERVER_H
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>

#include "UserSessionBackgroundWorker.h"
#include "../RAM/HashMap.h"
#include "../Storage/Persistence.h"
#include "../Worker/ThreadPool.h"
#include "../Worker/SnapshotScheduler.h"
#include "../Limit/Limiter.h"
#include "../Networking/NetworkTypes.h"
#include "../UserSession/UserSession.h"
#include "../UserSession/Connection.h"
#include "../Networking/EventLoopFactory.h"

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

    HashMap hashMap;                                            /** Server owns an instance of the hashmap */
    Persistence pers;                                           /** Server owns an instance of persistance class */
    ThreadPool tpool;                                           /** Server owns an instance of ThreadPool class */
    SnapshotScheduler ss;                                       /** Server owns an instance of SnapshotScheduler class */
    RateLimiter rt;                                             /** Server owns an instance of RateLimter class */
    UserSessionManager userSessionManager;                      /** Managing User Sessions */
    UserSessionBackgroundWorker user_session_background_worker; /** Background worker that sweeps expired sessions*/
    std::unordered_map<SocketType, std::shared_ptr<Connection>> connections;     /** Client connections, keyed by socket */
    std::unique_ptr<IEventLoop> eventLoop;                      /** Watches all client sockets for readiness */
    std::thread eventLoopThread;                                /** Thread that runs runEventLoop() */

    /**
     * @brief Runs continuously on eventLoopThread: waits for socket readiness
     *        and dispatches ready clients to the thread pool.
     */
    void runEventLoop();

    /**
     * @brief Fully tears down a client connection: stops watching it, removes
     *        its session and Connection entry, and closes the socket.
     * @param sock The socket to close.
     */
    void closeConnection(SocketType sock);

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
     * @brief Handles one ready-to-read event for a client: one recv() call,
     *        command parsing, and response.
     * @param userConnection The user connection
     *
     */
    void messageHandler(std::shared_ptr<Connection> userConnection);

    /**
    * What it is: An explicit call (epoll_ctl with EPOLL_CTL_MOD) executed when a thread finishes its work.
    *    What it does: Unmutes the socket so epoll can start listening for network activity again.
     *   Why we use it: Because EPOLLONESHOT completely mutes the socket, it will stay dead forever unless re-armed.
     *   Every exit path in your thread—whether it finished a full message or is waiting for more bytes (fragmentation)—must call rearm().
     *
     * @param clientSocket
     */
    void rearmSocket(SocketType clientSocket);


    uint16_t MAX_COMMAND_BUFFER_LENGTH = 2024;
};

#endif