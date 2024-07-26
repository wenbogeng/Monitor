//
//  Client.cpp
//  Monitor
//
//  Created by Wenbo Geng on 1/8/24.
//

#include <iostream>
#include <sstream>
#include <fstream>

#include "Client.hpp"

using namespace std;

Client::Client(const string& name, const std::string& ip, int32_t port) {
    _username = name;
    _ip = ip;
    _clientSock.init_ipaddr(_ip, port);
}

int Client::getReplyCode(const string &reply) {
    try {
        return stoi(reply.substr(0, 3));
    } catch (...) {
        return -1;
    }
}

bool Client::Login() {
    string request;
    string response;
    
    request = "USER " + _username + "\n";
    _clientSock.sendLine(request);
    response = _clientSock.receiveLine(false);
    if (getReplyCode(response) != 331) {
        cerr << "Username error: " <<response.substr(4)<<endl;
        return false;
    } else
        cout << "Login success"<<endl;
    
    return true;
}

bool Client::SetPasvMode() {
    string request = "PASV\n";
    string response;
    _clientSock.sendLine(request);
    response = _clientSock.receiveLine(false);
    if (getReplyCode(response) != 227) {
        cerr << "Set PASV mode failed, reply code " << getReplyCode(response) << endl;
        return false;
    }
    try {
        int port = stoi(response.substr(4));
        usleep(500);
        _dataSock = new SocketClient();
        _dataSock->init_ipaddr(_ip, port);
    } catch (...) {
        cerr << "PASV connect server failed" <<endl;
        return false;
    }
    return true;
}

static size_t getFileSize(const char *file_name) {
    
    if (file_name == NULL) return 0;
    // 这是一个存储文件(夹)信息的结构体，其中有文件大小和创建时间、访问时间、修改时间等
    struct stat statbuf;
    
    // 提供文件名字符串，获得文件属性结构体
    stat(file_name, &statbuf);
    // 获取文件大小
    size_t filesize = statbuf.st_size;
    
    return filesize;
}

static string int2str(int n) {
    ostringstream oss;
    oss << n;
    return oss.str();
}

void Client::SendFile(const string &path) {
    ifstream fin(path, ios_base::binary);
    if (!fin) {
        _dataSock->sendLine("-1\n");
        _dataSock->close_sock();
        delete _dataSock;
        return;
    }
    // Send file size
    {
        ostringstream oss;
        size_t fsize = getFileSize(path.c_str());
        oss<<fsize;
        _dataSock->sendLine(oss.str() + "\n");
    }
    
    // Send file content
    char buf[MSG_MAX_LENGTH];
    memset(buf, 0, MSG_MAX_LENGTH);
    while (!fin.eof()) {
        fin.read(buf, MSG_MAX_LENGTH);
        size_t rsize = fin.gcount();
        _dataSock->sendBytes(buf, (int32_t)rsize);
    }
    fin.close();
    _dataSock->close_sock();
    delete _dataSock;
}

static string removeLB(const string &s) {
    string res = s;
    if (res.back() == '\n')
        res.pop_back();
    return res;
}

void Client::ReceiveFile(const string &path) {
    ofstream fout(path, ios_base::binary);
    if (!fout) {
        cerr << "Open file failed: " << path << endl;
        _dataSock->sendLine("400\n");
        return;
    }
    _dataSock->sendLine("200\n");
    try {
        int fsize = stoi(removeLB(_dataSock->receiveLine(false)));
        char *buf = new char[fsize];
        int rsize =_dataSock->receiveBytes(buf, fsize);
        fout.write(buf, rsize);
        fout.close();
        _dataSock->close_sock();
        delete _dataSock;
    } catch (...) {
        return;
    }
}


void Client::SendList(const string &path) {
    // string cmd = string("ls -l --time-style=long-iso ") + path + " | awk '{if(NR>1){printf \"%s %s %s %s\\n\", $6, $7, $5, $8}}'";
    string cmd = string("ls -l --time-style=long-iso ") + path;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        cerr << "Popen failed: " << cmd <<endl;
        return;
    }
    string response;
    char buf[MSG_MAX_LENGTH];
    while(fgets(buf, sizeof(buf), pipe) != NULL) {
        response += buf;
    }
    pclose(pipe);
    if (response.empty())
        response = "ls Error: File or Path does not Exist\n";
    cout << "Send: " << response<<endl;
    _dataSock->sendLine(response);
    _dataSock->close_sock();
    delete _dataSock;
}

void Client::Run() {
    if (!Login()) return;
    
    while (true) {
        string receive = _clientSock.receiveLine(false);
        if (receive.empty()) {
            _clientSock.close_sock();
            return;
        }
        cout << "Receive message: " << receive << endl;
        vector<string> pm =  Client::STLStringToTokens(receive, ' ', false);
        if (pm[0] == "get") {
            if (!SetPasvMode())
                return;
            
            SendFile(pm[1]);
        } else if (pm[0] == "put") {
            if (!SetPasvMode())
                return;
            
            ReceiveFile(pm[1]);
        } else if (pm[0][0] == 'l' && pm[0][1] == 's') {
            if (!SetPasvMode())
                return;
            SendList(pm[1]);
        } else {
            cerr << "Invalid command: " <<pm[0]<<endl;
        }
    }
}

void Client::readConfig(string &name, string &ip, int &port) {
    ifstream fin("config.txt");
    if (!fin)
        cerr << "Can't open configure file config.txt"<<endl;
    fin>>name>>ip>>port;
    fin.close();
}

string eraseSpace(const string &str) {
    string res;
    for (int i = 0; i < str.size(); ++i) {
        if ((int)str[i] != 0 && !isspace(str[i]))
            res.push_back(str[i]);
    }
    return res;
}

vector<string> Client::STLStringToTokens(string const& str,  char delim, bool allowEmpty) {
    string line;
    vector<string> tokens;
    
    int i, j;
    for (i = j = 0; i < str.size(); ++i) {
        if (str[i] == delim) {
            if (i != j) {
                line = "";
                for (int k = j; k < i; ++k)
                    line.push_back(str[k]);
                tokens.push_back(eraseSpace(line));
            }
            j = i+1;
        }
    }
    if (i != j) {
        line = "";
        for (int k = j; k < i; ++k)
            line.push_back(str[k]);
        tokens.push_back(eraseSpace(line));
    }
    
    return tokens;
}
