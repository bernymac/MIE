//
//  PaillierCashClient.hpp
//  MIE
//
//  Created by Bernardo Ferreira on 02/11/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#ifndef PaillierCashClient_hpp
#define PaillierCashClient_hpp

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <set>
#include "EnglishAnalyzer.h"
#include "CashCrypt.hpp"
#include "CashClient.hpp"
#include "Util.h"
#include "gmp.h"
extern "C" {
#include "paillier.h"
}


using namespace std;
using namespace cv;

class PaillierCashClient : public CashClient {
    
    paillier_pubkey_t* homPub;
    paillier_prvkey_t* homPriv;
    
    void encryptAndIndex(void* keyword, int keywordSize, int counter, int docId, int tf,
                         map<vector<unsigned char>, vector<unsigned char> >* index);
    vector<QueryResult> receiveResults(int sockfd);
    set<QueryResult,cmp_QueryResult> calculateQueryResults(int sockfd);
    
public:
    PaillierCashClient();
    ~PaillierCashClient();
    
};


#endif /* PaillierCashClient_hpp */
