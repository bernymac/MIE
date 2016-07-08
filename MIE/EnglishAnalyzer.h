//
//  EnglishAnalyzer.h
//  MIE
//
//  Created by Bernardo Ferreira on 13/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __MIE__EnglishAnalyzer__
#define __MIE__EnglishAnalyzer__

#include <vector>
#include <set>
#include <string>
#include <stdlib.h>
#include <ctype.h>
#include "Util.h"
#include "PorterStemmer.c"

using namespace std;

class EnglishAnalyzer {

    #define INC 50
    char * s;
    int i_max;
    set<string> stopWords;
    
    void increase_s();
    
public:
    
    EnglishAnalyzer();
    ~EnglishAnalyzer();
    vector<string> extractFile(string fname);
    bool isStopWord(string word);
    
};
#endif /* defined(__MIE__EnglishAnalyzer__) */
