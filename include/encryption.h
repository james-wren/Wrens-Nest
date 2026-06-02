#include <iostream>

struct crypto_packet {
    std::vector<uint8_t> iv;
    std::vector<uint8_t> ciphertext;
};

std::string keyGen(std::string serverName);
std::vector<uint8_t> decodeBase64(std::string stringb64);
std::string encodeBase64(const std::vector<uint8_t>& binary_data);
crypto_packet encryptAES(const std::string& plaintext, const std::vector<uint8_t>& key);