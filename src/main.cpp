#include <cstdio>
#include <iostream>
#include <vector>
#include "agents.h"

int main(int argc, char* argv[]) {
    std::string response;

    if (argc > 1){
        std::string main_arg = argv[1];

        if (main_arg == "-h" || main_arg == "--help"){
            response = R"(
        -h or --help   : Help Menu
        -m or --manage : Manage servers
        -s or --status : Status quick view

        Uses:
            wn -m [action] [options]

        Manage:
            wn -m add [name] [user] [ip] [key] 
                Name : Users choosen name for the server
                User : Username for server login
                IP   : IP of server
                Key  : Path to private key on users computer (Optional)
            
            wn -m start/stop [selection 1] [selection 2] [etc]
                selection : List servers to start or stop seperated by spaces (ex. dev_server web_server).
            )";
        }

        if (main_arg == "-m" || main_arg == "--manage"){
            std::string action = argv[2];
            if (action == "add" || action == "a"){
                std::string name = argv[3];
                std::string user = argv[4];
                std::string ip = argv[5];
                std::string keypath = argv[6];
                addServ(name, user, ip, keypath);
            }

            if (action == "start") {
                std::vector<std::string> selections;
                for (int i = 3; i < argc; i++){
                    selections.push_back(argv[i]);
                    //add start logic
                }
            }
        }
    }
    
    std::cout << response << std::endl;
    return 0;
}