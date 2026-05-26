#include <cstdio>
#include <iostream>
#include <fstream>
#include <json.hpp>
#include <cstdlib>
#include "embedded/http_parser_py.h"

using json = nlohmann::json;

int getServerNum(){
    return 5; //CHANGE
}

std::string remoteExec(std::string cmd, std::string key_path, std::string user, std::string ip){
    std::string full_cmd = "ssh -i " + key_path + " " + user + "@" + ip + " " + " '" + cmd + "'";
    int result =  system(full_cmd.c_str());
    return result == 0  ? "Success " : "Failed " + full_cmd;
}

std::string scpServ(std::string file_path, std::string loco_path, std::string ip, std::string user, std::string key_path) {
    std::string cmd = "scp -i " + key_path + " " + file_path + " " + user + "@" + ip + ":" + loco_path;
    int result = system(cmd.c_str());
    return result == 0 ? "Success " : "Failed " + cmd;
}

std::string addServ(std::string name, std::string username, std::string ip, std::string key_path) {
    int serverNum = getServerNum() + 1;
    json config;
    config["id"] = serverNum;
    config["name"] = name;

    std::ofstream configFile("temp/agent_config_transfer.json");
    if (configFile.is_open()) {
        configFile << config;
        configFile.close();
    }

    std::ofstream agentFile("temp/agent.py");
    if (agentFile.is_open()){
        agentFile << _home_tag_Documents_Server_Manager_scripts_http_parser_py;
        agentFile.close();
    }

    std::cout << remoteExec("mkdir -p ~/.wrens_nest/", key_path, username, ip) << std::endl;
    std::cout << scpServ("temp/agent_config_transfer.json", "~/.wrens_nest/", ip, username, key_path) << std::endl;
    std::cout << scpServ("temp/agent.py", "~/.wrens_nest/", ip, username, key_path) << std::endl;

    std::ifstream f("data/servers.json");
    json serverData = json::parse(f);

    std::string serverNumStr = std::to_string(serverNum);
    serverData[serverNumStr] = {};
    serverData[serverNumStr]["name"] = config["name"];

    std::ofstream serverDataFile("data/servers.json");
    if (serverDataFile.is_open()){
        serverDataFile << serverData;
        serverDataFile.close();
    }

    return "Succesfully added server";
}