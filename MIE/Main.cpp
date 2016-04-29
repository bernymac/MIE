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
#include "PaillierCashClient.hpp"

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

void runMIEClientHolidayAdd() {
    LOGI("begin MIE Holiday Add!\n");
    MIEClient mie;
    timespec start = getTime();
    mie.addDocs("inriaHolidays","flickr_tags",1,1491,0);
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%.6f\n",mie.printTime().c_str(),total_time);
}

void runMIEClientFlickrAdd() {
    LOGI("begin MIE Flickr Add!\n");
    MIEClient mie;
    timespec start = getTime();
    mie.addDocs("flickr_imgs", "flickr_tags",1,1000,0);
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%.6f\n",mie.printTime().c_str(),total_time);
}

void runMIEClientIndex() {
    LOGI("begin MIE index!\n");
    MIEClient mie;
    timespec start = getTime();
    mie.index();
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%.6f\n",mie.printTime().c_str(),total_time);
}

void runMIEClientHolidaySingleSearch() {
    LOGI("begin MIE index!\n");
    MIEClient mie;
    string imgPath = homePath;
    imgPath += "Datasets/inriaHolidays/100701.jpg";
    string textPath = homePath;
    textPath += "Datasets/flickr_tags/tags1.txt";
    vector<QueryResult> queryResults = mie.search(1, imgPath, textPath);
    printQueryResults(queryResults);
}


void runMIEClientHolidayQueries() {
    LOGI("begin MIE Holiday Queries!\n");
    MIEClient mie;
    timespec start = getTime();
    map<int,vector<QueryResult> > queries;
    char* imgPath = new char[120];
    char* textPath = new char[120];
    for (int i = 100000; i <= 149900; i+=100) {
        bzero(imgPath, 120);
        bzero(textPath, 120);
        sprintf(imgPath, "%sDatasets/inriaHolidays/%d.jpg", homePath, i);
        sprintf(textPath, "%sDatasets/flickr_tags/tags%d.txt", homePath, i/100000);
        queries[i] = mie.search(i, imgPath, textPath);
    }
    delete[] imgPath;
    delete[] textPath;
    string fName = homePath;
    fName += "Data/Client/MIE/mieHoliday.dat";
    printHolidayResults(fName, queries);

}

//deprecated
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

void runCashClientHolidayAdd() {
    LOGI("begin Cash Holiday Add!\n");
    timespec start = getTime();
    CashClient cash;
    cash.train("inriaHolidays",1,1491);
    cash.addDocs("inriaHolidays","flickr_tags",1,1491,0);
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%.6f\n",cash.printTime().c_str(),total_time);
}

void runCashClientFlickrAdd() {
    LOGI("begin Cash Flickr Add!\n");
    int first = 1;
    int last = 1000;
    int groupsize = 1000;
    CashClient cash;
    cash.train("flickr_imgs",first,last);
    timespec start = getTime();
    for (unsigned i=first; i<=last; i+=groupsize)
        cash.addDocs("flickr_imgs","flickr_tags",i,i+groupsize-1,0);
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%.6f\n",cash.printTime().c_str(),total_time);
}

void runCashClientHolidaySingleSearch() {
    LOGI("begin Cash Holiday Queries!\n");
    CashClient cash;
    cash.train("inriaHolidays",1,1491);
    string imgPath = homePath;
    imgPath += "Datasets/inriaHolidays/100701.jpg";
    string textPath = homePath;
    textPath += "Datasets/flickr_tags/tags1.txt";
    timespec start = getTime();
    vector<QueryResult> queryResults = cash.search(imgPath,textPath,1, true);
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%.6f\n",cash.printTime().c_str(),total_time);
    printQueryResults(queryResults);
}

void runCashClientHolidayQueries() {
    LOGI("begin Cash Holiday Queries!\n");
    CashClient cash;
    cash.train("inriaHolidays",1,1491);
    map<int,vector<QueryResult> > queries;
    char* imgPath = new char[120];
    char* textPath = new char[120];
    for (int i = 100000; i <= 149900; i+=100) {
        bzero(imgPath, 120);
        bzero(textPath, 120);
        sprintf(imgPath, "%sDatasets/inriaHolidays/%d.jpg", homePath, i);
        sprintf(textPath, "%sDatasets/flickr_tags/tags%d.txt", homePath, i/100000);
        queries[i] = cash.search(imgPath,textPath,1, true);
    }
    delete[] imgPath;
    delete[] textPath;
    string fName = homePath;
    fName += "Data/Client/Cash/cashHoliday.dat";
    printHolidayResults(fName, queries);
}

void runPaillierCashClient() {
    LOGI("begin Paillier Cash SSE!\n");
    int first = 1;
    int last = 1000;
    int groupsize = 10;
    PaillierCashClient cash;
    cash.train("flickr_imgs",first,last);

    timespec start = getTime();    
//    for (unsigned i=first; i<=last; i+=groupsize)
//        cash.addDocs("flickr_imgs","flickr_tags",i,i+groupsize-1,0);
    //    for (int i = 0; i < 100; i++)
    //        cash.addDocs("flickr_imgs", "flickr_tags", first+i*10, 10+i*10, last);
//    cash.cleanTime();
    vector<QueryResult> queryResults = cash.search("flickr_imgs","flickr_tags",1, false);
    
    double total_time = diffSec(start, getTime());
    LOGI("%s total_time:%.6f\n",cash.printTime().c_str(),total_time);
    printQueryResults(queryResults);
}
 
int main(int argc, const char * argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2) {
        printf("Incorrect number of arguments. Please give a test name, e.g. \"mieHolidayAdd\", \"mieFlickrAdd\", \"mieIndex\", \"mieHolidayQueries\", \"mieHolidaySingleSearch\", \"sse\", \"Cash\" or \"PaillierCash\"\n");
        return 0;
    } else if (strcasecmp(argv[1], "mieHolidayAdd") == 0)
        runMIEClientHolidayAdd();
    else if (strcasecmp(argv[1], "mieFlickrAdd") == 0)
        runMIEClientFlickrAdd();
    else if (strcasecmp(argv[1], "mieIndex") == 0)
        runMIEClientIndex();
    else if (strcasecmp(argv[1], "mieHolidayQueries") == 0)
        runMIEClientHolidayQueries();
    else if (strcasecmp(argv[1], "mieHolidaySingleSearch") == 0)
        runMIEClientHolidaySingleSearch();
    else if (strcasecmp(argv[1], "sse") == 0)
        runSSEClient();
    else if (strcasecmp(argv[1], "cashHolidayAdd") == 0)
        runCashClientHolidayAdd();
    else if (strcasecmp(argv[1], "cashFlickrAdd") == 0)
        runCashClientFlickrAdd();
    else if (strcasecmp(argv[1], "cashHolidayQueries") == 0)
        runCashClientHolidayQueries();
    else if (strcasecmp(argv[1], "cashHolidaySingleSearch") == 0)
        runCashClientHolidaySingleSearch();
    else if (strcasecmp(argv[1], "PaillierCash") == 0)
        runPaillierCashClient();
    else {
        printf("Server command not recognized! Available Servers: \"mie\", \"sse\", \"Cash\" and \"PaillierCash\"\n");
        return 0;
    }
}
