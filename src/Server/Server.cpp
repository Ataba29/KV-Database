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

    userSessionManager.set_on_session_expired(
        [this](SocketType sock)
        { onSessionExpired(sock); });

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
        connections[AcceptSocket] = Connection{AcceptSocket, active_key};
        eventLoop->add(AcceptSocket);
        std::cout << "[SERVER] Client connected and registered!\n";
    }

    std::cout << "[SERVER] acceptClients loop ended\n";
}

void Server::stop()
{
    std::cout << "[SERVER] Stop requested...\n";

    running = false;
    for (auto &[sock, conn] : connections)
        CloseSocket(sock);
    connections.clear();
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
        // Close all the expired connections
        {
            std::vector<SocketType> close;
            {
                std::lock_guard<std::mutex> lock(expiredMutex);
                close.swap(expiredSockets);
            }
            for (auto &sock : close){
                std::cout << "[SERVER] Closing expired connection, socket " << sock << "\n";
                closeConnection(sock);
            }
        }
        int numEvents = eventLoop->wait(events);
        if (numEvents == -1)
            continue; // wait() itself failed, try again

        for (const auto &entry : events)
        {
            if (entry.event == IOEvent::Readable)
            {
                auto it = connections.find(entry.socket);
                if (it == connections.end()) continue;

                Connection conn = it->second; // small struct, cheap to copy
                // wont fire again until we re-arm it.
                tpool.acceptJob([this, conn] {
                    messageHandler(conn.socket, conn.sessionKey);
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
        userSessionManager.remove_session(it->second.sessionKey);
        connections.erase(it);
    }

    CloseSocket(sock);
}

void Server::messageHandler(SocketType clientSocket, const SessionKey &sessionKey)
{
    std::cout << "[CLIENT] Handling client message\n";

    char buffer[1024];

    // On Linux, the buffer is safely passed to standard recv
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

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
    userSessionManager.update_activity(sessionKey);

    std::cout << "[CLIENT] Received " << bytesReceived << " bytes\n";
    std::string message(buffer, bytesReceived);
    std::cout << "[CLIENT] Message: " << message << "\n";
    std::istringstream iss(message);
    std::string command, key, value;

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

void Server::onSessionExpired(SocketType sock)
{
    std::cout << "[SERVER] Session expired, queuing socket " << sock << " for close\n";
    std::lock_guard<std::mutex> lock(expiredMutex);
    expiredSockets.push_back(sock);
}