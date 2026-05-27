#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>
#include <climits>
#include "unistd.h"
#include "agents.h"

int setup(){
    std::cout << "Starting install" << std::endl;

    const std::string target_dir =  "/usr/local/bin/";

    // Check current directory
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string absolute_binary_path = (count != -1) ? std::string(result, count) : "";

    // Move self to target dir
    std::string command = "sudo cp \"" + absolute_binary_path + "\" \"" + target_dir + "wn" + "\" && sudo chmod +x \"" + target_dir + "wn" + "\"";
    int bin_result = system(command.c_str());
    if (bin_result != 0) {
        return 1;
    }

    // Create storage files
    std::string main_folder_cmd = "mkdir ~/.wrens_nest/";
    std::string temp_folder_cmd = "mkdir ~/.wrens_nest/temp/";
    std::string data_folder_cmd = "mkdir ~/.wrens_nest/data/";

    int main_result = system(main_folder_cmd.c_str());
    int temp_result = system(temp_folder_cmd.c_str());
    int data_result = system(data_folder_cmd.c_str());

    return 0;
}

int verify_setup(std::string file_path){
    std::ifstream f(file_path.c_str());
    if (!f.good()){
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    std::string test_folder = "/usr/local/bin/wn";
    if (verify_setup(test_folder) == 1){
        if (setup() != 0) {
            std::cout << "Failed to setup program, aborting" << std::endl;
        } else {
            std::cout << "Installation finished, program is ready." << std::endl;
        }
    } else {
        std::cout << "Installation verrifyed" << std::endl;
    }

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
                return addServ(name, user, ip, keypath);
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