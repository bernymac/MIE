//
//  PaillierCashClient.hpp
//  MIE
//
//  Created by Bernardo Ferreira on 02/11/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#ifndef PaillierCashClient_hpp
#define PaillierCashClient_hpp

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <set>
#include "EnglishAnalyzer.h"
#include "CashCrypt.hpp"
#include "Util.h"
#include "gmp.h"
extern "C" {
#include "paillier.h"
}


using namespace std;
using namespace cv;

class PaillierCashClient {
    double featureTime, cryptoTime, cloudTime, indexTime, trainTime;
    Ptr<FeatureDetector> detector;
    Ptr<DescriptorExtractor> extractor;
    Ptr<BOWImgDescriptorExtractor> bowExtractor;
    EnglishAnalyzer* analyzer;
    CashCrypt* crypto;
    
    vector<int>* imgDcount;
    map<string,int>* textDcount;
    
    paillier_pubkey_t* homPub;
    paillier_prvkey_t* homPriv;
    
    void encryptAndIndex(void* keyword, int keywordSize, int counter, int docId, int tf,
                         map<vector<unsigned char>, vector<unsigned char> >* index);
    
public:
    PaillierCashClient();
    ~PaillierCashClient();
    void train(const char* dataset, int first, int last);
    void addDocs(const char* imgDataset, const char* textDataset, int first, int last, int prefix);
    vector<QueryResult> search(const char* imgDataset, const char* textDataset, int id);
    string printTime();
};





#endif /* PaillierCashClient_hpp */
