#include <iostream>
#include <vector>
#include <fstream>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/err.h>

#include "main.h"
#include "json.hpp"

struct crypto_packet{
    std::vector<uint8_t> iv;
    std::vector<uint8_t> text;
};

crypto_packet encryptAES(const std::string& plaintext, const std::vector<uint8_t>& key){
    crypto_packet packet;
    packet.iv.resize(16);

    RAND_bytes(packet.iv.data(), static_cast<int>(packet.iv.size()));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), packet.iv.data());

    packet.text.resize(plaintext.size() + 16);

    int len = 0;
    int ciphertext_len = 0;

    EVP_EncryptUpdate(ctx, packet.text.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), static_cast<int>(plaintext.size()));
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, packet.text.data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    packet.text.resize(ciphertext_len);

    return packet;
}

std::vector<uint8_t> decodeBase64(std::string stringb64){
    BIO* bio = BIO_new_mem_buf(stringb64.data(), static_cast<int>(stringb64.length()));

    BIO* b64 = BIO_new(BIO_f_base64());

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO* bio_chain = BIO_push(b64, bio);

    std::vector<uint8_t>  decoded_bytes(stringb64.length());

    int decoded_length = BIO_read(bio_chain, decoded_bytes.data(), static_cast<int>(decoded_bytes.size()));

    BIO_free_all(bio_chain);

    decoded_bytes.resize(decoded_length);

    return decoded_bytes;
} 

std::string encodeBase64(const std::vector<uint8_t>& binary_data){
    BIO * bio_mem = BIO_new(BIO_s_mem());

    BIO * bio_b64 = BIO_new(BIO_f_base64());

    BIO_set_flags(bio_b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bio_chain = BIO_push(bio_b64, bio_mem);

    BIO_write(bio_chain, binary_data.data(), binary_data.size());

    BIO_flush(bio_chain);

    BUF_MEM* buffer_pointer;
    BIO_get_mem_ptr(bio_chain, &buffer_pointer);

    std::string base64_text(buffer_pointer->data, buffer_pointer->length);

    BIO_free_all(bio_chain);

    return base64_text;
}

std::string keyGen(std::string serverName) {
    const size_t AES_256_KEY_SIZE = 32;
    std::vector<uint8_t> key(AES_256_KEY_SIZE);

    std::cout << "Generating secure AES-256 key" << std::endl;

    if(RAND_bytes(key.data(), key.size()) != 1) {
        unsigned long err = ERR_get_error();

        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));

        std::cout << "Critical error encountered in key generation: " << err_buf << std::endl;

        return "Failed";
    }

    std::cout << "Key generated succesfully" << std::endl;
    
    std::string text_key = encodeBase64(key);

    return text_key;
}