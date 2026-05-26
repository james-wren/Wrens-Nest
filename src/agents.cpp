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
    json config;
    config["id"] = getServerNum();
    config["name"] = name;

    std::ofstream configFile("temp/agent_config_transfer.json");
    if (configFile.is_open()) {
        configFile << config;
        configFile.close();
    }

    std::ofstream agentFile("agent.py");
    if (agentFile.is_open()){
        agentFile << _home_tag_Documents_Server_Manager_scripts_http_parser_py;
        agentFile.close();
    }

    std::cout << remoteExec("mkdir -p ~/.wrens_nest/", key_path, username, ip) << std::endl;
    std::cout << scpServ("temp/agent_config_transfer.json", "~/.wrens_nest/", ip, username, key_path) << std::endl;
    std::cout << scpServ("agent.py", "~/.wrens_nest/", ip, username, key_path) << std::endl;

    return "Succesfully added server";
}
