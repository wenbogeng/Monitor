#include <iostream>
#include <fstream>
#include <sstream>

#include "Server.hpp"

using namespace std;

Server::Server() {}

uint16_t getRandomPort() { return rand()%10000 + 50000; }

void Server::init(int32_t port, int32_t maxConnects, TypeSocket type) {
    _serverSock.init(port, maxConnects, type);
    
    _listenFd = _serverSock.GetSocket();
    
    // int keepAlive = 1;
    // setsockopt(_listenFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
    // int opt = 1;
    // setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
}

Server::~Server() {}

static vector<string> STLStringToTokens(string const& str,  char delim, bool allowEmpty) {
    stringstream ss(str);
    
    string line;
    
    vector<string> tokens;
    
    while (getline(ss, line, delim)) {
        if (line.empty() && !allowEmpty) continue;
        
        tokens.push_back(line);
    }
    
    return tokens;
}

string getTime() {
    time_t timep;
    struct tm *p;
    
    time(&timep); //获取从1970至今过了多少秒，存入time_t类型的timep
    p = localtime(&timep);//用localtime将秒数转化为struct tm结构体
    
    char ctime[50];
    snprintf(ctime, sizeof(ctime), "%d.%d.%d %02d:%02d:%02d",
             1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
             p->tm_hour, p->tm_min, p->tm_sec);
    return ctime;
}

static string removeLB(const string &s) {
    string res = s;
    if (res.back() == '\n')
        res.pop_back();
    return res;
}

void printPromt() { cout << "\nPlease Enter the Command: " << flush; }

static size_t getFileSize(const char *file_name) {
    
    if (file_name == NULL) {
        return 0;
    }
    
    // 这是一个存储文件(夹)信息的结构体，其中有文件大小和创建时间、访问时间、修改时间等
    struct stat statbuf;
    
    // 提供文件名字符串，获得文件属性结构体
    stat(file_name, &statbuf);
    
    // 获取文件大小
    size_t filesize = statbuf.st_size;
    
    return filesize;
}

void* processClient(void* arg) {
    SOCKET* s = (SOCKET*)arg;
    Socket sk(*s);
    string name = "";
    char buf[MSG_MAX_LENGTH];
    while (true) {
        // memset(buf, 0, sizeof(buf));
        // int rv = recv(*s, buf, sizeof(buf), 0);
        string rev = sk.receiveLine(false);
        if (rev.empty()) {
            {
                lock_guard<mutex> lck(sv._lock);
                if (name != "" && sv._name2controlSocket.find(name) != sv._name2controlSocket.end()) {
                    sv._name2controlSocket.erase(name);
                }
            }
            sk.close_sock();
            delete s;
            return NULL;
        }
        // cout << buf <<endl;
        rev = removeLB(rev);
        vector<string> pms = STLStringToTokens(rev, ' ', false);
        if (pms[0] == "USER") {
            lock_guard<mutex> lck(sv._lock);
            if (sv._name2controlSocket.find(pms[1]) == sv._name2controlSocket.end()) {
                // strcpy(buf, "331\n");
                // ret = send(*s, buf, strlen(buf), 0);
                sk.sendLine("331\n");
            } else {
                // strcpy(buf, "404\n");
                // ret = send(*s, buf, strlen(buf), 0);
                sk.sendLine("404\n");
                continue;
            }
            name = pms[1];
            cout << endl << name << " at " << getTime() << "Connected to the Server" <<endl;
            sv._name2controlSocket.insert(make_pair(pms[1], *s));
            printPromt();
        } else if (pms[0] == "PASV") {
            if (name == "") {
                sk.sendLine("400 you have not login\n");
                continue;
            }
            uint16_t port;
            SocketServer ss;
            while (true) {
                try {
                    port = getRandomPort();
                    ss.init(port, 100);
                    break;
                } catch (...) {
                }
            }
            snprintf(buf, sizeof(buf) ,"227 %d\n", port);
            // cout <<"Port: "<<port<<endl;
            // ret = send(*s, buf, sizeof(buf), 0);
            sk.sendLine(buf);
            SOCKET ssk = ss.GetSocket();
            struct sockaddr_in client_addr;
            uint32_t addr_len = sizeof(struct sockaddr_in);
            SOCKET tmp = accept(ssk, (struct sockaddr*)&client_addr, &addr_len);
            if (tmp > 0) {
                lock_guard<mutex> lck(sv._lock);
                if (sv._name2dataSocket.find(name) == sv._name2dataSocket.end()) {
                    sv._name2dataSocket.insert(make_pair(name, tmp));
                } else {
                    sv._name2dataSocket[name] = tmp;
                }
            }
        }
    }
}

int getReplyCode(const string &reply) {
    try {
        return stoi(reply.substr(0, 3));
    } catch (...) {
        return -1;
    }
}

void* processInput(void* arg) {
    Server* serv = (Server*)arg;
    cout <<"        _________________________________________________________  " << endl
    << "        |                FTP Command List                       |   " << endl
    << "        | 1. get <remote_path> <local_path>                     |   " << endl
    << "        | 2. put <local_path> <remote_path>                     |   " << endl
    << "        | 3. ls <remote_directory>                              |   " << endl
    << "        | 4. ? or help (Display this help message)              |   " << endl
    << "        | 5. quit (Quit the FTP client)                         |   " << endl
    << "        |_______________________________________________________|    " << endl;
    
    Socket sk(0);
    Socket data_sk(0);
    while (true) {
        string request;
        string line;
        printPromt();
        getline(cin, line);
        vector<string> pms = STLStringToTokens(line, ' ', false);
        if (pms.size() < 3) {
            cerr << "Input Error" <<endl;
            continue;
        }
        {
            lock_guard<mutex> lck(serv->_lock);
            if (serv->_name2controlSocket.find(pms[1]) == serv->_name2controlSocket.end()) {
                cerr << "User doesn't Exit" <<endl;
                if (pms[0] == "ls") {
                    cerr << "ls Command Failed" << endl;
                } else {
                    cerr << pms[0] << "File Transfer Failed, Please Check whether the User Name, Path or Other Information Exist" << endl;
                }
                continue;
            }
            sk = Socket(serv->_name2controlSocket[pms[1]]);
        }
        if (pms[0] == "ls") {
            if (pms.size() == 3) {
                string tmp;
                request = pms[0] + " " + pms[2] + "\n";
                sk.sendLine(request);
                while (serv->_name2dataSocket.find(pms[1]) == serv->_name2dataSocket.end()) { usleep(200); }
                {
                    lock_guard<mutex> lck(serv->_lock);
                    data_sk = Socket(serv->_name2dataSocket[pms[1]]);
                }
                while(true) {
                    string res = data_sk.receiveLine(false);
                    if (res.empty()) {
                        break;
                    }
                    cout<<res<<flush;
                }
                data_sk.close_sock();
                {
                    lock_guard<mutex> lck(serv->_lock);
                    serv->_name2dataSocket.erase(pms[1]);
                }
                continue;
            }
        } else if (pms[0] == "get") {
            if (pms.size() == 4) {
                string tmp;
                request = pms[0] + " " + pms[2] + "\n";
                sk.sendLine(request);
                while (serv->_name2dataSocket.find(pms[1]) == serv->_name2dataSocket.end()) { usleep(200); }
                {
                    lock_guard<mutex> lck(serv->_lock);
                    data_sk = Socket(serv->_name2dataSocket[pms[1]]);
                }
                int fsize = stoi(removeLB(data_sk.receiveLine(false)));
                if (fsize < 0) {
                    cerr << "Client File does not Exist: " << pms[2] << endl;
                    cerr << pms[0] << "File Transfer Failed, Please Check whether the User Name, Path or other Information Exist" << endl;
                    data_sk.close_sock();
                    {
                        lock_guard<mutex> lck(serv->_lock);
                        serv->_name2dataSocket.erase(pms[1]);
                    }
                    continue;
                }
                string fname = pms[3] + "/" + basename((char*)pms[2].c_str());
                ofstream fout(fname, ios_base::binary);
                if (!fout) {
                    cerr << "Server Path does not Exist: " << pms[3] << endl;
                    cerr << pms[0] << "File Transfer Failed, Please Check whether the User Name, Path or other Information Exist"  << endl;
                    data_sk.close_sock();
                    {
                        lock_guard<mutex> lck(serv->_lock);
                        serv->_name2dataSocket.erase(pms[1]);
                    }
                    continue;
                }
                // cout << "fsize: " << fsize << ", fname: " << fname << endl;
                char *buf = new char[fsize];
                data_sk.receiveBytes(buf, fsize);
                fout.write(buf, fsize);
                fout.close();
                data_sk.close_sock();
                {
                    lock_guard<mutex> lck(serv->_lock);
                    serv->_name2dataSocket.erase(pms[1]);
                }
                cout << "get File Transfer Successful" << endl;
                continue;
            }
        } else if (pms[0] == "put") {
            if (pms.size() == 4) {
                ifstream fin(pms[2], ios_base::binary);
                if (!fin) {
                    cerr << "Failed to Open Server File: " << pms[2]<<endl;
                    cerr << pms[0] << "File Transfer Failed, Please Check whether the User Name, Path or other Information Exist" << endl;
                    data_sk.close_sock();
                    {
                        lock_guard<mutex> lck(serv->_lock);
                        serv->_name2dataSocket.erase(pms[1]);
                    }
                    continue;
                }
                string tmp;
                string cfname = pms[3] + "/" + basename((char*)pms[2].c_str());
                request = pms[0] + " " + cfname + "\n";
                sk.sendLine(request);
                while (serv->_name2dataSocket.find(pms[1]) == serv->_name2dataSocket.end()) { usleep(200); }
                {
                    lock_guard<mutex> lck(serv->_lock);
                    data_sk = Socket(serv->_name2dataSocket[pms[1]]);
                }
                string res = data_sk.receiveLine(false);
                if (getReplyCode(res) != 200) {
                    cerr << "Failed to Open Client File: " << cfname <<endl;
                    cerr << pms[0] << "File Transfer Failed, Please Check whether the User Name, Path or other Information Exist"  << endl;
                    data_sk.close_sock();
                    {
                        lock_guard<mutex> lck(serv->_lock);
                        serv->_name2dataSocket.erase(pms[1]);
                    }
                    continue;
                }
                // Send file size
                {
                    ostringstream oss;
                    int fsize = (int32_t)getFileSize(pms[2].c_str());
                    oss << fsize;
                    data_sk.sendLine(oss.str() + "\n");
                }
                while(true) {
                    if (fin.eof()) break;
                    char buf[MSG_MAX_LENGTH];
                    fin.read(buf, MSG_MAX_LENGTH);
                    data_sk.sendBytes(buf, (int32_t)fin.gcount());
                }
                fin.close();
                data_sk.close_sock();
                {
                    lock_guard<mutex> lck(serv->_lock);
                    serv->_name2dataSocket.erase(pms[1]);
                }
                cout << "put File Transfer Successful" << endl;
                continue;
            }
        }
        cerr << "Invalid Input" <<endl;
        // lock_guard<mutex> lck(serv->_lock);
    }
}

void Server::Run() {
    THREAD_HANDLE pThread;
    int ret = pthread_create(&pThread, NULL, processInput, this);
    struct sockaddr_in client_addr;
    uint32_t addr_len = sizeof(struct sockaddr_in);
    
    while (true) {
        SOCKET* new_sock = new SOCKET;
        *new_sock = accept(_listenFd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (*new_sock == -1) return;
        
        ret = pthread_create(&pThread, NULL, processClient, new_sock);
    }
}

Server sv;
