//
//  Server.h
//  MIE
//
//  Created by Bernardo Ferreira on 02/05/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __MIE__MIEServer__
#define __MIE__MIEServer__

#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "ServerUtil.h"
#include "Server.h"

using namespace std;
using namespace cv;

class MIEServer : public Server {
    void startServer();
    void receiveDoc(int newsockfd, int& id, Mat& mat,
                    vector<vector<unsigned char> >& keywords);
    void addDoc(int newsockfd, map<int,Mat>& imgFeatures,
                map<int,vector<vector<unsigned char> > >& textFeatures);
    bool readOrPersistFeatures(map<int,cv::Mat>& imgFeatures,
                               map<int,vector<vector<unsigned char>>>& textFeatures);
    void indexImgs(map<int,Mat>& imgFeatures, vector<map<int,int> >& imgIndex,
                   BOWImgDescriptorExtractor& bowExtr, int& nImgs);
    void indexText(map<int,vector<vector<unsigned char> > >& textFeatures,
                   map<vector<unsigned char>,map<int,int> >& textIndex, int& nTextDocs);
    void persistImgIndex(vector<map<int,int> >& imgIndex, int nImgs);
    void persistTextIndex(map<vector<unsigned char>,map<int,int> >& textIndex, int nTextDocs);
    void persistIndex(vector<map<int,int> >& imgIndex,
                      map<vector<unsigned char>,map<int,int> >& textIndex, int nImgs, int nTextDocs);
    bool readIndex(vector<map<int,int> >& imgIndex,
                   map<vector<unsigned char>,map<int,int> >& textIndex, int& nImgs, int& nTextDocs);
    set<QueryResult,cmp_QueryResult> imgSearch (Mat& features, BOWImgDescriptorExtractor& bowExtr,
                                                           vector<map<int,int> >& imgIndex, int& nImgs);
    set<QueryResult,cmp_QueryResult> textSearch(vector<vector<unsigned char> >& keywords,
                                                map<vector<unsigned char>,map<int,int> >& textIndex, int& nTextDocs);
//    set<QueryResult,cmp_QueryResult> mergeSearchResults(set<QueryResult,cmp_QueryResult>* imgResults,
//                                                                   set<QueryResult,cmp_QueryResult>* textResults);
//    void sendQueryResponse(int newsockfd, set<QueryResult,cmp_QueryResult>* mergedResults);
    void search(int newsockfd, BOWImgDescriptorExtractor& bowExtr, vector<map<int,int> >& imgIndex,
                int& nImgs, map<vector<unsigned char>,map<int,int> >& textIndex, int& nTextDocs);
public:
    MIEServer();
};

#endif