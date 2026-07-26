#include "Server.h"
#include <iostream>
#include <sstream>

Server::Server(int port) : port(port), serverSocket(INVALID_SOCKET),
                           ss([this]()
                              { pers.createSnapshot(hashMap); }),
                           user_session_background_worker{userSessionManager}
{
    std::cout << "[SERVER] Starting server...\n";

    eventLoop = makeEventLoop();

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        throw std::runtime_error("WSAStartup Didnt Work Try Again Later!");
    }
    std::cout << "[SERVER] WSAStartup Successful!\n";
#endif

    // On Linux, IPPROTO_TCP can safely be 0 for standard streams
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw std::runtime_error("Failed to create socket");
    }

    std::cout << "[SERVER] Socket created successfully!\n";

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    std::cout << "[SERVER] Configured server on port " << port << "\n";

    running = true;
}

Server::~Server()
{
    std::cout << "[SERVER] Shutting down...\n";

    stop();

#ifdef _WIN32
    WSACleanup();
#endif

    std::cout << "[SERVER] Cleanup complete\n";
}

void Server::start()
{
    std::cout << "[SERVER] Binding socket...\n";

    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
    {
        throw std::runtime_error("Server Failed to Start using bind");
    }

    std::cout << "[SERVER] Bind successful!\n";
    std::cout << "[SERVER] Listening for clients...\n";

    if (listen(serverSocket, SOMAXCONN) != 0)
    {
        throw std::runtime_error("Server Failed to Listen using listen");
    }

    std::cout << "[SERVER] Server is now accepting connections!\n";

    pers.loadDataOnStartup(hashMap);

    // Start the event loop on its own thread now that the server is live.
    eventLoopThread = std::thread(&Server::runEventLoop, this);
}

void Server::acceptClients()
{
    while (running)
    {
        std::cout << "[SERVER] Waiting for client...\n";
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        SocketType AcceptSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientAddrLen);

        if (AcceptSocket == INVALID_SOCKET)
        {
            std::cout << "[SERVER] Accept failed (or socket closed)\n";
            break;
        }

        // Get client IP
        uint32_t clientIP{static_cast<uint32_t>(clientAddr.sin_addr.s_addr)};

        // Check rate limit
        if (!rt.isAllowed(clientIP))
        {
            std::cout << "[RATELIMIT] Blocked IP: " << clientIP << "\n";
            CloseSocket(AcceptSocket);
            continue;
        }

        if (!running)
        {
            std::cout << "[SERVER] Server stopping, exiting accept loop\n";
            break;
        }

        SessionKey active_key = userSessionManager.add_session(AcceptSocket, clientAddr);

        if (!setNonBlocking(AcceptSocket))
        {
            std::cout << "[SERVER] Failed to set non-blocking, dropping client\n";
            userSessionManager.remove_session(active_key);
            CloseSocket(AcceptSocket);
            continue;
        }
        connections[AcceptSocket] = std::make_shared<Connection>(AcceptSocket, active_key);
        eventLoop->add(AcceptSocket);
        std::cout << "[SERVER] Client connected and registered!\n";
    }

    std::cout << "[SERVER] acceptClients loop ended\n";
}

void Server::stop()
{
    std::cout << "[SERVER] Stop requested...\n";

    running = false;
    userSessionManager.close_all_sessions();
    CloseSocket(serverSocket);
    // Forces accept() loop to unblock by breaking the file descriptor channel

    // eventLoop->wait() times out every ~1s, so this join returns promptly
    // once running is false, without needing an explicit wake-up signal.
    if (eventLoopThread.joinable())
    {
        eventLoopThread.join();
    }

    std::cout << "[SERVER] Server socket closed, shutdown signal sent\n";
}

void Server::runEventLoop()
{
    std::vector<EventLoopEntry> events;

    while (running)
    {
        int numEvents = eventLoop->wait(events);
        if (numEvents == -1)
            continue; // wait() itself failed, try again

        for (const auto &entry : events)
        {
            if (entry.event == IOEvent::Readable)
            {
                auto it = connections.find(entry.socket);
                if (it == connections.end()) continue;

                std::shared_ptr<Connection> user_connection = it->second; // No Copy
                // wont fire again until we re-arm it.
                tpool.acceptJob([this, conn = std::move(user_connection)] {
                    messageHandler(conn);
                });
            } else // HangUp or Error
            {
                closeConnection(entry.socket);
            }
        }
    }
}

void Server::closeConnection(SocketType sock)
{
    eventLoop->remove(sock);

    auto it = connections.find(sock);
    if (it != connections.end())
    {
        userSessionManager.remove_session(it->second->sessionKey);
        connections.erase(it);
    }

    CloseSocket(sock);
}

void Server::messageHandler(std::shared_ptr<Connection> clientConnection)
{
    std::cout << "[CLIENT] Handling client message\n";

    char tempBuffer[1024];
    SocketType clientSocket = clientConnection->socket;

    // On Linux, the buffer is safely passed to standard recv
    int bytesReceived = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);

    if (bytesReceived == 0)
    {
        // A graceful close: the client actually hung up.
        std::cout << "[CLIENT] Client disconnected\n";
        closeConnection(clientSocket);
        return;
    }
    if (bytesReceived < 0)
    {
        if (wouldBlock())
        {
            // Nothing to read right now - a different job for this socket
            // already drained it, or epoll notified us before this job
            // got scheduled. Not an error, just nothing to do.
            this->rearmSocket(clientSocket);
            return;
        }
        std::cout << "[CLIENT] recv error, disconnecting\n";
        closeConnection(clientSocket);
        return;
    }

    clientConnection->commandBuffer.append(tempBuffer, bytesReceived);

    if (clientConnection->commandBuffer.length() > this->MAX_COMMAND_BUFFER_LENGTH) {
        std::cout << "[CLIENT] Client is abusing the command buffer disconnecting them\n";
        closeConnection(clientSocket);
        return;
    }

    if (clientConnection->commandBuffer.find('\n') == std::string::npos) {
        this->rearmSocket(clientSocket);
        return;
    }



    userSessionManager.update_activity(clientConnection->sessionKey);

    std::cout << "[CLIENT] Received " << bytesReceived << " bytes\n";
    std::string message = clientConnection->commandBuffer;
    std::cout << "[CLIENT] Message: " << message << "\n";
    std::istringstream iss(message);
    std::string command, key, value;
    clientConnection->commandBuffer.clear();

    iss >> command;
    iss >> key;

    std::cout << "[CLIENT] Command: " << command << "\n";
    std::cout << "[CLIENT] Key: " << key << "\n";

    if (command == "INSERT")
    {
        iss >> value;

        std::cout << "[SERVER] INSERT request received\n";
        std::cout << "[SERVER] Value: " << value << "\n";
        if (value.empty())
        {
            std::string response = "Empty Value Recieved, Try again\n";
            send(clientSocket, response.c_str(), response.length(), 0);
            this->rearmSocket(clientSocket);
            return;
        }

        hashMap.insert(key, value);
        pers.appendToLog(command, key, value);

        std::string response = "Insert command was successful\n";
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else if (command == "GET")
    {
        std::cout << "[SERVER] GET request received\n";

        std::string resultGet = hashMap.get(key);

        std::string response;
        if (!resultGet.empty())
            response = "Get command was successful: " + resultGet + "\n";
        else
            response = "Get command was successful but key dont exist\n";

        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else if (command == "DELETE")
    {
        std::cout << "[SERVER] DELETE request received\n";

        hashMap.remove(key);
        pers.appendToLog(command, key, "");

        std::string response = "Delete command was successful\n";
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else
    {
        std::cout << "[SERVER] Unknown command received\n";

        std::string response = "No command was received\n";
        send(clientSocket, response.c_str(), response.length(), 0);
    }

    //Re-enable epoll notifications for the next command from this client
    this->rearmSocket(clientSocket);
}

void Server::rearmSocket(SocketType clientSocket)
{
    if (!eventLoop->rearm(clientSocket))
    {
        std::cout << "[SERVER] Failed to re-arm socket " << clientSocket << ", closing.\n";
        closeConnection(clientSocket);
    }
}