#include "Server.h"
#include <iostream>
#include <thread>
#include <sstream>

Server::Server(int port) : port(port), serverSocket(INVALID_SOCKET),
                           ss([this]()
                              { pers.createSnapshot(hashMap); })
{
    std::cout << "[SERVER] Starting server...\n";

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
}

void Server::acceptClients()
{
    while (running)
    {
        std::cout << "[SERVER] Waiting for client...\n";

        SocketType AcceptSocket = accept(serverSocket, NULL, NULL);

        if (!running)
        {
            std::cout << "[SERVER] Server stopping, exiting accept loop\n";
            break;
        }

        if (AcceptSocket == INVALID_SOCKET)
        {
            std::cout << "[SERVER] Accept failed (or socket closed)\n";
            break;
        }

        std::cout << "[SERVER] Client connected!\n";

        tpool.acceptJob([this, AcceptSocket]()
                        { this->messageHandler(AcceptSocket); });

        std::cout << "[SERVER] Created client thread\n";
    }

    std::cout << "[SERVER] acceptClients loop ended\n";
}

void Server::stop()
{
    std::cout << "[SERVER] Stop requested...\n";

    running = false;

    // Forces accept() loop to unblock by breaking the file descriptor channel
    CloseSocket(serverSocket);

    std::cout << "[SERVER] Server socket closed, shutdown signal sent\n";
}

void Server::messageHandler(SocketType clientSocket)
{
    std::cout << "[CLIENT] Handling client message\n";

    char buffer[1024];

    // On Linux, the buffer is safely passed to standard recv
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived <= 0)
    {
        std::cout << "[CLIENT] Client disconnected or error\n";
        CloseSocket(clientSocket);
        return;
    }

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

        hashMap.insert(key, value);
        pers.appendToLog(command, key, value);

        std::string response = "Insert command was successful";
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else if (command == "GET")
    {
        std::cout << "[SERVER] GET request received\n";

        std::string resultGet = hashMap.get(key);

        std::string response;
        if (resultGet != "")
            response = "Get command was successful: " + resultGet;
        else
            response = "Get command was successful but key dont exist";

        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else if (command == "DELETE")
    {
        std::cout << "[SERVER] DELETE request received\n";

        hashMap.remove(key);
        pers.appendToLog(command, key, "");

        std::string response = "Delete command was successful";
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    else
    {
        std::cout << "[SERVER] Unknown command received\n";

        std::string response = "No command was received";
        send(clientSocket, response.c_str(), response.length(), 0);
    }

    std::cout << "[CLIENT] Closing connection\n";
    CloseSocket(clientSocket);
}