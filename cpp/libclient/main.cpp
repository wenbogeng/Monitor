//
//  main.cpp
//  Monitor
//
//  Created by Wenbo Geng on 1/7/24.
//

#include <iostream>
#include "Client.hpp"

int main(int argc, const char * argv[]) {
    string name;
    string ip;
    int port;

    Client::readConfig(name, ip, port);

    Client cl(name, ip, port);
    cl.Run();

    return 0;
//    SocketServer server;
//    server.init(12345, 5);
//
//    while (true) {
//        Socket* client = server.try_accept();
//        if (client) {
//            std::cout << "Client connected!" << std::endl;
//            std::string receivedMessage = client->receiveLine();
//            std::cout << "Received: " << receivedMessage << std::endl;
//
//            client->sendLine("Hello from server!");
//            delete client;
//            std::cout << "Client disconnected." << std::endl;
//        }
//    }
//    return 0;
}

