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
    double cryptoTime, cloudTime, indexTime, trainTime;
    Ptr<FeatureDetector> detector;
    Ptr<DescriptorExtractor> extractor;
    Ptr<BOWImgDescriptorExtractor> bowExtractor;
    EnglishAnalyzer* analyzer;
    CashCrypt* crypto;
    vector<int>* imgDcount;
    map<string,int>* textDcount;
    
    void encryptAndIndex(void* keyword, int keywordSize, int counter, int docId, int tf,
                         map<vector<unsigned char>, vector<unsigned char> >* index);
//    void processDoc(int id, vector< vector<float> >* features, vector< vector<unsigned char> >* encKeywords);
//    map<int,int> decryptPostingList(unsigned char* encPostingList, int size);
//    void retrieveIndex(vector< map<int,int> >* imgIndex, map<vector<unsigned char>,map<int,int> >* textIndex);
//    void receivePostingLists(int sockfd, vector<map<int,int> >* imgPostingLists, map<vector<unsigned char>,map<int,int> >* textPostingLists);
//    set<QueryResult,cmp_QueryResult> calculateQueryResults(int sockfd, map<int,int>* vws, map<vector<unsigned char>,int>* encKeywords);
//    set<QueryResult,cmp_QueryResult> mergeSearchResults(set<QueryResult,cmp_QueryResult>* imgResults,                                                                   set<QueryResult,cmp_QueryResult>* textResults);
    
public:
    CashClient();
    ~CashClient();
    void train();
    //    void addDocs();
    void addDocs(const char* imgDataset, const char* textDataset, bool firstUpdate, int first, int last, int prefix);
    vector<QueryResult> search(int id);
    string printTime();
};


#endif /* CashClient_hpp */
