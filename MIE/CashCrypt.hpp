//
//  CashCrypt.hpp
//  MIE
//
//  Created by Bernardo Ferreira on 01/10/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#ifndef CashCrypt_hpp
#define CashCrypt_hpp

#include <stdint.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <vector>
#include <openssl/hmac.h>
#include "Util.h"


using namespace std;

class CashCrypt {
    
    unsigned char* K;
    
    void hmac (unsigned char* key, unsigned char* data, int dataSize, unsigned char* md);
    
public:
    
    static const unsigned int Ksize = 20; //hmac sha1 key and output size
    
    CashCrypt();
    ~CashCrypt();
    vector<unsigned char> encCounter (unsigned char* key, unsigned char* data, int dataSize);
    void deriveKey (void* append, int appendSize, void* data, int dataSize, unsigned char* md);
    vector<unsigned char> encData (unsigned char* key, unsigned char* data, int size);
//    int decData (unsigned char* key, unsigned char* ciphertext, int ciphertextSize, unsigned char* plaintext);

    
};


#endif /* CashCrypt_hpp */
