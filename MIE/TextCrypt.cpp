//
//  TextCrypt.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 13/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "TextCrypt.h"

using namespace std;

TextCrypt::TextCrypt() {
    string keyFilename = homePath;
    FILE* f = fopen((keyFilename+"Data/MIE/textKey").c_str(), "rb");
    key = (unsigned char*)malloc(keysize);
    if (key == NULL) pee("malloc error in TextCrypt::TextCrypt()");
    if (f != NULL)
        fread (key, 1, keysize, f);
    else {
        spc_rand(key, keysize);
        f = fopen((keyFilename+"Data/MIE/textKey").c_str(), "wb");
        fwrite(key, 1, keysize, f);
    }
    fclose(f);
//    key = (unsigned char*)malloc(keysize);
//    spc_rand(key, keysize);
}

TextCrypt::~TextCrypt() {
    free(key);
}

vector<unsigned char> TextCrypt::encode(string keyword) {
    unsigned char* mac = HMAC(EVP_sha1(), key, keysize, (unsigned char*)keyword.c_str(), keyword.size(), NULL, NULL);
    vector<unsigned char> v;
    v.resize(keysize);
    for (int i = 0; i < strlen((char*)mac); i++)
        v[i] = mac[i];
    return v;
}

