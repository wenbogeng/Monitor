#pragma once
#include <string>
#include <stdint.h>

enum class TypeSocket : int8_t
{
    BlockingSocket,
    NonBlockingSocket
};

#include "pub.h"

// CURRENT only support TCP, ref Socket()
class Socket
{
public:
    virtual ~Socket();
    Socket(const Socket&);
    void get_socket();
    void close_sock();
    Socket& operator=(const Socket&);

    std::string receiveLine(bool readAll=true);
    int32_t receiveBytes(char* buf, int32_t buflen);
    void sendLine(const std::string& s);
    void sendBytes(const char* msg, int32_t len);

    SOCKET GetSocket() const { return _socket; }


    explicit Socket(SOCKET s);
protected:
    friend class SocketServer;
    friend class SocketSelect;

    Socket();
    //explicit Socket(SOCKET s);
    SOCKET _socket;
    int32_t* _refCounter;
private:
    static void start();
    static void end();
    static int32_t _nofSockets;
};

class SocketClient : public Socket
{
public:
    SocketClient() {}
    SocketClient(const std::string& hostname, int32_t port);
    void init_url(const std::string& hostname, int32_t port);
    void init_ipaddr(const std::string& ip, int32_t port);
};

class SocketServer : public Socket
{
public:
    void init(int32_t port, int32_t maxConnects, TypeSocket type = TypeSocket::BlockingSocket);
    Socket* try_accept();
};
class SocketSelect
{
public:
    explicit SocketSelect(Socket const* const s1, Socket const* const s2 = nullptr, TypeSocket type = TypeSocket::BlockingSocket);
    bool readable(Socket const* const s);
private:
    fd_set _fds;
};
