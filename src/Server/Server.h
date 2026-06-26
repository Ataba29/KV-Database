#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <stdexcept>
#include <string>

/**
 * @brief TCP server that listens for client connections
 *        and dispatches commands to workers.
 */
class Server
{
private:
    SOCKET serverSocket;    ///< The main listening socket
    sockaddr_in serverAddr; ///< Server address structure
    int port;               ///< Port the server listens on

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
     * @brief Handles communication with a connected client.
     * @param clientSocket The socket returned by accept().
     */
    void messageHandler(SOCKET clientSocket);

    /**
     * @brief Forwards a parsed command to the thread pool workers.
     * @param command The parsed command string (INSERT, GET, DELETE).
     */
    void contactWorkers(const std::string &command);
};

#endif