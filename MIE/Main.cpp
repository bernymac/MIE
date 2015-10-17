//
//  main.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 27/02/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "MIEClient.h"
#include "SSEClient.h"
#include "CashClient.hpp"

using namespace cv;
using namespace std;

void printQueryResults (vector<QueryResult> queryResults) {
    for (int i = 0; i < queryResults.size(); i++)
        LOGI("docId: %d score: %f\n", queryResults[i].docId, queryResults[i].score);
}

void printQueryResults (set<QueryResult,cmp_QueryResult> queryResults) {
    for (set<QueryResult,cmp_QueryResult>::iterator it = queryResults.begin(); it!=queryResults.end(); ++it)
        LOGI("docId: %d score: %f\n", (*it).docId, (*it).score);
}

void runMIEClient() {
    LOGI("begin MIE!\n");
    int first = 1;
    int last = 3000;
    MIEClient mie;
    timespec start = getTime();
    
    mie.addDocs("flickr_imgs","flickr_tags",first,last,0);
//    mie.addDocs("flickr_imgs", "flickr_tags",1,10,1000);
//    mie.index();
//    vector<QueryResult> queryResults = mie.search(0, "wang", "docs");
    
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%f.6\n",mie.printTime().c_str(),total_time);
//    printQueryResults(queryResults);
}

void runSSEClient() {
    LOGI("begin SSE!\n");
    SSEClient sse;
    sse.train();

    sse.addDocs("flickr_imgs","flickr_tags",true,1,1000,0);
//    for (int i = 0; i < 100; i++)
//        sse.addDocs("flickr_imgs", "flickr_tags", false,1+i*10,10+i*10,1000);
    set<QueryResult,cmp_QueryResult> queryResults = sse.search(0);
    LOGI("%s\n",sse.printTime().c_str());
    printQueryResults(queryResults);
}

void runCashClient() {
    LOGI("begin Cash SSE!\n");
    int first = 1;
    int last = 3000;
    int groupsize = 10;
    CashClient cash;
    cash.train("flickr_imgs",first,last);
    timespec start = getTime();
    
    for (unsigned i=first; i<=last; i+=groupsize)
        cash.addDocs("flickr_imgs","flickr_tags",i,i+groupsize-1,0);
//    for (int i = 0; i < 100; i++)
//        cash.addDocs("flickr_imgs", "flickr_tags", first+i*10, 10+i*10, last);
//    vector<QueryResult> queryResults = cash.search("flickr_imgs","flickr_tags",1);
    
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%f.6\n",cash.printTime().c_str(),total_time);
//    printQueryResults(queryResults);
}
 
int main(int argc, const char * argv[]) {
//    runMIEClient();
//    runSSEClient();
    runCashClient();
}

