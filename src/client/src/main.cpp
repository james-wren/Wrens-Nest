#include <cstdio>
#include <iostream>

std::string runCurl(const std::string& url) {
    std::string cmd = "curl -s " + url;

    FILE* pipe = popen(cmd.c_str(), "r");
    if(!pipe) return "";

    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        result += buffer;

    pclose(pipe);
    return result;
}

int main(int argc, char* argv[]) {
    std::string response;

    if (argc > 1){
        response = runCurl("localhost:1690?action=" + std::string(argv[1]));
    } else {
        response = runCurl("localhost:1690");
    }
    
    std::cout << response << std::endl;
    return 0;
}