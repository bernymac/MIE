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
    startServer();
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
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) pee("ERROR on accept");
        char buffer[1];
        if (read(newsockfd,buffer,1) < 0) pee("ERROR reading from socket");
        //        printf("command received: %c\n",buffer[0]);
        switch (buffer[0]) {
            case 'a':
                receiveDocs(newsockfd);
                break;
            case 's':
                search(newsockfd);
                break;
            default:
                pee("unkonwn command!\n");
        }
//        ack(newsockfd);
        close(newsockfd);
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
        if (encImgIndex->find(vw)!=encImgIndex->end())                  //debug
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
    //initialize and read variables
    long buffSize = 3*sizeof(int);
    char buffer[buffSize];
    if (read(newsockfd,buffer,buffSize) < 0) pee("CashServer::search error reading from socket");
    int pos = 0;
    const int vwsSize = readIntFromArr(buffer, &pos);
    const int kwsSize = readIntFromArr(buffer, &pos);
    const int Ksize = readIntFromArr(buffer, &pos);
    
    buffSize = (vwsSize + kwsSize) * (2*Ksize + sizeof(int));
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in CashServer::search receive vws and kws");
    receiveAll(newsockfd, buff, buffSize);
    pos = 0;
    set<QueryResult,cmp_QueryResult> imgQueryResults = calculateQueryResults(vwsSize, Ksize, buff, &pos, encImgIndex);
    set<QueryResult,cmp_QueryResult> textQueryResults = calculateQueryResults(kwsSize, Ksize, buff, &pos, encTextIndex);
    free(buff);
    set<QueryResult,cmp_QueryResult> mergedResults = mergeSearchResults(&imgQueryResults, &textQueryResults);
    sendQueryResponse(newsockfd, &mergedResults);
}

    
set<QueryResult,cmp_QueryResult> CashServer::calculateQueryResults(int kwsSize, int Ksize, char* buff, int* pos,
                                                 map<vector<unsigned char>,vector<unsigned char> >* index) {
    map<int,float> queryResults;
    for (int i = 0; i < kwsSize; i++) {
        unsigned char k1[Ksize];
        readFromArr(k1, Ksize*sizeof(unsigned char), buff, pos);
        unsigned char k2[Ksize];
        readFromArr(k2, Ksize*sizeof(unsigned char), buff, pos);
        const int queryTf = readIntFromArr(buff, pos);
//        printf("vw 0 k1: %s\n",getHexRepresentation(k1,Ksize).c_str());
//        printf("vw 0 k2: %s\n",getHexRepresentation(k2,Ksize).c_str());
        
        map<int,int> postingList;
        int c = 0;
        vector<unsigned char> encCounter = f(k1, Ksize, (unsigned char*)&c, sizeof(int));
        map<vector<unsigned char>,vector<unsigned char> >::iterator it = index->find(encCounter);
        while (it != index->end()) {
            int ciphertextSize = (int)it->second.size();
//            unsigned char* ciphertext = (unsigned char*)malloc(ciphertextSize);
//            for(int j = 0; j < ciphertextSize; j++)
//                ciphertext[j] = it->second[j];
            unsigned char* rawPosting = (unsigned char*)malloc(ciphertextSize);
            dec(k2, it->second.data(), ciphertextSize, rawPosting);
            int x = 0;
            int id = readIntFromArr((char*)rawPosting, &x);
            int tf = readIntFromArr((char*)rawPosting, &x);
//            printf("docId: %i tf: %i\n",id,tf);
            postingList[id] = tf;
            c++;
//            free(ciphertext);
            free(rawPosting);
//            printf("vw 0 counter 0 EncCounter: %s\n",getHexRepresentation(encCounter.data(),Ksize).c_str());
//            printf("vw 0 counter 0 EncData: %s\n",getHexRepresentation(it->second.data(),16).c_str());
            encCounter = f(k1, Ksize, (unsigned char*)&c, sizeof(int));
            it = index->find(encCounter);
        }
        if (c != 0) {
            const float idf = getIdf(nDocs, c);
            for (map<int,int>::iterator it=postingList.begin(); it != postingList.end(); ++it) {
                const float score = getTfIdf(queryTf, it->second, idf);
                map<int,float>::iterator docScoreIt = queryResults.find(it->first);
                if (docScoreIt == queryResults.end())
                    queryResults[it->first] = score;
                else
                    docScoreIt->second += score;
            }
        }
    }
    return sort(queryResults);
}
