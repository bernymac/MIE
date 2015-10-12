//
//  SSECrypt.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 30/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "SSECrypt.h"

SSECrypt::SSECrypt() {
//    ERR_load_crypto_strings();
//    OpenSSL_add_all_algorithms();
//    OPENSSL_config(NULL);
    string keyFilename = dataPath;
    FILE* f = fopen((keyFilename+"/SSE/aesKey").c_str(), "rb");
    aesKey = (unsigned char*)malloc(aesKeySize);
    if (aesKey == NULL) pee("malloc error in SSECrypt::SSECrypt()");
    if (f != NULL)
        fread (aesKey, 1, aesKeySize, f);
    else {
        spc_rand(aesKey, aesKeySize);
        f = fopen((keyFilename+"/SSE/aesKey").c_str(), "wb");
        fwrite(aesKey, 1, aesKeySize, f);
    }
    fclose(f);
}

SSECrypt::~SSECrypt() {
    free(aesKey);
}

vector<unsigned char> SSECrypt::encrypt (unsigned char* data, int size) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    unsigned char iv[16] = {0};
    unsigned char* ciphertext = (unsigned char*)malloc(size+16);
    
    if(!(ctx = EVP_CIPHER_CTX_new()))
        pee("SSECrypt::encrypt - could not create ctx\n");
    
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aesKey, iv))
        pee("SSECrypt::encrypt - could not init encrypt\n");
    
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, data, size))
        pee("SSECrypt::encrypt - could not encrypt update\n");
    ciphertext_len = len;
    
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        pee("SSECrypt::encrypt - could not encrypt final\n");
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);
/* 
    unsigned char iv[16] = {0};
    unsigned char* out = (unsigned char*)malloc(size+16); //at least one block longer than in[]
    int written = 0, temp = 0;

    EVP_EncryptInit(ctx, EVP_aes_256_cbc(), aesKey, iv);
    if (!EVP_EncryptUpdate(ctx, out, &temp, &data[written], size))
        pee("error encrypting");
    written = temp;
    if (!EVP_EncryptFinal(ctx, out+temp, &temp))
        pee("error padding after encryption");
    written += temp;
//    LOGI("ciphertext length: %d\n", written);
*/
    vector<unsigned char> v;
    v.resize(ciphertext_len);
    for (int i = 0; i < ciphertext_len; i++)
        v[i] = ciphertext[i];
    free(ciphertext);
    return v;
}

int SSECrypt::decrypt (unsigned char* ciphertext, int ciphertextSize, unsigned char* plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    unsigned char iv[16] = {0};
    
    if (!(ctx = EVP_CIPHER_CTX_new()))
        pee("SSECrypt::decrypt - could not create ctx\n");

    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aesKey, iv))
        pee("SSECrypt::decrypt - could not init decrypt\n");
    
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertextSize))
        pee("SSECrypt::decrypt - could not decrypt update\n");
    plaintext_len = len;

    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        pee("SSECrypt::decrypt - could not decrypt final\n");
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
/*
    vector<unsigned char> v;
    v.resize(plaintext_len);
    for (int i = 0; i < plaintext_len; i++)
        v[i] = plaintext[i];
    free(plaintext);
    return v;
 */
    return plaintext_len;
}
