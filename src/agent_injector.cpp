// #include <cstdio>
// #include <iostream>
// #include <fstream>
// #include <json.hpp>
// #include <cstdlib>

// using json = nlohmann::json;

// int getServerNum(){
//     return 5; //CHANGE
// }

// std::string scpServ(std::string file_path, std::string loco_path, std::string ip, std::string user, std::string key_path) {
//     std::string cmd = "scp -i " + key_path + " " + file_path + " " + user + "@" + ip + ":" + loco_path;

//     FILE* pipe = popen(cmd.c_str(), "r");
//     if (!pipe) {
//         return "Failed to open terminal";
//     }

//     pclose(pipe);
//     return "Sucessufly performed scp";
// }

// std::string addServ(std::string username, std::string ip, std::string name, std::string key_path) {
//     json config;
//     config["id"] = getServerNum();
//     config["name"] = name;

//     std::ofstream file("temp/agent_config_transfer.json");
//     if (file.is_open()) {
//         file << config;
//         file.close();
//     }

//     std::cout << scpServ("temp/agent_config_transfer.json", "location", ip, username, key_path) << std::endl;

//     return "Succesfully added server";
// }

// int main() {
//     std::cout << addServ("tag", "192.98.62.14", "dev server", "keypath/example") << std::endl;
//     return 0;
// }