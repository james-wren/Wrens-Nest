#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/evp.h>

#include "../../src/encryption.cpp"

struct TestState {
    int failed = 0;

    void expect(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << std::endl;
            ++failed;
        }
    }
};

std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const uint8_t byte : bytes) {
        out << std::setw(2) << static_cast<int>(byte);
    }
    return out.str();
}

void print_response(const std::string& request_name, const std::string& response) {
    std::cout << request_name << " response: " << response << std::endl;
}

std::vector<uint8_t> decrypt_cbc(const crypto_packet& packet, const std::vector<uint8_t>& key) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        return {};
    }

    std::vector<uint8_t> plaintext(packet.text.size() + 16);
    int len = 0;
    int total = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), packet.iv.data());
    EVP_DecryptUpdate(
        ctx,
        plaintext.data(),
        &len,
        packet.text.data(),
        static_cast<int>(packet.text.size())
    );
    total = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    total += len;
    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(total);
    return plaintext;
}

int main() {
    TestState state;

    const std::vector<uint8_t> payload = {0x00, 0x01, 0x02, 0xfe, 0xff};
    const std::string encoded = encodeBase64(payload);
    const std::vector<uint8_t> decoded = decodeBase64(encoded);
    print_response(
        "base64 round-trip",
        "payload_hex=" + bytes_to_hex(payload) +
            " encoded=" + encoded +
            " decoded_hex=" + bytes_to_hex(decoded)
    );
    state.expect(decoded == payload, "encodeBase64/decodeBase64 should round-trip");

    const std::string generated_key = keyGen("demo-server");
    const std::vector<uint8_t> decoded_key = decodeBase64(generated_key);
    print_response(
        "keyGen",
        "encoded_length=" + std::to_string(generated_key.size()) +
            " decoded_key_bytes=" + std::to_string(decoded_key.size())
    );
    state.expect(decoded_key.size() == 32, "keyGen should produce a 32-byte AES key");

    const std::vector<uint8_t> aes_key = {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    };
    const std::string plaintext = "status: online";
    const crypto_packet packet = encryptAES(plaintext, aes_key);
    const std::vector<uint8_t> decrypted = decrypt_cbc(packet, aes_key);
    print_response(
        "encryptAES",
        "plaintext=\"" + plaintext +
            "\" iv_b64=" + encodeBase64(packet.iv) +
            " ciphertext_b64=" + encodeBase64(packet.text) +
            " decrypted=\"" + std::string(decrypted.begin(), decrypted.end()) + "\""
    );

    state.expect(packet.iv.size() == 16, "encryptAES should generate a 16-byte IV");
    state.expect(!packet.text.empty(), "encryptAES should produce ciphertext");
    state.expect(std::string(decrypted.begin(), decrypted.end()) == plaintext, "encryptAES should round-trip");

    if (state.failed != 0) {
        std::cerr << state.failed << " test(s) failed" << std::endl;
        return 1;
    }

    std::cout << "C++ client tests passed" << std::endl;
    return 0;
}
