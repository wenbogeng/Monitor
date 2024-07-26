//
//  Server.hpp
//  Monitor
//
//  Created by Wenbo Geng on 1/8/24.
//

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string>
#include <stdint.h>

#include <map>
#include <vector>
#include <mutex>

#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>

#include "Socket.h"

using namespace std;

class Server {
public:
    Server();
    void init(int32_t port, int32_t maxConnects,
              TypeSocket type = TypeSocket::BlockingSocket);  //init: socket(),
    void Run();  //bind(), listen()
    ~Server();

    map<string, SOCKET> _name2controlSocket;
    map<string, SOCKET> _name2dataSocket;
    SocketServer _serverSock;
    mutex _lock;
        
    SOCKET _listenFd;
};

extern Server sv; 

#endif
