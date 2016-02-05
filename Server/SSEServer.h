//
//  SSEServer.h
//  MIE
//
//  Created by Bernardo Ferreira on 03/05/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#ifndef __MIE__SSEServer__
#define __MIE__SSEServer__

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
#include "Server.h"

using namespace std;

class SSEServer : public Server {
    vector<vector<unsigned char> >* encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> >* encTextIndex;
    
    void startServer();
    void receiveDocs(int newsockfd);
    void search(int newsockfd);
    void returnIndex(int newsockfd);
    void sendPostingLists(int newsockfd, vector<vector<unsigned char> >* imgPostingLists, map<vector<unsigned char>, vector<unsigned char> >* textPostingLists);
public:
    SSEServer();
    ~SSEServer();
};

#endif