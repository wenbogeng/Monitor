#include "./libnet/Socket.h"
#include <iostream>
#include <string.h>
#include <pthread.h>
#include <fstream>
#include <sstream>

using namespace std;
int32_t Socket::_nofSockets = 0; // 静态整型变量，记录已创建的 Socket 对象数量。

#if defined(WIN32)
#define CLOSESOCKET closesocket
#define IOCTLSOCKET ioctlsocket
#else
#define CLOSESOCKET close
#define IOCTLSOCKET ioctl // ioctl 函数是 input/output control ioctl 函数可以获取套接字可读数据字节数，套接字的状态等，而 read() 函数只能读取数据。 因此，使用 ioctl 函数可以避免 read() 函数发生阻塞。
#endif

/*
 start(): 增加 _nofSockets 的值，并在 Windows 平台上初始化 Winsock API
 */
void Socket::start()
{
#if defined(WIN32) //
    if (!_nofSockets)
    {
        WSADATA info;
        if (WSAStartup(MAKEWORD(2, 0), &info))
        {
            throw "Could not start WSA";
        }
    }
#endif
    ++_nofSockets; // 对象数量加一
}
/*
 只有当没有 Socket 对象存在时，才会调用 WSACleanup() 清理 Winsock 环境
 */
void Socket::end()
{
#if defined(WIN32)
    WSACleanup();
#endif
}
Socket::Socket() : _socket(0)
{
    start();
    get_socket();
    if (_refCounter == nullptr)
    {
        _refCounter = new int32_t(1);
    }
}
/*
 构造函数Socket(SOCKET s): 接收一个已存在的套接字句柄，并初始化成员变量。
 */
Socket::Socket(SOCKET s) : _socket(s)
{
    start();
    if (_refCounter == nullptr)
    {
        _refCounter = new int32_t(1);
    }
};

/*
 get_socket(): 创建一个 TCP 套接字，并将句柄赋值给 _socket。
 */
void Socket::get_socket()
{
    // UDP: use SOCK_DGRAM instead of SOCK_STREAM
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket == -1)
    {
        throw "-1";
    }
}
/*
 析构函数，在引用计数为 0 时关闭套接字并释放资源。
 */
Socket::~Socket()
{
    if (!--(*_refCounter))
    {
        close_sock();
        delete _refCounter;
    }
    
    --_nofSockets;
    if (!_nofSockets)
    {
        end();
    }
}
/*
 构造函数 (copy constructor):
    Socket(const Socket& o): 复制构造函数，用于创建新的 Socket 对象，并从另一个 Socket 对象 (o) 中拷贝其数据。
    _refCounter = o._refCounter; ++(*_refCounter);: 增加对 o 引用计数的引用，确保源对象不会被释放。
    _socket = o._socket;: 复制源对象的套接字句柄。
    ++_nofSockets;: 增加全局的套接字数量。
 */
Socket::Socket(const Socket& o)
{
    _refCounter = o._refCounter;
    ++(*_refCounter);
    _socket = o._socket;
    
    ++_nofSockets;
}
/*
 赋值运算符 (copy assignment):
    Socket& operator=(const Socket& o): 赋值运算符，用于将另一个 Socket 对象 (o) 的数据复制到当前对象。
    ++(*o._refCounter);: 增加对 o 引用计数的引用，确保源对象不会被释放。
    _refCounter = o._refCounter;: 将当前对象的引用计数指向 o 的引用计数，避免重复释放资源。
    _socket = o._socket;: 复制源对象的套接字句柄。
    ++_nofSockets;: 增加全局的套接字数量。
    return *this;: 返回当前对象自身，用于链式赋值。
 */
Socket& Socket::operator=(const Socket& o)
{
    ++(*o._refCounter);
    
    _refCounter = o._refCounter;
    _socket = o._socket;
    
    ++_nofSockets;
    
    return *this;
}
/*
 close_sock() 函数:
 close_sock(): 关闭当前对象的套接字，使用平台相关的 CLOSESOCKET 函数。
 */
void Socket::close_sock()
{
    CLOSESOCKET(_socket);
}
/*
 ReceiveBytes() 函数:

    receiveBytes(char* outmsg, int32_t buflen): 接收来自套接字的数据，将其存储到 outmsg 缓冲区中。

    循环读取数据：
    IOCTLSOCKET(_socket, FIONREAD, &arg): 获取套接字可读数据字节数，存储在 arg 中。
    如果没有可读数据或出错，则退出循环。
    将可读数据与 buflen 进行比较，取较小值作为本次读取的字节数。
    使用 recv(_socket, buf, arg, 0) 从套接字读取数据到 buf 中，返回实际读取的字节数。
    将读取到的数据复制到 outmsg 缓冲区中。
    更新 ci 和 msglen 变量。
    如果读取到的数据超过 buflen，则打印警告信息并返回 -1。
    返回已接收的字节总数。
 */
int32_t Socket::receiveBytes(char* outmsg, int32_t buflen)
{
    int32_t ci = 0; // ci: 当前已接收的数据字节数。
    const int32_t maxbuflen = 2048; // maxbuflen: 临时缓冲区 buf 的大小。
    char buf[maxbuflen]; // buf: 临时缓冲区，用于接收数据。
    
    /*
     循环读取数据：
     IOCTLSOCKET(_socket, FIONREAD, &arg): 获取套接字可读数据字节数，存储在 arg 中。
     如果没有可读数据或出错，则退出循环。
     将可读数据与 buflen 进行比较，取较小值作为本次读取的字节数。
     使用 recv(_socket, buf, arg, 0) 从套接字读取数据到 buf 中，返回实际读取的字节数。
     将读取到的数据复制到 outmsg 缓冲区中。
     更新 ci 和 msglen 变量。
     如果读取到的数据超过 buflen，则打印警告信息并返回 -1。
     */
    while (1)
    {
        // 检查套接字是否有可读数据。如果有可读数据，则将 arg 变量的值作为本次读取的字节数。如果没有可读数据，则循环结束。
        u_long arg = 0; // 定义一个 u_long 类型的变量 arg，用于存储可读数据的字节数
        if (IOCTLSOCKET(_socket, FIONREAD, &arg) != 0) // 使用 ioctl() 函数调用 FIONREAD 命令，获取套接字可读数据字节数。如果调用失败，则 break 退出循环
        {
            break;
        }
        
        if (arg == 0) // 检查 arg 变量的值。如果 arg 为 0，则表示套接字没有可读数据，也 break 退出循环。
        {
            break;
        }
        
        if (arg > maxbuflen) // 防止读取的数据超过 buf 缓冲区的大小。 将可读数据与 buflen 进行比较，取较小值作为本次读取的字节数。
        {
            arg = maxbuflen;
        }
        
        int32_t rv = (int)recv(_socket, buf, arg, 0); // recv() 函数只能获取套接字可读数据字节数，而 ioctl() 函数可以获取套接字的任何属性。 recv() 函数可能会发生阻塞，而 ioctl() 函数不会发生阻塞。 函数的返回值是实际读取的字节数。如果返回值小于或等于 0，则表示发生错误
        if (rv <= 0)
        {
            break;
        }
        
        int32_t msglen = ci + rv; // msglen = ci + rv 表示已读取数据的长度。ci 表示已读取数据的字节数，rv 表示本次读取的字节数。
        
        /*
         如果 msglen > buflen，则表示读取到的数据超过 buf 缓冲区的大小，函数会打印错误信息，并将 outmsg 缓冲区中剩余的数据复制到 buf 缓冲区中。
         */
        if (msglen > buflen)
        {
            printf("!!ERR!! msg.len > buf.len : %d %d\n", msglen, buflen);
            memcpy(&outmsg[ci], buf, buflen - ci); // 将 outmsg 缓冲区中从 ci 位置开始的 buflen - ci 字节的数据复制到 buf 缓冲区中
            return -1;
        }
        else // 没有超过的话 将本次读取到的数据写入 outmsg 缓冲区中，并更新 ci 变量的值
        {
            memcpy(&outmsg[ci], buf, rv);
            ci += rv;
        }
    }
    return ci;
}

/*
 函数用于从套接字读取一行文本。该函数有两个参数：

 readAll：表示是否读取完整行。如果为 true，则函数会一直读取直到遇到换行符 \n。如果为 false，则函数会在遇到换行符 \n 之前停止读取。
 */
string Socket::receiveLine(bool readAll)
{
    /*
     初始化变量

     函数首先初始化以下变量：

     max_wait：最大等待时间（默认为 100）
     cur_wait：剩余等待时间（初始值为 max_wait）
     msg：空字符串，用于存储接收到的行
     lastCode：上次接收到的字符（初始值为 \0）
     r：临时变量，用于存储接收到的字符
     */
    const int32_t max_wait = 100;
    int32_t cur_wait = max_wait;
    string msg;
    char lastCode = '\0';
    char r;
    
    /*
     循环读取字符

     循环会一直运行，直到发生以下情况之一：

     读取到换行符 \n
     发生超时
     在循环中，函数会执行以下操作：

     清空 r 变量
     使用 recv() 函数从套接字读取一个字符，并将其存储到 r 变量中
     检查 recv() 函数的返回值：
     如果返回值大于 0，则表示读取到一个字符。
     如果返回值小于 0，则表示发生错误或超时。
     如果返回值等于 0，则表示套接字已关闭。
     */
    
    while (1) // 读取到换行符 \n 发生超时
    {
        r = '\0';
        int32_t ret = (int)recv(_socket, &r, 1, 0); // 参数 1 的值为 1，表示只读取一个字符。参数 0 的值为 0，表示使用默认的读取模式。
        if (ret > 0)
        {
            msg += r; // 将字符 r 追加到字符串 msg 中。
            lastCode = r; // 将字符 r 赋值给变量 lastCode。
            // 如果字符 r 为换行符 \n，并且参数 readAll 的值为 false，则退出循环。
            if (r == '\n' && readAll == false) // 如果字符 r 为换行符 \n，并且参数 readAll 的值为 false，则表示接收到了一行完整的数据。此时，函数会退出循环，并返回接收到的行
            {
                break;
            }
        }
        else
        {
            if (lastCode == '\n')
            {
                /*
                 如果读取失败，则检查 lastCode 的值。如果 lastCode 为换行符 \n，则表示接收到了一行部分数据。此时，函数会退出循环，并返回接收到的部分行。
                 */
                break;
            }
            if (errno == EAGAIN)
            {
                /*
                 如果读取失败，并且 errno 的值为 EAGAIN，则表示套接字没有数据可读。此时，函数会退出循环，并返回 NULL
                 */
                break;
            }
            
            //MSLEEP(10);
            // cur_wait 变量来实现超时机制。如果 cur_wait 变量的值小于 0，则表示发生超时。此时，函数会退出循环，并打印一条错误信息。
            cur_wait = cur_wait - 1;
            if (cur_wait <= 0)
                /*
                 如果 errno 的值为 EAGAIN，则表示套接字没有数据可读。此时，函数会继续循环，等待数据到来。但为了防止无限等待，函数会在每次循环时将 cur_wait 减一。如果 cur_wait 减到 0，则表示发生超时。

                 */
            {
                printf(">> socket time out.");
                break;
            }
        }
    }
    if (r == '\n') // 如果字符 r 为换行符 \n，则表示接收到了一行完整的数据。此时，函数会打印一条日志信息，并返回接收到的行。
    {
        printf("GET LINE: %d\n", (int32_t)msg.size());
    }
    else // 如果字符 r 不为换行符 \n，则表示接收到了部分数据。此时，函数也会打印一条日志信息，但返回 NULL。

    {
        printf("GET: %d\n", (int32_t)msg.size());
    }
    fflush(stdout); // 如果不使用 fflush() 函数，则打印的日志信息可能会延迟显示。这是因为，在 printf() 函数执行完毕后，数据仍然会存储在缓冲区中。
    return msg;
}
void Socket::sendLine(const string& s) // sendLine() 函数用于向套接字发送一行数据。该函数接受一个 string 类型的参数，该参数表示要发送的数据
{
    send(_socket, s.c_str(), (int32_t)s.length(), 0);
}
void Socket::sendBytes(const char* s, int32_t msglen) // sendBytes() 函数用于向套接字发送一段字节数据。该函数接受两个参数： s：指向字节数据的指针。msglen：字节数据的长度。
{
    send(_socket, s, msglen, 0);
}

/*
  init 主要用于初始化服务器端，设置监听地址和端口，创建套接字，并将其绑定到指定的地址和端口。同时，还可以根据需要设置套接字为非阻塞模式，并指定最大连接数。 
 */
void SocketServer::init(int32_t port, int32_t connections, TypeSocket type)
{
    // 创建一个 sockaddr_in 结构体 sa，并用 memset 清空内存
    sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = PF_INET; // 设置 sa.sin_family 为 PF_INET，表示使用 IPv4 协议
    sa.sin_port = htons(port); // 设置 sa.sin_port 为 htons(port)，将端口号 port 转换为网络字节序
    _socket = socket(AF_INET, SOCK_STREAM, 0); // 使用 socket(AF_INET, SOCK_STREAM, 0) 创建一个 TCP 套接字，并将其赋值给 _socket 成员变量。
    if (_socket == -1) // 如果创建失败，抛出异常 "-1"。
    {
        throw "-1";
    }
    // 设置非阻塞模式 (可选): 如果 type 为 TypeSocket::NonBlockingSocket，则将套接字设置为非阻塞模式。
    if (type == TypeSocket::NonBlockingSocket)// 如果 type 变量的值为 TypeSocket::NonBlockingSocket
    {
        // 使用 IOCTLSOCKET(_socket, FIONBIO, &arg) 将 arg 设置为 统调用的 FIONBIO 选项。
        u_long arg = 1;
        IOCTLSOCKET(_socket, FIONBIO, &arg); // IOCTLSOCKET() 系统调用将将 arg 变量设置为 1，以将套接字设置为非阻塞模式。
    }

    // bind the socket to the internet address
    /*
     绑定套接字到监听地址和端口:

     使用 ::bind(_socket, (sockaddr*)&sa, sizeof(sockaddr_in)) 将套接字绑定到 sa 指定的地址和端口。
     如果绑定失败，关闭套接字并抛出异常 "-1"
     */
    if (::bind(_socket, (sockaddr*)&sa, sizeof(sockaddr_in)) == -1)
    {
        CLOSESOCKET(_socket);
        throw "-1";
    }
    // 使用 listen(_socket, connections) 指定该服务器同时可以接受的最大连接数为 connections。
    listen(_socket, connections);
}

/*
 
 try_accpet 用于尝试接受一个新的连接。它首先使用 accept() 函数接受连接。如果 accept() 函数返回 -1，则表示接受连接失败。

 如果 accept() 函数返回 -1，则需要检查错误码。如果错误码为 WSAEWOULDBLOCK，则表示连接请求尚未到达。在这种情况下，可以返回 nullptr，表示没有新的连接。

 如果错误码不为 WSAEWOULDBLOCK，则表示接受连接失败。在这种情况下，可以抛出异常。

 如果 accept() 函数返回非 -1，则表示接受连接成功。在这种情况下，可以创建一个新的 Socket 对象，并将其返回。
 
 */
Socket* SocketServer::try_accept()
{
    SOCKET new_sock = accept(_socket, 0, 0); // 接受连接 参数 0 和 0 表示不关心客户端地址信息。
    if (new_sock == -1)  // 检查错误

    {
#if defined(WIN32)  // 检查 Windows 平台的错误码
        int32_t rc = WSAGetLastError();
        if (rc == WSAEWOULDBLOCK)
        {// 非阻塞模式，没有请求
            return 0; // non-blocking call, no request pending
        }
        else
        {// 其他错误
            throw "Invalid Socket";
        }
#endif
        // 非 Windows 平台的错误处理
        printf("!!WARN!! accept: %d\n", (int32_t)new_sock);
        return nullptr;
    }
    // 返回新的 Socket 对象
    return new Socket(new_sock);
}

/*
 Client init_url 用于客户端连接到指定 URL 的服务器。它首先解析域名，然后设置服务器地址，最后尝试连接到服务器。如果连接成功，则打印连接成功的訊息；如果连接失败，则打印错误信息并退出程序。
 
 */
void SocketClient::init_url(const string& hostname, int32_t port)
{
    /*
     解析主机名:
     使用 gethostbyname(hostname.c_str()) 根据域名 hostname 查找其对应的主机名信息。
     如果查找失败，则抛出异常 strerror(errno)，其中 errno 保存了错误码。
     */
    hostent* he; // hostent 结构体一般存放的就是主机名及其对应的 IP 地址信息
    if ((he = gethostbyname(hostname.c_str())) == 0)
    {
        throw strerror(errno);
    }
    /*
     创建一个 sockaddr_in 结构体 addr，并进行以下设置：
     sin_family 设置为 AF_INET，表示 IPv4 协议。
     sin_port 设置为 htons(port)，将端口号 port 转换为网络字节序。
     sin_addr 复制从 he->h_addr 获取的 IP 地址信息到 addr 结构体中。
     sin_zero 填充为 0。
     */
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((in_addr*)he->h_addr);
    // addr 结构体的 sin_zero 成员变量填充为 0。
    memset(&(addr.sin_zero), 0, 8); // sin_zero 成员变量是 sockaddr_in 结构体的最后 8 个字节，用于填充结构体，以满足对齐要求，目的就是为了对齐
    
    /*
     连接到服务器:

     使用 ::connect(_socket, (sockaddr*)&addr, sizeof(sockaddr)) 尝试连接到服务器。
     _socket 是已经创建的套接字。
     (sockaddr*)&addr 将 addr 结构体转换成一个指针，指向服务器地址信息。
     sizeof(sockaddr) 指定 addr 结构体的大小。
     如果连接失败，则 ret 会返回非零值。
     */
    int32_t ret = ::connect(_socket, (sockaddr*)&addr, sizeof(sockaddr));
    if (ret)
    {
        string error;
#if defined(WIN32)
        error = strerror(WSAGetLastError());
#endif
        /*
         处理连接失败:

         如果连接失败，则根据不同的平台获取错误信息：
         在 Windows 平台上，使用 strerror(WSAGetLastError()) 获取错误信息。
         在其他平台上，直接使用 strerror(errno)。
         将错误信息打印到标准输出并退出程序。
         */
        printf("!!ERR!! connect fail: %d %s\n", ret, error.c_str());
        fflush(stdout);
        exit(1);
    }
    // 否则就是连接成功，打印连接服务器的 IP 地址和端口号到标准输出。
    printf(">> connect %s %d\n", hostname.c_str(), port);
    fflush(stdout);
}

/*
 初始化客户端连接到指定 IP 地址和端口的服务器
 */
void SocketClient::init_ipaddr(const string& ip, int32_t port)
{
    /*
     设置服务器地址:

     创建一个 sockaddr_in 结构体 addr。
     将 addr.sin_family 设置为 AF_INET，表示 IPv4 协议。
     将 addr.sin_port 设置为 htons(port)，将端口号 port 转换为网络字节序。
     使用 inet_addr(ip.c_str()) 将 IP 地址字符串 ip 转换为网络字节序并存放到 addr.sin_addr.s_addr 中。
     使用 memset(&(addr.sin_zero), 0, 8) 将 addr.sin_zero 填充为 0。
     */
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    /*
     sin_addr.s_addr 和 s_addr 都指的是 IP 地址，只是存储方式不同。sin_addr.s_addr 是网络字节序的 IP 地址，而 s_addr 是主机字节序的 IP 地址。
     */
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    memset(&(addr.sin_zero), 0, 8);
    
    /*
     连接到服务器:

     使用 ::connect(_socket, (sockaddr*)&addr, sizeof(sockaddr)) 尝试连接到服务器。
     _socket 是已经创建的套接字。
     (sockaddr*)&addr 将 addr 结构体转换成一个指针，指向服务器地址信息。
     sizeof(sockaddr) 指定 addr 结构体的大小。
     */
    int32_t ret = ::connect(_socket, (sockaddr*)&addr, sizeof(sockaddr));
    if (ret)
    {
#if defined(WIN32)
        printf("!!ERR!! connect fail: %d %s\n", ret, strerror(WSAGetLastError()));
#else
        printf("!!ERR!! connect fail: %d\n", ret);
#endif
        exit(1);
    }
    printf(">> connect %s %d\n", ip.c_str(), port);
    fflush(stdout); // printf() 函数将数据写入标准输出之前，会将数据缓存在标准输出缓冲区中。如果不刷新标准输出缓冲区，则数据不会立即显示在标准输出中
}
/*
 SocketClient 构造函数用于初始化一个基于主机名和端口的客户端连接。

 它首先调用 init_url 方法，该方法将解析主机名并将其转换为 IP 地址。然后，它将使用此 IP 地址创建一个套接字并连接到服务器。
 */
SocketClient::SocketClient(const string& hostname, int32_t port) : Socket()
{
    init_url(hostname, port); // init_url 方法来使用提供的 hostname 和 port 初始化客户端。
}
/*
 SocketSelect 构造函数用于初始化一个套接字选择器。
 套接字选择器是一种用于监视多个套接字准备就绪状态的机制。

 SocketSelect 构造函数会将 _fds 文件描述符集清空，然后将指定的套接字的文件描述符添加到集合中。然后，它将使用 select 系统调用来监视这些套接字的准备就绪状态。
 */
SocketSelect::SocketSelect(Socket const* const s1, Socket const* const s2, TypeSocket type)
{
    FD_ZERO(&_fds); // 它使用 FD_ZERO 宏来清空文件描述符集 (_fds)
    FD_SET(const_cast<Socket*>(s1)->_socket, &_fds); // 然后使用 FD_SET 将第一个套接字 (s1) 的文件描述符添加到集合中。
    if (s2) // 如果提供了第二个套接字 (s2)，则也将其文件描述符添加到集合中
    {
        FD_SET(const_cast<Socket*>(s2)->_socket, &_fds);
    }
    
    // 设置 struct timeval 对象 tval，具有 0 秒的超时和 1 微秒的延迟
    struct timeval tval;
    tval.tv_sec = 0;
    tval.tv_usec = 1;
    
    /*
     根据 type 参数，它决定是否使用超时：
     如果是 NonBlockingSocket，则将 ptval 设置为指向 tval 以进行超时。
     否则，将 ptval 设置为 0 以使用无限等待。
     */
    struct timeval* ptval;
    if (type == TypeSocket::NonBlockingSocket)
    {
        ptval = &tval;
    }
    else
    {
        ptval = 0;
    }
    /*
     它调用 select 系统调用，以下是参数：
     0：要监视的最小文件描述符号。
     &_fds：要监视可读性的文件描述符集。
     0：用于可写性的文件描述符集（未使用）。
     0：用于异常的文件描述符集（未使用）。
     ptval：超时值（或 0 表示无限等待）。
     如果 select 返回 -1，则抛出一个指示错误的异常
     
     */
    if (select(0, &_fds, (fd_set*)0, (fd_set*)0, ptval) == -1) // select() 系统调用用于监视多个文件描述符的准备就绪状态。当文件描述符准备好进行 I/O 操作时，select() 系统调用会返回。
        // 如果 select() 系统调用成功，则返回值为正数，表示有文件描述符准备好进行 I/O 操作。如果 select() 系统调用失败，则返回值为 -1，并设置 errno 变量
    {
        throw "Error in select";
    }
}
// SocketSelect::readable 方法用于检查指定的套接字是否准备好读取数据。
bool SocketSelect::readable(Socket const* const s)
{
    // 它使用 FD_ISSET 宏来检查套接字的文件描述符是否在 _fds 集中设置。如果设置了，则表示套接字有数据可读。
    // 总体而言，上述代码用于实现一个基于套接字选择器的异步 I/O 模型
    return (FD_ISSET(s->_socket, &_fds));
}
