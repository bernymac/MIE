 //
//  CashServer.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 06/10/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#include "CashServer.hpp"

using namespace std;

CashServer::CashServer() {
//    startServer();
}

CashServer::~CashServer() {
    free(encImgIndex);
    free(encTextIndex);
}

void CashServer::startServer() {
    encImgIndex = new map<vector<unsigned char>,vector<unsigned char> >;
    encTextIndex = new map<vector<unsigned char>,vector<unsigned char> >;
    nDocs = 0;
    int sockfd = initServer();
    printf("Cash server started!\n");
    while (true) {
        resetTime();
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            printf("ERROR on accept\n");
            continue;
        }
        char buffer[1];
        if (read(newsockfd,buffer,1) < 0)  {
            printf("ERROR reading from socket\n");
            close(newsockfd);
            continue;
        }
        //        printf("command received: %c\n",buffer[0]);
        switch (buffer[0]) {
            case 'a':
                receiveDocs(newsockfd);
                break;
            case 's':
                this->search(newsockfd);
                break;
            default:
                printf("unkonwn command!\n");
        }
//        ack(newsockfd);
        close(newsockfd);
        printf("%s\n",printTime().c_str());
    }
}


void CashServer::receiveDocs(int newsockfd) {
    int buffSize = 5*sizeof(int);
    char buffer[buffSize];
    bzero(buffer,buffSize);
    if (read(newsockfd,buffer,buffSize) < 0) pee("CashServer::receiveDocs ERROR reading from socket");
    int pos = 0;
    nDocs += readIntFromArr(buffer, &pos);
    const int imgIndexSize = readIntFromArr(buffer, &pos);
    const int textIndexSize = readIntFromArr(buffer, &pos);
    const int Ksize = readIntFromArr(buffer, &pos);
    int postingSize = readIntFromArr(buffer, &pos);
    for (int i = 0; i < imgIndexSize; i++) {
        vector<unsigned char> vw;
        vw.resize(Ksize);
        vector<unsigned char> posting;
        posting.resize(postingSize);
        buffSize = Ksize*sizeof(unsigned char) + postingSize*sizeof(unsigned char) + sizeof(int);
        char* buff = (char*)malloc(buffSize);
        pos = 0;
        receiveAll(newsockfd, buff, buffSize);
        for (int j = 0; j < Ksize; j++)
            readFromArr(&vw[j], sizeof(unsigned char), buff, &pos);
        for (int j = 0; j < postingSize; j++)
            readFromArr(&posting[j], sizeof(unsigned char), buff, &pos);
        postingSize = readIntFromArr(buff, &pos);
        if (encImgIndex->find(vw)!=encImgIndex->end())                  //debug - not supposed to happen
            pee("Found an unexpected collision in encImgIndex");
        (*encImgIndex)[vw] = posting;
        free(buff);
    }
    for (int i = 0; i < textIndexSize; i++) {
        vector<unsigned char> kw;
        kw.resize(Ksize);
        vector<unsigned char> posting;
        posting.resize(postingSize);
        buffSize = Ksize*sizeof(unsigned char) + postingSize*sizeof(unsigned char);
        if (i < textIndexSize-1)
            buffSize += sizeof(int);
        char* buff = (char*)malloc(buffSize);
        pos = 0;
        receiveAll(newsockfd, buff, buffSize);
        for (int j = 0; j < Ksize; j++)
            readFromArr(&kw[j], sizeof(unsigned char), buff, &pos);
        for (int j = 0; j < postingSize; j++)
            readFromArr(&posting[j], sizeof(unsigned char), buff, &pos);
        if (i < textIndexSize-1)
            postingSize = readIntFromArr(buff, &pos);
        if (encTextIndex->find(kw)!=encTextIndex->end())                  //debug
            pee("Found an unexpected collision in encTextIndex");
        (*encTextIndex)[kw] = posting;
        free(buff);
    }
    //    printf("finished receiving docs\n");
}

void CashServer::search(int newsockfd) {
    timespec start = getTime(); 
    //initialize and read variables
    long buffSize = 3*sizeof(int);
    char buffer[buffSize];
    receiveAll(newsockfd, buffer, buffSize);
    int pos = 0;
    const int vwsSize = readIntFromArr(buffer, &pos);
    const int kwsSize = readIntFromArr(buffer, &pos);
    const int Ksize = readIntFromArr(buffer, &pos);
    
    //if RO
    buffSize = (vwsSize + kwsSize) * (2*Ksize + sizeof(int));
    char* buff = new char[buffSize];
    receiveAll(newsockfd, buff, buffSize);
    pos = 0;
    cloudTime += diffSec(start, getTime());
    
    set<QueryResult,cmp_QueryResult> imgQueryResults = calculateQueryResultsRO( vwsSize, Ksize, buff, &pos, encImgIndex);
    set<QueryResult,cmp_QueryResult> textQueryResults = calculateQueryResultsRO( kwsSize, Ksize, buff, &pos, encTextIndex);
    delete[] buff;
    
    //if std
//    set<QueryResult,cmp_QueryResult> imgQueryResults = calculateQueryResults(newsockfd, vwsSize, Ksize, encImgIndex);
//    set<QueryResult,cmp_QueryResult> textQueryResults = calculateQueryResults(newsockfd, kwsSize, Ksize, encTextIndex);

    start = getTime();
    set<QueryResult,cmp_QueryResult> mergedResults = mergeSearchResults(&imgQueryResults, &textQueryResults);
    indexTime += diffSec(start, getTime());
    
    start = getTime();
    sendQueryResponse(newsockfd, &imgQueryResults/*mergedResults*/, -1);
    cloudTime += diffSec(start, getTime());
}

//search in std model
set<QueryResult,cmp_QueryResult> CashServer::calculateQueryResults(int newsockfd, int kwsSize, int Ksize, /*char* buff, int* pos,*/
                                                 map<vector<unsigned char>,vector<unsigned char> >* index) {
    map<int,double> queryResults;
    for (int i = 0; i < kwsSize; i++) {
        map<int,int> postingList;
        char smallBuff[sizeof(int)];
        receiveAll(newsockfd, smallBuff, sizeof(int));
        int pos = 0;
        
        int counter = readIntFromArr(smallBuff, &pos);
        int buffSize = Ksize + sizeof(int) + counter*Ksize;
        char* buff = new char[buffSize];
        receiveAll(newsockfd, buff, buffSize);
        pos = 0;
        unsigned char k2[Ksize];
        readFromArr(k2, Ksize*sizeof(unsigned char), buff, &pos);
        const int queryTf = readIntFromArr(buff, &pos);
        const double idf = getIdf(nDocs, counter);
        
        for (int j = 0; j < counter; j++) {
            vector<unsigned char> encCounter;
            encCounter.resize(Ksize);
            for (int z = 0; z < Ksize; z++) {
                readFromArr(&encCounter[z], sizeof(unsigned char), buff, &pos);
            }
            map<vector<unsigned char>,vector<unsigned char> >::iterator it = index->find(encCounter);
            if (it == index->end()) pee("CashServer::search received vw index position doesn't exist");
            int ciphertextSize = (int)it->second.size();
            unsigned char* rawPosting = (unsigned char*)malloc(ciphertextSize);
            dec(k2, it->second.data(), ciphertextSize, rawPosting);
            int pos2 = 0;
            int id = readIntFromArr((char*)rawPosting, &pos2);
            int tf = readIntFromArr((char*)rawPosting, &pos2);
            free(rawPosting);
            
            const double score = getTfIdf(queryTf, tf, idf);
            map<int,double>::iterator docScoreIt = queryResults.find(id);
            if (docScoreIt == queryResults.end())
                queryResults[id] = score;
            else
                docScoreIt->second += score;
        }
        delete[] buff;
    }
    return sort(&queryResults);
}

//search with Random Oracles
set<QueryResult,cmp_QueryResult> CashServer::calculateQueryResultsRO(int kwsSize, int Ksize, char* buff, int* pos,
                                                                   map<vector<unsigned char>,vector<unsigned char> >* index) {
    timespec start;
    map<int,double> queryResults;
    for (int i = 0; i < kwsSize; i++) {
        start = getTime();
        unsigned char k1[Ksize];
        readFromArr(k1, Ksize*sizeof(unsigned char), buff, pos);
        unsigned char k2[Ksize];
        readFromArr(k2, Ksize*sizeof(unsigned char), buff, pos);
        const int queryTf = readIntFromArr(buff, pos);
        cloudTime += diffSec(start, getTime());
        start = getTime();
        
        map<int,int> postingList;
        int c = 0;
        vector<unsigned char> encCounter = f(k1, Ksize, (unsigned char*)&c, sizeof(int));
        map<vector<unsigned char>,vector<unsigned char> >::iterator it = index->find(encCounter);
        while (it != index->end()) {
            int ciphertextSize = (int)it->second.size();
            unsigned char* rawPosting = new unsigned char[ciphertextSize];
            dec(k2, it->second.data(), ciphertextSize, rawPosting);
            int x = 0;
            int id = readIntFromArr((char*)rawPosting, &x);
            int tf = readIntFromArr((char*)rawPosting, &x);
            postingList[id] = tf;
            c++;
            delete[] rawPosting;
            encCounter = f(k1, Ksize, (unsigned char*)&c, sizeof(int));
            it = index->find(encCounter);
        }
        cryptoTime += diffSec(start, getTime());
        start = getTime();
        
        if (c != 0) {
            const double idf = getIdf(nDocs, c);
            for (map<int,int>::iterator it=postingList.begin(); it != postingList.end(); ++it) {
                const double score = scaledTfIdf/*getTfIdf*/(queryTf, it->second, idf);
                map<int,double>::iterator docScoreIt = queryResults.find(it->first);
                if (docScoreIt == queryResults.end())
                    queryResults[it->first] = score;
                else
                    docScoreIt->second += score;
            }
        }
        indexTime += diffSec(start, getTime());
    }
    start = getTime();
    std::set<QueryResult,cmp_QueryResult> sortedResults = sort(&queryResults);
    indexTime += diffSec(start, getTime());
    return sortedResults;
}

void CashServer::resetTime() {
    featureTime = 0;
    cryptoTime = 0;
    cloudTime = 0;
    indexTime = 0;
}

string CashServer::printTime() {
    char char_time[120];
    sprintf(char_time, "featureTime:%f cryptoTime:%f indexTime:%f cloudTime:%f",
            featureTime, cryptoTime, indexTime, cloudTime);
    string time = char_time;
    return time;
}
