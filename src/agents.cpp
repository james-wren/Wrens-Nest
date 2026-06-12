#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <filesystem>

#include "json.hpp"
#include "main.h"
#include "encryption.h"
#include "embedded/agent_go.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

std::string getHome(){
    const char* homeDir = std::getenv("HOME");
    return homeDir ? std::string(homeDir) : "";
}

const std::string HOME_DIR = getHome();

int getServerNum(){
    std::ifstream serverData(HOME_DIR + "/.wrens_nest/data/servers.json");
    json data = json::parse(serverData);
    int size = static_cast<int>(data.size());
    return size;
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

// Refactoring code to this order
// 1. Prep remote server with directorys
// 2. Create json variables and generate neccesary values
// 3. Save remoteley
// 4. Save locally

int addServ(std::string name, std::string username, std::string ip, std::string key_path) {
    // Create directory for incoming files on server
    std::cout << remoteExec("mkdir -p ~/.wrens_nest/", key_path, username, ip) << std::endl;

    const std::string AES_KEY = keyGen(name);

    std::ifstream infoFile((HOME_DIR + "/.wrens_nest/data/info.json").c_str());
    json info = json::parse(infoFile);

    // Create json for server
    int serverNum = getServerNum();
    json config;
    config["id"] = serverNum;
    config["name"] = name;
    config["key"] = AES_KEY;
    config["user"] = username;
    config["ip"] = ip;
    config["ssh_key"] = key_path;
    config["uid"] = info["uid"];
    
    // Read current saved server data and save it as a variable to add to
    // Im done debugging ts, time to write the most bulletproof fucking code ever.
    std::ifstream f(HOME_DIR + "/.wrens_nest/data/servers.json");
    json serverData = json::parse(f);
    
    // Adds new server data to the json data
    serverData[name] = {};
    serverData[name]["id"] = serverNum;
    serverData[name]["user"] = username;
    serverData[name]["ip"] = ip;
    serverData[name]["key"] = AES_KEY;
    serverData[name]["ssh_key"] = key_path;
    

    // Updates the file with new data
    std::ofstream serverDataFile(HOME_DIR + "/.wrens_nest/data/servers.json");
    if (serverDataFile.is_open()){
        serverDataFile << serverData.dump();
        serverDataFile.close();
    }

    // Save config to temp file
    std::ofstream configFile(HOME_DIR + "/.wrens_nest/temp/agent_config_transfer.json");
    if (configFile.is_open()) {
        configFile << config;
        configFile.close();
    }

    // Take go script and save it to file for transfer
    std::ofstream agentFile(HOME_DIR + "/.wrens_nest/temp/agent", std::ios::out | std::ios::binary);
    if (agentFile.is_open()){
        agentFile.write(reinterpret_cast<const char*>(agent), agent_len);
        agentFile.close();
    }
    // Transfer files to server
    std::cout << scpServ(HOME_DIR + "/.wrens_nest/temp/agent_config_transfer.json", "~/.wrens_nest/", ip, username, key_path) << std::endl;
    std::cout << scpServ(HOME_DIR + "/.wrens_nest/temp/agent", "~/.wrens_nest/", ip, username, key_path) << std::endl;

    // Remove temporary files
    fs::path config_temp = HOME_DIR + "/.wrens_nest/temp/agent_config_transfer.json";
    fs::path script_temp = HOME_DIR + "/.wrens_nest/temp/agent";
    fs::remove(config_temp);
    fs::remove(script_temp);

    return 0;
}

int startServer(std::string name){
    std::ifstream f(HOME_DIR + "/.wrens_nest/data/servers.json");
    json serverData = json::parse(f);
    json server = serverData[name];

    std::cout << "starting server: " + name << std::endl;

    remoteExec("[Command to start server]", server["ssh_key"], server["user"], server["ip"]);

    return 0;
}