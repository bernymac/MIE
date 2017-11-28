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
#include "Server.h"
#include "ServerUtil.h"

using namespace std;

class CashServer : public Server {
    
protected:
    double featureTime, cryptoTime, cloudTime, indexTime;
    
//    atomic<map<vector<unsigned char>,vector<unsigned char> > >* encImgIndex;
//    atomic<map<vector<unsigned char>,vector<unsigned char> > >* encTextIndex;
    map<vector<unsigned char>,vector<unsigned char> >* encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> >* encTextIndex;
    int nDocs;
    
    void startServer();
    void receiveDocs(int newsockfd);
    virtual void search(int newsockfd);
//    void returnIndex(int newsockfd);
//    void sendPostingLists(int newsockfd, vector<vector<unsigned char> >* imgPostingLists, map<vector<unsigned char>, vector<unsigned char> >* textPostingLists);

    set<QueryResult,cmp_QueryResult> calculateQueryResults(int newsockfd, int kwsSize, int Ksize,                      map<vector<unsigned char>,vector<unsigned char> >* index);
    set<QueryResult,cmp_QueryResult> calculateQueryResultsRO(int kwsSize, int Ksize, char* buff, int* pos,                                                                        map<vector<unsigned char>,vector<unsigned char> >* index);
    
    void resetTime();
    string printTime();
public:
    CashServer();
    ~CashServer();
};



#endif /* CashServer_hpp */
