//
//  SSEServer.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 30/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "SSEServer.h"

using namespace std;

SSEServer::SSEServer() {
//    startServer();
}

SSEServer::~SSEServer() {
    free(encImgIndex);
    free(encTextIndex);
}

void SSEServer::startServer() {
    encImgIndex = new vector<vector<unsigned char> >;
    encTextIndex = new map<vector<unsigned char>,vector<unsigned char> >;
    int sockfd = initServer();
    printf("SSE server started!\n");
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
            case 'd':
                returnIndex(newsockfd);
                break;
            case 's':
                search(newsockfd);
                break;
            default:
                pee("unkonwn command!\n");
        }
        ack(newsockfd);
    }
}

void SSEServer::receiveDocs(int newsockfd) {
    int buffSize = 4*sizeof(int);
    char buffer[buffSize];
    bzero(buffer,buffSize);
    if (read(newsockfd,buffer,buffSize) < 0) pee("SSEServer::receiveDocs ERROR reading from socket");
    int pos = 0;
    const int imgIndexSize = readIntFromArr(buffer, &pos);
    const int textIndexSize = readIntFromArr(buffer, &pos);
    const int keywordSize = readIntFromArr(buffer, &pos);
    int postingListSize = readIntFromArr(buffer, &pos);
    encImgIndex->resize(imgIndexSize);
    for (int i = 0; i < imgIndexSize; i++) {
        vector<unsigned char> postingList;
        postingList.resize(postingListSize);
        buffSize = postingListSize*sizeof(unsigned char) + sizeof(int);
        char* buff = (char*)malloc(buffSize);
//        bzero(buff, buffSize);
        pos = 0;
        receiveAll(newsockfd, buff, buffSize);
        for (int j = 0; j < postingListSize; j++)
            readFromArr(&postingList[j], sizeof(unsigned char), buff, &pos);
        postingListSize = readIntFromArr(buff, &pos);
        (*encImgIndex)[i] = postingList;
        free(buff);
    }
    for (int i = 0; i < textIndexSize; i++) {
        vector<unsigned char> keyword;
        keyword.resize(keywordSize);
        vector<unsigned char> postingList;
        postingList.resize(postingListSize);
        buffSize = keywordSize*sizeof(unsigned char) + postingListSize*sizeof(unsigned char);
        if (i < textIndexSize-1)
            buffSize += sizeof(int);
        char* buff = (char*)malloc(buffSize);
        bzero(buff, buffSize);
        pos = 0;
        receiveAll(newsockfd, buff, buffSize);
        for (int j = 0; j < keywordSize; j++)
            readFromArr(&keyword[j], sizeof(unsigned char), buff, &pos);
        for (int j = 0; j < postingListSize; j++)
            readFromArr(&postingList[j], sizeof(unsigned char), buff, &pos);
        if (i < textIndexSize-1)
            postingListSize = readIntFromArr(buff, &pos);
        (*encTextIndex)[keyword] = postingList;
        free(buff);
    }
//    printf("finished receiving docs\n");
}

void SSEServer::search(int newsockfd) {
    //initialize and read variables
//    map<int, vector<unsigned char> > imgPostingLists;
    vector<vector<unsigned char> > imgPostingLists;
    imgPostingLists.resize(encImgIndex->size());
    map<vector<unsigned char>, vector<unsigned char> > textPostingLists;
    long buffSize = 3*sizeof(int);
    char buffer[buffSize];
    if (read(newsockfd,buffer,buffSize) < 0) pee("SSEServer::search error reading from socket");
    int pos = 0;
    const int vwsSize = readIntFromArr(buffer, &pos);
    const int encKeywordsSize = readIntFromArr(buffer, &pos);
    const int keywordSize = readIntFromArr(buffer, &pos);
    
    //receive vws and fetch img posting lists
    buffSize = vwsSize*sizeof(int);
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in SSEServer::search receive vws");
    receiveAll(newsockfd, buff, buffSize);
    pos = 0;
    for (int i = 0; i < vwsSize; i++) {
        const int vw = readIntFromArr(buff, &pos);
        vector<unsigned char> encPostingList = (*encImgIndex)[vw];
        if (encPostingList.size() > 0)
            imgPostingLists[vw] = encPostingList;
    }
    free(buff);

    //receive keywords and fetch text posting lists
    buffSize = encKeywordsSize*keywordSize;
    buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in SSEServer::search receive keywords");
    receiveAll(newsockfd, buff, buffSize);
    pos = 0;
    for (int i = 0; i < encKeywordsSize; i++) {
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
    free(buff);
    
    //send posting lists
    sendPostingLists(newsockfd, &imgPostingLists, &textPostingLists);
}

void SSEServer::returnIndex(int newsockfd) {
    sendPostingLists(newsockfd, encImgIndex, encTextIndex);
//    receiveDocs(newsockfd);
}

void SSEServer::sendPostingLists(int newsockfd, vector<vector<unsigned char> >* imgPostingLists, map<vector<unsigned char>, vector<unsigned char> >* textPostingLists) {
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
    if (buff == NULL) pee("malloc error in SSEServer::search send posting lists");
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
    for (int i = 0; i < imgPostingLists->size(); i++) {
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
}

