//
//  PaillierCashServer.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 08/01/16.
//  Copyright © 2016 NovaSYS. All rights reserved.
//

#include "PaillierCashServer.hpp"


PaillierCashServer::PaillierCashServer() {
    printf("Starting Paillier Cash Server...\n");
    FILE* fHomPub = fopen((dataPath+"Cash/homPub").c_str(), "rb");
    if (fHomPub != NULL) {
        fseek(fHomPub,0,SEEK_END);
        const int pubKeySize = (int)ftell(fHomPub);
        fseek(fHomPub,0,SEEK_SET);
        char* pubHex = new char[pubKeySize];
        fread (pubHex, 1, pubKeySize, fHomPub);
        homPub = paillier_pubkey_from_hex(pubHex);
        delete pubHex;
    } else
        pee("Couldn't read Paillier Public Key");
}

PaillierCashServer::~PaillierCashServer() {
    delete homPub;
}


void PaillierCashServer::search(int newsockfd) {
    //initialize and read variables
    long buffSize = 3*sizeof(int);
    char buffer[buffSize];
    receiveAll(newsockfd, buffer, buffSize);
    int pos = 0;
    const int vwsSize = readIntFromArr(buffer, &pos);
    const int kwsSize = readIntFromArr(buffer, &pos);
    const int Ksize = readIntFromArr(buffer, &pos);
    
    //    buffSize = (vwsSize + kwsSize) * (2*Ksize + sizeof(int));
    //    char* buff = (char*)malloc(buffSize);
    //    if (buff == NULL) pee("malloc error in CashServer::search receive vws and kws");
    //    receiveAll(newsockfd, buff, buffSize);
    //    pos = 0;
    map<int,paillier_ciphertext_t> imgQueryResults = calculateQueryResults(newsockfd, vwsSize, Ksize, encImgIndex);
    map<int,paillier_ciphertext_t> textQueryResults = calculateQueryResults(newsockfd, kwsSize, Ksize, encTextIndex);
    //    free(buff);
//    set<QueryResult,cmp_QueryResult> mergedResults = mergeSearchResults(&imgQueryResults, &textQueryResults);
    sendPaillierQueryResponse(newsockfd, {&imgQueryResults, &textQueryResults} );
}


map<int,paillier_ciphertext_t> PaillierCashServer::calculateQueryResults(int newsockfd, int kwsSize, int Ksize,
                                                                           map<vector<unsigned char>,vector<unsigned char> >* index) {
    map<int,paillier_ciphertext_t> queryResults;
    for (int i = 0; i < kwsSize; i++) {
        map<int,int> postingList;
        char smallBuff[sizeof(int)];
        receiveAll(newsockfd, smallBuff, sizeof(int));
        int pos = 0;
        int counter = readIntFromArr(smallBuff, &pos);
        int buffSize = Ksize*sizeof(unsigned char) + sizeof(int) + counter*Ksize*sizeof(unsigned char);
        char* buff = (char*)malloc(buffSize);
        if (buff == NULL) pee("malloc error in CashServer::search receive vws");
        receiveAll(newsockfd, buff, buffSize);
        pos = 0;
        unsigned char k2[Ksize];
        readFromArr(k2, Ksize*sizeof(unsigned char), buff, &pos);
        const int queryTf = readIntFromArr(buff, &pos);
        const float idf = getIdf(nDocs, counter);
        
        paillier_ciphertext_t tf;
        mpz_init(tf.c);
        paillier_plaintext_t ptx;
        mpz_init(ptx.m);
        for (int j = 0; j < counter; j++) {
            vector<unsigned char> encCounter;
            encCounter.resize(Ksize);
            for (int z = 0; z < Ksize; z++)
                readFromArr(&encCounter[z], sizeof(unsigned char), buff, &pos);
            map<vector<unsigned char>,vector<unsigned char> >::iterator it = index->find(encCounter);
            if (it == index->end()) pee("CashServer::search received vw index position doesn't exist");
            
            //get docId and hom tf
            const int encTfSize = paillier_get_ciphertext_size(homPub);
            const int encDocIdSize = (int)it->second.size() - encTfSize;// 16;   //aes block size
            unsigned char encDocId [encDocIdSize];
            for (int i = 0; i < encDocIdSize; i++)
                encDocId[i] = it->second[i];
            unsigned char rawPosting [encDocIdSize];
            dec(k2, it->second.data(), encDocIdSize, rawPosting);
            int pos2 = 0;
            int id = readIntFromArr((char*)rawPosting, &pos2);
//            free(rawPosting);
            char encTf [encTfSize];
            for (int i = encDocIdSize; i < it->second.size(); i++)
                encTf[i] = it->second[i];
            mpz_import(tf.c, encTfSize, 1, 1, 0, 0, encTf);
            
            //calculate hom score
            mpz_set_ui(ptx.m, (int)idf);
            paillier_exp(homPub, &tf, &tf, &ptx);
            mpz_set_ui(ptx.m,queryTf);
            paillier_exp(homPub, &tf, &tf, &ptx);
            map<int,paillier_ciphertext_t>::iterator docScoreIt = queryResults.find(id);
            if (docScoreIt == queryResults.end())
                queryResults[id] = tf;
            else
                paillier_mul(homPub, &docScoreIt->second, &tf, &docScoreIt->second);

        }
        free(buff);
//        mpz_clear(ptx.m);
//        mpz_clear(tf.c);
    }
        return queryResults;
}

void PaillierCashServer::sendPaillierQueryResponse(int newsockfd, initializer_list< map<int,paillier_ciphertext_t>* > queryResults) {
    const int encSize = paillier_get_ciphertext_size(homPub);
    long size = 0;
    for (initializer_list< map<int,paillier_ciphertext_t>* >::iterator it = queryResults.begin(); it != queryResults.end(); ++it)
        size += sizeof(int) + (*it)->size() * (sizeof(int) + encSize);
    char* buff = (char*)malloc(size);
    if (buff == NULL) pee("malloc error in PaillierCashServer::sendPaillierQueryResponse");
    int pos = 0;
    for (initializer_list< map<int,paillier_ciphertext_t>* >::iterator it = queryResults.begin(); it != queryResults.end(); ++it) {
        addIntToArr ((int)(*it)->size(), buff, &pos);
        for (map<int,paillier_ciphertext_t>::const_iterator it2 = (*it)->begin(); it2 != (*it)->end(); ++it2) {
            addIntToArr(it2->first, buff, &pos);
            char* encTFBuff = new char[encSize];
            mpz_export(encTFBuff/*buff+pos*/, 0, 1, 1, 0, 0, it2->second.c);
//            pos += encSize;
//          mpz_clear(it2->second.c);
            addToArr(encTFBuff, encSize, buff, &pos);
            delete encTFBuff;
        }
    }
    socketSend (newsockfd, buff, size);
    printf("Search Network Traffic part 2: %ld\n",size);
    free(buff);
}

