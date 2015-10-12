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
//            case 'd':
//                returnIndex(newsockfd);
//                break;
            case 's':
                search(newsockfd);
                break;
            default:
                pee("unkonwn command!\n");
        }
        ack(newsockfd);
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
//    encImgIndex->resize(imgIndexSize);
    for (int i = 0; i < imgIndexSize; i++) {
/*        vector<unsigned char> encPosting;
        encPosting.resize(postingSize);
        buffSize = postingSize*sizeof(unsigned char) + sizeof(int);
        char* buff = (char*)malloc(buffSize);
        pos = 0;
        receiveAll(newsockfd, buff, buffSize);
        for (int j = 0; j < postingSize; j++)
            readFromArr(&encPosting[j], sizeof(unsigned char), buff, &pos);
        postingSize = readIntFromArr(buff, &pos);
        (*encImgIndex)[i] = postingList;
        free(buff);*/
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
    //    map<int, vector<unsigned char> > imgPostingLists;
//    vector<vector<unsigned char> > imgPostingLists;
//    imgPostingLists.resize(encImgIndex->size());
//    map<vector<unsigned char>, vector<unsigned char> > textPostingLists;
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

/*        map<vector<unsigned char>,vector<unsigned char> >::iterator it = encImgIndex->find(vw);
        if (it != encTextIndex->end())
            textPostingLists[encKeyword] = it->second;
        
        const int vw = readIntFromArr(buff, &pos);
        vector<unsigned char> encPostingList = (*encImgIndex)[vw];
        if (encPostingList.size() > 0)
            imgPostingLists[vw] = encPostingList;
 
    }*/
//    free(buff);
    
    //receive keywords and fetch text posting lists
//    buffSize = encKeywordsSize*keywordSize;
//    buff = (char*)malloc(buffSize);
//    if (buff == NULL) pee("malloc error in CashServer::search receive keywords");
//    receiveAll(newsockfd, buff, buffSize);
//    pos = 0;
/*    for (int i = 0; i < encKeywordsSize; i++) {
        vector<unsigned char> encKeyword;
        encKeyword.resize(keywordSize);
        for (int j = 0; j < keywordSize; j++) {
            unsigned char x;
            readFromArr(&x, sizeof(unsigned char), buff, &pos);
            encKeyword[j] = x;
        }
        map<vector<unsigned char>,vector<unsigned char> >::iterator it = encTextIndex->find(encKeyword);
        if (it != encTextIndex->end())
            textPostingLists[encKeyword] = it->second;
    }
    free(buff);*/
    
    //send posting lists
//    sendPostingLists(newsockfd, &imgPostingLists, &textPostingLists);
    
}

/*void CashServer::returnIndex(int newsockfd) {
    sendPostingLists(newsockfd, encImgIndex, encTextIndex);
    //    receiveDocs(newsockfd);
}*/
    
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


/*void CashServer::sendPostingLists(int newsockfd, vector<vector<unsigned char> >* imgPostingLists, map<vector<unsigned char>, vector<unsigned char> >* textPostingLists) {
    long buffSize = 2*sizeof(int);
    //    int textSize = sizeof(int);
    //    for (map<int, vector<unsigned char> >::iterator it=imgPostingLists.begin(); it!=imgPostingLists.end(); ++it)
    //        buffSize += 2*sizeof(int) + it->second.size()*sizeof(unsigned char);
    int nImgPostingLists = 0;
    for (int i = 0; i < imgPostingLists->size(); i++) {
        const int postingListSize = (int)(*imgPostingLists)[i].size();
        if (postingListSize > 0) {
            buffSize += 2*sizeof(int) + postingListSize*sizeof(unsigned char);
            nImgPostingLists++;
        }
    }
    for (map<vector<unsigned char>, vector<unsigned char> >::iterator it=textPostingLists->begin(); it!=textPostingLists->end(); ++it) {
        buffSize += it->first.size()*sizeof(unsigned char) + sizeof(int) + it->second.size()*sizeof(unsigned char);
        //        textSize += it->first.size()*sizeof(unsigned char) + sizeof(int) + it->second.size()*sizeof(unsigned char);
    }
    //    textSize += sizeof(int);
    //    printf("Text Search Network Traffic part 2: %d\n",textSize);
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in CashServer::search send posting lists");
    int pos = 0;
    //    addIntToArr((int)imgPostingLists->size(), buff, &pos);
    addIntToArr(nImgPostingLists, buff, &pos);
    addIntToArr((int)textPostingLists->size(), buff, &pos);
    
    /*    for (map<int, vector<unsigned char> >::iterator it=imgPostingLists.begin(); it!=imgPostingLists.end(); ++it) {
     addIntToArr((int)it->second.size(), buff, &pos);
     addIntToArr(it->first, buff, &pos);
     for (int i = 0; i < it->second.size(); i++)
     addToArr(&it->second[i], sizeof(unsigned char), buff, &pos);
     } */

/*    for (int i = 0; i < imgPostingLists->size(); i++) {
        const int postingListSize = (int)(*imgPostingLists)[i].size();
        if (postingListSize > 0) {
            addIntToArr(postingListSize, buff, &pos);
            addIntToArr(i, buff, &pos);
            for (int j = 0; j < postingListSize; j++)
                addToArr(&(*imgPostingLists)[i][j], sizeof(unsigned char), buff, &pos);
        }
    }
    for (map<vector<unsigned char>,vector<unsigned char> >::iterator it=textPostingLists->begin(); it!=textPostingLists->end(); ++it) {
        addIntToArr((int)it->second.size(), buff, &pos);
        for (int i = 0; i < it->first.size(); i++) {
            unsigned char x = it->first[i];
            addToArr(&x, sizeof(unsigned char), buff, &pos);
        }
        for (int i = 0; i < it->second.size(); i++)
            addToArr(&it->second[i], sizeof(unsigned char), buff, &pos);
    }
    socketSend (newsockfd, buff, buffSize);
    //    printf("Search Network Traffic part 2: %ld\n",buffSize);
    free(buff);
}*/
