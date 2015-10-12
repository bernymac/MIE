//
//  SSEClient.h
//  MIE
//
//  Created by Bernardo Ferreira on 28/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __MIE__SSEClient__
#define __MIE__SSEClient__

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <set>
#include "EnglishAnalyzer.h"
#include "TextCrypt.h"
#include "Util.h"
#include "SSECrypt.h"

using namespace std;
using namespace cv;

class SSEClient {
    double cryptoTime, cloudTime, indexTime, trainTime;
    Ptr<FeatureDetector> detector;
    Ptr<DescriptorExtractor> extractor;
    Ptr<BOWImgDescriptorExtractor> bowExtractor;
    EnglishAnalyzer* analyzer;
    TextCrypt* textCrypto;
    SSECrypt* aesCrypto;
    
    void processDoc(int id, vector< vector<float> >* features, vector< vector<unsigned char> >* encKeywords);
    map<int,int> decryptPostingList(unsigned char* encPostingList, int size);
    void retrieveIndex(vector< map<int,int> >* imgIndex, map<vector<unsigned char>,map<int,int> >* textIndex);
    void receivePostingLists(int sockfd, vector<map<int,int> >* imgPostingLists, map<vector<unsigned char>,map<int,int> >* textPostingLists);
    set<QueryResult,cmp_QueryResult> calculateQueryResults(int sockfd, map<int,int>* vws, map<vector<unsigned char>,int>* encKeywords);
    set<QueryResult,cmp_QueryResult> mergeSearchResults(set<QueryResult,cmp_QueryResult>* imgResults,                                                                   set<QueryResult,cmp_QueryResult>* textResults);
    
public:
    SSEClient();
    ~SSEClient();
    void train();
//    void addDocs();
    void addDocs(const char* imgDataset, const char* textDataset, bool firstUpdate, int first, int last, int prefix);
    set<QueryResult,cmp_QueryResult> search(int id);
    string printTime();
};


#endif /* defined(__MIE__SSEClient__) */
