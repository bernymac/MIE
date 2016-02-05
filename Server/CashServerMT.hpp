//
//  CashServerMT.hpp
//  MIE
//
//  Created by Bernardo Ferreira on 18/10/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#ifndef CashServerMT_hpp
#define CashServerMT_hpp

#include <vector>
#include <map>
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
#include "ServerUtil.h"
#include "ThreadPool.h"
#include "Server.h"
#include <mutex>

using namespace std;

class CashServerMT : public Server {
    
    struct encCounter {
        vector<unsigned char> value;
        mutex lock;
    };

    static map<vector<unsigned char>,vector<unsigned char> > encImgIndex;
    static map<vector<unsigned char>,vector<unsigned char> > encTextIndex;
    static atomic<int> nDocs;
    static map<int,encCounter> imgDcount;
    static map<string,encCounter> textDcount;
    static mutex imgIndexLock, textIndexLock;
    
    void startServer();
    static void clientThread(int newsockfd);
    static void receiveDocs(int newsockfd);
    static void search(int newsockfd);
    static set<QueryResult,cmp_QueryResult> calculateQueryResults(int kwsSize, int Ksize, char* buff, int* pos, map<vector<unsigned char>,vector<unsigned char> >* index, mutex* indexLock);
    
public:
    CashServerMT();
    ~CashServerMT();
};

#endif /* CashServerMT_hpp */
