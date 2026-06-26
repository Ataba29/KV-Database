#include "Server.h"
#include <iostream>
#include <thread>
#include <sstream>

Server::Server(int port) : port(port), serverSocket(INVALID_SOCKET)
{
    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)

        throw std::runtime_error("WSAStartup Didnt Work Try Again Later!");

    // Create the TCP socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    // Configure the server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
}

Server::~Server()
{
    closesocket(serverSocket);
    WSACleanup();
}

void Server::start()
{
    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
        throw std::runtime_error("Server Failed to Start using bind");

    if (listen(serverSocket, SOMAXCONN) != 0)
        throw std::runtime_error("Server Failed to Listen using listen");
}

void Server::acceptClients()
{
    while (1) // need to keep listening to clients
    {
        SOCKET AcceptSocket = accept(serverSocket, NULL, NULL);
        if (AcceptSocket == INVALID_SOCKET)
            throw std::runtime_error("Server Failed to Accept Clients");

        // TODO replace with a thread pool instead of detach
        // Spin up a new thread for each client so the loop keeps accepting
        std::thread clientThread(&Server::messageHandler, this, AcceptSocket);
        clientThread.detach();
    }
}

void Server::messageHandler(SOCKET clientSocket)
{
    char buffer[1024];

    // Receive data from client into buffer
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0)
    {
        // connection closed or error
        closesocket(clientSocket);
        return;
    }

    // Convert buffer to string for parsing
    std::string message(buffer, bytesReceived);
    std::istringstream iss(message);

    std::string command, key, value;
    iss >> command;
    iss >> key;

    if (command == "INSERT")
    {
        iss >> value;
        // TODO: send to workers
        std::string messageToClient = "Insert command was successful";

        if (send(clientSocket, messageToClient.c_str(), messageToClient.length(), 0) == SOCKET_ERROR)
            throw std::runtime_error("Server Failed to Send to Client");
    }
    else if (command == "GET")
    {
        // TODO: send to workers
        std::string messageToClient = "Get command was successful";

        if (send(clientSocket, messageToClient.c_str(), messageToClient.length(), 0) == SOCKET_ERROR)
            throw std::runtime_error("Server Failed to Send to Client");
    }
    else if (command == "DELETE")
    {
        // TODO: send to workers
        std::string messageToClient = "Delete command was successful";

        if (send(clientSocket, messageToClient.c_str(), messageToClient.length(), 0) == SOCKET_ERROR)
            throw std::runtime_error("Server Failed to Send to Client");
    }
    else
    {
        // TODO: send error back to client
        std::string messageToClient = "No command was recieved";

        if (send(clientSocket, messageToClient.c_str(), messageToClient.length(), 0) == SOCKET_ERROR)
            throw std::runtime_error("Server Failed to Send to Client");
    }

    closesocket(clientSocket);
}