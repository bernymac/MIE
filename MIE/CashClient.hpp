//
//  CashClient.hpp
//  MIE
//
//  Created by Bernardo Ferreira on 25/09/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//
//  Implementation of the scheme by Cash et al. NDSS 2014
//  without random oracles and extended to multimodal ranked searching
//

#ifndef CashClient_hpp
#define CashClient_hpp

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <set>
#include "EnglishAnalyzer.h"
#include "CashCrypt.hpp"
#include "Util.h"

using namespace std;
using namespace cv;

class CashClient {
    double featureTime, cryptoTime, cloudTime, indexTime, trainTime;
    Ptr<FeatureDetector> detector;
    Ptr<DescriptorExtractor> extractor;
    Ptr<BOWImgDescriptorExtractor> bowExtractor;
    EnglishAnalyzer* analyzer;
    CashCrypt* crypto;
    
    vector<int>* imgDcount;
    map<string,int>* textDcount;
    
    void encryptAndIndex(void* keyword, int keywordSize, int counter, int docId, int tf,
                         map<vector<unsigned char>, vector<unsigned char> >* index);
    vector<QueryResult> queryRO(map<int,int>* vws, map<string,int>* kws);
    vector<QueryResult> queryStd(map<int,int>* vws, map<string,int>* kws);
    
public:
    CashClient();
    ~CashClient();
    void train(const char* dataset, int first, int last);
    void addDocs(const char* imgDataset, const char* textDataset, int first, int last, int prefix);
    vector<QueryResult> search(const char* imgDataset, const char* textDataset, int id);
    string printTime();
};


#endif /* CashClient_hpp */
