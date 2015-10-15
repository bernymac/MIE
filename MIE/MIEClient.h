//
//  MIEClient.h
//  MIE
//
//  Created by Bernardo Ferreira on 27/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __MIE__MIEClient__
#define __MIE__MIEClient__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include "EnglishAnalyzer.h"
#include "SBE.h"
#include "TextCrypt.h"
#include <pthread.h>
#include "Util.h"

using namespace std;
using namespace cv;

class MIEClient {
    
    struct sbeThreadData{
        SBE* sbe;
        Mat* descriptors;
        vector<vector<float> >* features;
        int first;
        int last;
    };
    
    double featureTime, indexTime, cryptoTime, cloudTime;
    Ptr<FeatureDetector> detector;
    Ptr<DescriptorExtractor> extractor;
    EnglishAnalyzer* analyzer;
    SBE* sbe;
    TextCrypt* textCrypto;
    
    void processDoc(int id, const char* imgDataset, const char* textDataset, vector< vector<float> >* features, vector< vector<unsigned char> >* encKeywords);
    int sendDoc(char op, int id, vector< vector<float> >* features, vector< vector<unsigned char> >* encKeywords);
    static void* sbeEncryptionThread(void* data);

    
public:
    MIEClient();
    ~MIEClient();
    void addDocs(const char* imgDataset, const char* textDataset, int first, int last, int prefix);
    void index();
    vector<QueryResult> search(int id, const char* imgDataset, const char* textDataset);
    string printTime();
    
};


#endif /* defined(__MIE__MIEClient__) */
