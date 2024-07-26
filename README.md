### High Level:

FTP 被动模式，客户端启动后主动发起连接，等待服务器端指令，根据启动加载其他功能模块（可以是线程或新进程）传输文件 和 client 的在线状况。

客户端主动连接服务器，1 个服务器控制多个客户端

### Low Level:

***已有 Socket.h 和 Socket.cpp***

【目前没有考虑】一次上传多个文件的情况

**具体交互如下:**

1. 客户端连接到 FTP 服务器的控制端口 (21) 或者其他端口因为默认的是 21。
2. 客户端发送 PASV 命令，告诉服务器使用被动模式 eg: 220, Service ready for new user 
3. 服务器随机选择一个非特权端口，并将该端口号发送给客户端。
4. 客户端使用随机选择的端口连接到服务器的数据端口。

controlSocket 负责控制指令 

dataSocket 负责传输文件 

链接建立后，但是由 Server 发Shell 指令给 Client，Client 返回指令结果给 Server，1 个 Server 可以控制多个 Client，用线性表去映射 <client_username, socket> 

**Server:** 

【接受用户指令发送给客户端】Server 相当于只负责 Send

shell 调用 linux 系统指令 处理固定命令: 
ls remotepath
put localfile remotepath
get remotefile

限制条件：文件传输从小到大传，太大的文件可以另起一个线程。

【多个客户端的控制】

1 Server 对 多个 Client 目前使用的是 多线程，因为自己本地测试了 Socket.cpp 里的 Linux selector 运行的不流畅

【监控】

显示客户端在线状态

显示客户端汇报的进程状态 *待实现*
【汇报内容可以变动】
可用 json格式

msgtype: x
msglen: x
msg:  x

【output】 例子：

$ ./bin/server.exe 

user1 就是 client 1 user2 就是 client 2 就是服务器去控制这些客户端 去做文件的传输 <client_username, socket> 

The server is listening... 
user1|get|trade.log|./本地保存地址/
user2|put|./price.csv|/home/appadmin/data/
exec|orders.csv

**Client:** 

【客户端执行接受服务器命令，执行完返回给服务器】Client 相当于只负责 Recv