//
//  TextCrypt.h
//  MIE
//
//  Created by Bernardo Ferreira on 13/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __MIE__TextCrypt__
#define __MIE__TextCrypt__

#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <openssl/hmac.h>
#include "Util.h"

using namespace std;

class TextCrypt {
    
    unsigned char* key;
    
public:
    
    static const unsigned int keysize = 20;
    
    TextCrypt();
    ~TextCrypt();
    vector<unsigned char> encode (string keyword);
    
};

#endif /* defined(__MIE__TextCrypt__) */
