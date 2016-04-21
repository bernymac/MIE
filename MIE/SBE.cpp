//
//  SBE.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 04/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

//#include <random>
#include "SBE.h"

using namespace std;

SBE::SBE (int dimensions) {
    m = 128;//256;//64
    delta = 0.5f;
    k = dimensions;
    
    string keyFilename = homePath;
    FILE* f = fopen((keyFilename+"Data/Client/MIE/sbeKey").c_str(), "rb");
    size_t buffSize = m * k * sizeof(float) + m * sizeof(float);
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in SBE::SBE (int dimensions)");
    if (f != NULL)
        fread (buff, 1, buffSize, f);
    int pos = 0;
    a.resize(m);
    for (int i = 0; i < m; i++) {
        a[i].resize(k);
        for (int j = 0; j < k; j++) {
            if (f != NULL)
                readFromArr(&a[i][j], sizeof(float), buff, &pos);
            else {
                float rand = spc_rand_real();
                a[i][j] = rand;
                addToArr(&rand, sizeof(float), buff, &pos);
            }
        }
    }
    w.resize(m);
    for (int i = 0; i < m; i++) {
        if (f != NULL)
            readFromArr(&w[i], sizeof(float), buff, &pos);
        else {
            float rand = spc_rand_real_range(0.f, delta);
            w[i] = rand;
            addToArr(&rand, sizeof(float), buff, &pos);
        }
    }
    if (f == NULL) {
        f = fopen((keyFilename+"Data/Client/MIE/sbeKey").c_str(), "wb");
        fwrite(buff, 1, buffSize, f);
    }
    free(buff);
    fclose(f);
//} else {
//        random_device rd;
//        mt19937 engine (rd());
//        uniform_real_distribution<float> distributionAll (0.f,1.f);
//        a.resize(m);
//        for (int i = 0; i < m; i++) {
//            a[i].resize(k);
//            for (int j = 0; j < k; j++)
//                a[i][j] = distributionAll (engine);
//                a[i][j] = spc_rand_real();
//        }
//        w.resize(m);
//        uniform_real_distribution<float> distributionDelta (0.f,delta);
//        for (int i = 0; i < m; i++)
//            w[i] = distributionDelta (engine);
//            w[i] = spc_rand_real_range(0.f, delta);
//           }
}


vector<float> SBE::encode (vector<float> x) {
        vector<float> encoded (m,0);
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < k; j++)
                encoded[i] += a[i][j] * x[j];			//multiply input with random matrix a
            encoded[i] += w[i];						//add dither
            encoded[i] /= delta;						// divide by delta
            if (int(encoded[i]) % 2)
                encoded[i] = 0;
            else
                encoded[i] = 1;
        }
        return encoded;
}
    
