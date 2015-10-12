//
//  SSECrypt.h
//  MIE
//
//  Created by Bernardo Ferreira on 30/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//
#ifndef __MIE__SSECrypt__
#define __MIE__SSECrypt__


#include <stdint.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <vector>
#include <openssl/evp.h>
#include "Util.h"

using namespace std;

class SSECrypt {
    
    unsigned char* aesKey;
    
public:
    
    static const unsigned int aesKeySize = 32;
    
    SSECrypt();
    ~SSECrypt();
    vector<unsigned char> encrypt (unsigned char* data, int size);
//    vector<unsigned char> decrypt (unsigned char* data, int size);
    int decrypt (unsigned char* ciphertext, int ciphertextSize, unsigned char* plaintext);
    
};


#endif /* defined(__MIE__SSECrypt__) */
