// #include <cstdio>
// #include <iostream>

// std::string runCurl(const std::string& url) {
//     std::string cmd = "curl -s " + url;

//     FILE* pipe = popen(cmd.c_str(), "r");
//     if(!pipe) return "";

//     char buffer[256];
//     std::string result;
//     while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
//         result += buffer;

//     pclose(pipe);
//     return result;
// }