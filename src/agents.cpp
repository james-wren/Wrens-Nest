#include <cstdio>
#include <iostream>
#include <fstream>
#include <json.hpp>
#include <cstdlib>
#include "embedded/http_parser_py.h"

using json = nlohmann::json;

int getServerNum(){
    return 5; // CHANGE
}

std::string getHome(){
    const char* homeDir = std::getenv("HOME");
    return homeDir ? std::string(homeDir) : "";
}

// Function to execute a command over ssh
std::string remoteExec(std::string cmd, std::string key_path, std::string user, std::string ip){
    std::string full_cmd = "ssh -i " + key_path + " " + user + "@" + ip + " " + " '" + cmd + "'";
    int result =  system(full_cmd.c_str());
    return result == 0  ? "Success " : "Failed " + full_cmd;
}

// Function to send a file to a server
std::string scpServ(std::string file_path, std::string loco_path, std::string ip, std::string user, std::string key_path) {
    std::string cmd = "scp -i " + key_path + " " + file_path + " " + user + "@" + ip + ":" + loco_path;
    int result = system(cmd.c_str());
    return result == 0 ? "Success " : "Failed " + cmd;
}

int addServ(std::string name, std::string username, std::string ip, std::string key_path) {
    const std::string HOME_DIR = getHome();

    // Create json for server
    int serverNum = getServerNum() + 1;
    json config;
    config["id"] = serverNum;
    config["name"] = name;

    // Save config to temp file
    std::ofstream configFile(HOME_DIR + "/.wrens_nest/temp/agent_config_transfer.json");
    if (configFile.is_open()) {
        configFile << config;
        configFile.close();
    }

    // Take python script and save it to file for transfer
    std::ofstream agentFile(HOME_DIR + "/.wrens_nest/temp/agent.py");
    if (agentFile.is_open()){
        agentFile << _home_tag_Documents_Server_Manager_scripts_http_parser_py;
        agentFile.close();
    }

    // Create directory for incoming files on server
    std::cout << remoteExec("mkdir -p ~/.wrens_nest/", key_path, username, ip) << std::endl;
    // Transfer files to server
    std::cout << scpServ("~/.wrens_nest/temp/agent_config_transfer.json", "~/.wrens_nest/", ip, username, key_path) << std::endl;
    std::cout << scpServ("~/.wrens_nest/temp/agent.py", "~/.wrens_nest/", ip, username, key_path) << std::endl;

    // Read current saved server data and save it as a variable to add to
    std::ifstream f( HOME_DIR + "/.wrens_nest/data/servers.json");
    json serverData = json::parse(f);

    // Adds new server data to the json data
    std::string serverNumStr = std::to_string(serverNum);
    serverData[serverNumStr] = {};
    serverData[serverNumStr]["name"] = config["name"];

    // Updates the file with new data
    std::ofstream serverDataFile(HOME_DIR + "/.wrens_nest/data/servers.json");
    if (serverDataFile.is_open()){
        serverDataFile << serverData;
        serverDataFile.close();
    }

    return 0;
}