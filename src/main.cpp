#include <iostream>
#include <string>
#include "RAM/HashMap.h"
#include "Server/Server.h"

int main()
{
    int port = 6625;
    Server server = Server(port);
    server.start();
    server.acceptClients();
    return 0;
}