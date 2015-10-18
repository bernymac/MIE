//
//  Server.h
//  MIE
//
//  Created by Bernardo Ferreira on 02/05/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __MIE__MIEServerMT__
#define __MIE__MIEServerMT__

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
#include "ThreadPool.h"
#include <mutex>

using namespace std;
using namespace cv;

class MIEServerMT {
    
    static map<int,Mat> imgFeatures;
    static map<int,vector<vector<unsigned char> > > textFeatures;
    static vector<map<int,int> > imgIndex;
    static map<vector<unsigned char>,map<int,int> > textIndex;
    static atomic<int> nImgs, nDocs;
    static BOWImgDescriptorExtractor* bowExtr;
    static mutex imgFeaturesLock,textFeaturesLock,imgIndexLock,textIndexLock;
    
    void startServer();
    static void clientThread(int newsockfd);
    static void receiveDoc(int newsockfd, int* id, Mat* mat, vector<vector<unsigned char> >* keywords);
    static void addDoc(int newsockfd);
    static bool readOrPersistFeatures(string dataPath);
    static void indexImgs();
    static void indexText();
    static void persistImgIndex(string dataPath);
    static void persistTextIndex(string dataPath);
    static void persistIndex(string dataPath);
    static bool readIndex(string dataPath);
    static set<QueryResult,cmp_QueryResult> imgSearch (Mat* features);
    static set<QueryResult,cmp_QueryResult> textSearch(vector<vector<unsigned char> >* keywords);
    static void search(int newsockfd);
    
public:
    MIEServerMT();
    ~MIEServerMT();
    
};

#endif