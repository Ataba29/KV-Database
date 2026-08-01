#include <atomic>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "RAM/HashMap.h"
#include "Server/Server.h"

namespace
{
    std::atomic<bool> shutdownRequested{false};
    void handleSignal(int)
    {
        shutdownRequested.store(true, std::memory_order_relaxed);
    }
}

int main()
{
    int port = 6625;

    Server server(port);

    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);

    if (auto result = server.start(); !result)
    {
        std::cerr << "[MAIN] Failed to start server: " << result.error() << "\n";
        return 1;
    }

    // Run accept loop in background thread
    std::jthread serverThread(&Server::acceptClients, &server);

    std::cout << "[MAIN] Server running. Type 'stop' to shut it down.\n";

    std::thread stdinThread([]{
        std::string cmd;
        while(!shutdownRequested.load(std::memory_order_relaxed))
        {
            // The application was ran from docker
            if(!(std::cin >> cmd))
                break;
            
            // This application is running on terminal use old logic to shut down
            if(cmd == "stop"){
                shutdownRequested.store(true, std::memory_order_relaxed);
                break;
            }
        }
    });

    while(!shutdownRequested.load(std::memory_order_relaxed)){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "[MAIN] Stopping server...\n";
    server.stop();

    // Wait for server thread to finish
    serverThread.join();
    stdinThread.detach();

    std::cout << "[MAIN] Server exited cleanly\n";

    return 0;
}