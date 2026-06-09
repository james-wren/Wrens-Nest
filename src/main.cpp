#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>
#include <climits>
#include "unistd.h"
#include "agents.h"
#include "json.hpp"
#include "monitor.h"
#include "tui.h"
#include "main.h"

using json = nlohmann::json;

const std::string HOME_DIR = getHome();

int setup(){
    std::cout << "Starting install" << std::endl;

    const std::string target_dir =  "/usr/local/bin/";

    // Check current directory
    char result[PATH_MAX];
    size_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string absolute_binary_path = (count != -1) ? std::string(result, count) : "";

    // Create storage files
    std::string main_folder_cmd = "mkdir ~/.wrens_nest/";
    std::string temp_folder_cmd = "mkdir ~/.wrens_nest/temp/";
    std::string data_folder_cmd = "mkdir ~/.wrens_nest/data/";

    int main_result = system(main_folder_cmd.c_str());
    int temp_result = system(temp_folder_cmd.c_str());
    int data_result = system(data_folder_cmd.c_str());

    cpr::Response r = cpr::Get(
        cpr::Url{"https://localhost:1690/register"},
        cpr::Ssl(
            cpr::ssl::VerifyHost{false},
            cpr::ssl::VerifyPeer{false}
        )
    );

    if (r.status_code != 200) {
        std::cout << "Proxy registration failed with status code: " << r.status_code << std::endl;
        return 1;
    }

    json uid = json::parse(r.text);
    int uid_num = uid["uid"];

    json info;
    info["uid"] = uid_num;
    std::ofstream infoFile(HOME_DIR + "/.wrens_nest/data/info.json");
    if (infoFile.is_open()){
        infoFile << info.dump();
        infoFile.close(); 
    }

    std::string details = "DO NOT MODIFY, this is the system configuration file, all details are crucial to this programs function";
    json data;
    data["details"] = details;

    std::ofstream dataFile(HOME_DIR + "/.wrens_nest/data/servers.json");
    if (dataFile.is_open()){
        dataFile << data.dump();
        dataFile.close();
    }

    // Move self to target dir, made this the final step as it also serves as the installation check
    std::string command = "sudo cp \"" + absolute_binary_path + "\" \"" + target_dir + "wn" + "\" && sudo chmod +x \"" + target_dir + "wn" + "\"";
    int bin_result = system(command.c_str());
    if (bin_result != 0) {
        return 1;
    }

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
            return 1;
        } else {
            std::cout << "Installation finished, program is ready." << std::endl;
            return 0;
        }
    } else {
        std::cout << "Installation verrified" << std::endl;
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
            wn
            wn -m [action] [options]

        Manage:
            wn -m add [name] [user] [ip] [key] 
                Name : Users choosen name for the server
                User : Username for server login
                IP   : IP of server
                Key  : Path to private key on users computer (Optional)
            
            wn -m start/stop [selections]
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
                    std::string name = argv[i];
                    startServer(name);
                }
            }
        }
    } else {
        std::string jsonPath = HOME_DIR + "/.wrens_nest/data/servers.json";
        std::ifstream jsonFile(jsonPath.c_str());
        
        json serverData = json::parse(jsonFile);

        // for (auto& [key, value] : serverData.items()){
        //     if (key != "details"){
        //         std::list<std::string> selections = {"status"};
        //         getStats(selections, key);
        //     }
        // }

        homeScreen();
    }
    std::cout << response << std::endl;
    return 0;
}