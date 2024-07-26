//
//  main.cpp
//  Monitor_Server
//
//  Created by Wenbo Geng on 1/8/24.
//

#include <iostream>
#include "Server.hpp"

int main(int argc, const char * argv[]) {
    // insert code here...
//    Server sv;

    if (argc != 2) {
        cerr << "Please run: server [port_number]" << endl;
        return 0;
    }

    sv.init(atoi(argv[1]), 100);
    sv.Run();
    std::cout << "Hello, World!\n";
    return 0;
}
