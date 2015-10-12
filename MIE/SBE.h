//
//  SBE.h
//  MIE
//
//  Created by Bernardo Ferreira on 05/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//
#ifndef __MIE__SBE__
#define __MIE__SBE__

#include <vector>
#include "Util.h"

using namespace std;

class SBE {
   
    int m;
    float delta;
    
    int k;
    vector< vector<float> > a;
    vector<float> w;
    
public:
    SBE (int dimensions);
    vector<float> encode (vector<float> x);
    
};

#endif
