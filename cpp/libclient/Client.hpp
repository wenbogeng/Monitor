//
//  CLIENT.hpp
//  Monitor
//
//  Created by Wenbo Geng on 1/8/24.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <string.h>
#include <memory>
#include <sstream>
#include <vector>
#include <map>
#include <numeric>
#include <string>
#include <functional>
#include <sys/stat.h>

#include "Socket.h"

using namespace std;

class Client
{
public:
    Client(const string& name, const std::string& ip, int32_t port);
    void Run();
    
    static vector<string> STLStringToTokens(string const& str,  char delim, bool allowEmpty = false);
    static void readConfig(string &name, string &ip, int &port);
    
protected:
    int getReplyCode(const string &reply);
    bool Login();
    bool SetPasvMode();
    void SendFile(const string &path);
    void ReceiveFile(const string &path);
    void SendList(const string &path);
    
    string          _ip;
    SocketClient        _clientSock;
    SocketClient        *_dataSock;
    
    string            _username;
    // string          _password;
};

#endif
