#include "main.h"
#include "agents.h"
#include "encryption.h"

using json = nlohmann::json;
const std::string HOME_DIR = getHome();

std::string getStats(std::list<std::string> selections, std::string name){
    std::ifstream jsonFile(HOME_DIR + "/.wrens_nest/data/servers.json");
    json serverData = json::parse(jsonFile);

    json request;
    request["get"] = selections;

    std::vector<uint8_t> key = decodeBase64(serverData[name]["key"]);
    auto [iv, encrypted_text] = encryptAES(request.dump(), key);

    std::string b64_iv = encodeBase64(iv);
    std::string b64_bodytext = encodeBase64(encrypted_text);

    json http_body;
    http_body["iv"] = b64_iv;
    http_body["text"] = b64_bodytext;

    cpr::Response r = cpr::Post(
        cpr::Url{serverData[name]["ip"].get<std::string>() + ":1690"},
        cpr::Body{http_body.dump()},
        cpr::Header{{"Content-Type", "application/json"}}
    );

    return "no errors";
}