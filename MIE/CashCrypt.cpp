//
//  CashCrypt.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 01/10/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#include "CashCrypt.hpp"

using namespace std;

CashCrypt::CashCrypt() {
    string keyFilename = homePath;
    FILE* f = fopen((keyFilename+"Data/Client/Cash/K").c_str(), "rb");
    K = (unsigned char*)malloc(Ksize);
    if (K == NULL) pee("malloc error in TextCrypt::TextCrypt()");
    if (f != NULL)
        fread (K, 1, Ksize, f);
    else {
        spc_rand(K, Ksize);
        f = fopen((keyFilename+"Data/Client/Cash/K").c_str(), "wb");
        fwrite(K, 1, Ksize, f);
    }
    fclose(f);
}

CashCrypt::~CashCrypt() {
    free(K);
}

void CashCrypt::hmac(unsigned char* key, unsigned char* data, int dataSize, unsigned char* md) {
    unsigned int mdSize;
    HMAC(EVP_sha1(), key, Ksize, data, dataSize, md, &mdSize);
    if (mdSize != Ksize)
        pee("CashCrypt::hmac - md size of different from expected\n");
}

vector<unsigned char> CashCrypt::encCounter(unsigned char* key, unsigned char* data, int dataSize) {
    unsigned char md[Ksize];
    hmac(key,data,dataSize,md);
    vector<unsigned char> v;
    v.resize(Ksize);
    for (int i = 0; i < Ksize/*strlen((char*)mac)*/; i++)
        v[i] = md[i];
    return v;
}

void CashCrypt::deriveKey (void* append, int appendSize, void* data, int dataSize, unsigned char* md) {
    unsigned char key[appendSize + dataSize];
    int pos = 0;
    addToArr(append, appendSize, (char*)key, &pos);
    addToArr(data, dataSize, (char*)key, &pos);
    hmac(K, key, pos, md);
}

vector<unsigned char> CashCrypt::encData (unsigned char* key, unsigned char* data, int size) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    unsigned char iv[16] = {0};
    unsigned char* ciphertext = (unsigned char*)malloc(size+16);
    
    if(!(ctx = EVP_CIPHER_CTX_new()))
        pee("CashCrypt::encrypt - could not create ctx\n");
    
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) //key will have 20 bytes but aes128 will only use the first 16
        pee("CashCrypt::encrypt - could not init encrypt\n");
    
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, data, size))
        pee("CashCrypt::encrypt - could not encrypt update\n");
    ciphertext_len = len;
    
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        pee("CashCrypt::encrypt - could not encrypt final\n");
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    vector<unsigned char> v;
    v.resize(ciphertext_len);
    for (int i = 0; i < ciphertext_len; i++)
        v[i] = ciphertext[i];
    free(ciphertext);
    return v;
}

//decryption only being used in the server
/*int CashCrypt::decData (unsigned char* key, unsigned char* ciphertext, int ciphertextSize, unsigned char* plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    unsigned char iv[16] = {0};
    
    if (!(ctx = EVP_CIPHER_CTX_new()))
        pee("ServerUtil::decrypt - could not create ctx\n");
    
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) //key will have 20 bytes but aes128 will only use the first 16
        pee("ServerUtil::decrypt - could not init decrypt\n");
    
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertextSize))
        pee("ServerUtil::decrypt - could not decrypt update\n");
    plaintext_len = len;
    
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        pee("ServerUtil::decrypt - could not decrypt final\n");
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
 
//     vector<unsigned char> v;
//     v.resize(plaintext_len);
//     for (int i = 0; i < plaintext_len; i++)
//     v[i] = plaintext[i];
//     free(plaintext);
//     return v;
 
    return plaintext_len;
}*/
