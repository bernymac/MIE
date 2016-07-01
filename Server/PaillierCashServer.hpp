//
//  PaillierCashServer.hpp
//  MIE
//
//  Created by Bernardo Ferreira on 08/01/16.
//  Copyright © 2016 NovaSYS. All rights reserved.
//

#ifndef PaillierCashServer_hpp
#define PaillierCashServer_hpp

#include "CashServer.hpp"
#include "gmp.h"
extern "C" {
#include "paillier.h"
}

class PaillierCashServer : public CashServer {
    
    paillier_pubkey_t* homPub;
    
    void search(int newsockfd);
    map<int,paillier_ciphertext_t> calculateQueryResults(int newsockfd, int kwsSize, int Ksize, /*char* buff, int* pos,*/
                                                           map<vector<unsigned char>,vector<unsigned char> >* index);
    void sendPaillierQueryResponse (int newsockfd, initializer_list< map<int,paillier_ciphertext_t>* > queryResults);
    
public:
    PaillierCashServer() ;
    ~PaillierCashServer();
    
};

#endif /* PaillierCashServer_hpp */
