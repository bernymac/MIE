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
    MIEClient mie;
    
    mie.addDocs("wang","docs",0,999,0);
//    mie.addDocs("wang", "docs",0,9,1000);
    mie.index();
    vector<QueryResult> queryResults = mie.search(0, "wang", "docs");
    LOGI("%s\n",mie.printTime().c_str());
    printQueryResults(queryResults);
}

void runSSEClient() {
    LOGI("begin SSE!\n");
    SSEClient sse;
    sse.train();
    
    sse.addDocs("wang","docs",true,0,999,0);
//    for (int i = 0; i < 100; i++)
//        sse.addDocs("wang", "docs", false,i*10,9+i*10,1000);
    set<QueryResult,cmp_QueryResult> queryResults = sse.search(0);
    LOGI("%s\n",sse.printTime().c_str());
    printQueryResults(queryResults);
}

void runCashClient() {
    LOGI("begin Cash SSE!\n");
    CashClient cash;
    cash.train();
    
    cash.addDocs("wang","docs",true,0,999,0);
//    for (int i = 0; i < 100; i++)
//        cash.addDocs("wang", "docs", false,i*10,9+i*10,1000);
    vector<QueryResult> queryResults = cash.search(0);
    LOGI("%s\n",cash.printTime().c_str());
    printQueryResults(queryResults);
}
 
int main(int argc, const char * argv[]) {
//    runMIEClient();
    runSSEClient();
//    runCashClient();
}

