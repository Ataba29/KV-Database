#include <iostream>
#include <string>
#include <thread>

#include "RAM/HashMap.h"
#include "Server/Server.h"

int main()
{
    int port = 6625;

    Server server(port);

    if (auto result = server.start(); !result)
    {
        std::cerr << "[MAIN] Failed to start server: " << result.error() << "\n";
        return 1;
    }

    // Run accept loop in background thread
    std::jthread serverThread(&Server::acceptClients, &server);

    std::cout << "[MAIN] Server running. Type 'stop' to shut it down.\n";

    std::string cmd;

    while (true)
    {
        std::cin >> cmd;

        if (cmd == "stop")
        {
            std::cout << "[MAIN] Stopping server...\n";

            server.stop();

            break;
        }
    }

    std::cout << "[MAIN] Server exited cleanly\n";

    return 0;
}