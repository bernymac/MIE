//
//  CashServer.hpp
//  MIE
//
//  Created by Bernardo Ferreira on 06/10/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#ifndef CashServer_hpp
#define CashServer_hpp

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
//#include <atomic>
//#include <thread>
#include "ServerUtil.h"

using namespace std;

class CashServer {
//    atomic<map<vector<unsigned char>,vector<unsigned char> > >* encImgIndex;
//    atomic<map<vector<unsigned char>,vector<unsigned char> > >* encTextIndex;
    map<vector<unsigned char>,vector<unsigned char> >* encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> >* encTextIndex;
    int nDocs;
    
    void startServer();
    void receiveDocs(int newsockfd);
    void search(int newsockfd);
//    void returnIndex(int newsockfd);
//    void sendPostingLists(int newsockfd, vector<vector<unsigned char> >* imgPostingLists, map<vector<unsigned char>, vector<unsigned char> >* textPostingLists);
    set<QueryResult,cmp_QueryResult> calculateQueryResults(int kwsSize, int Ksize, char* buff, int* pos, map<vector<unsigned char>,vector<unsigned char> >* index);
public:
    CashServer();
    ~CashServer();
};



#endif /* CashServer_hpp */