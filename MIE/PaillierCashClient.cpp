//
//  PaillierCashClient.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 02/11/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//

#include "PaillierCashClient.hpp"

PaillierCashClient::PaillierCashClient() {
    //automatically runs super() at start
    const string keyFilename = homePath;
    FILE* fHomPub = fopen((keyFilename+"Data/Client/Cash/homPub").c_str(), "rb");
    FILE* fHomPriv = fopen((keyFilename+"Data/Client/Cash/homPriv").c_str(), "rb");
    if (fHomPub != NULL && fHomPriv != NULL) {
        fseek(fHomPub,0,SEEK_END);
        const int pubKeySize = (int)ftell(fHomPub);
        fseek(fHomPub,0,SEEK_SET);
        char* pubHex = new char[pubKeySize];
        fread (pubHex, 1, pubKeySize, fHomPub);
        homPub = paillier_pubkey_from_hex(pubHex);
        delete[] pubHex;
        
        fseek(fHomPriv,0,SEEK_END);
        const int privKeySize = (int)ftell(fHomPriv);
        fseek(fHomPriv,0,SEEK_SET);
        char* privHex = new char[privKeySize];
        fread (privHex, 1, privKeySize, fHomPriv);
        homPriv = paillier_prvkey_from_hex(privHex, homPub);
        delete[] privHex;
    } else {
        paillier_keygen(1024, &homPub, &homPriv, paillier_get_rand_devurandom);
        fHomPub = fopen((keyFilename+"Data/Client/Cash/homPub").c_str(), "wb");
        char* pubHex = paillier_pubkey_to_hex(homPub);
        fwrite(pubHex, 1, strlen(pubHex), fHomPub);
        fHomPriv = fopen((keyFilename+"Data/Client/Cash/homPriv").c_str(), "wb");
        char* privHex = paillier_prvkey_to_hex(homPriv);
        fwrite(privHex, 1, strlen(privHex), fHomPriv);
    }
    fclose(fHomPub);
    fclose(fHomPriv);
}

PaillierCashClient::~PaillierCashClient() {
    delete homPub;
    delete homPriv;
}


void PaillierCashClient::encryptAndIndex(void* keyword, int keywordSize, int counter, int docId, int tf,
                                 map<vector<unsigned char>, vector<unsigned char> >* index) {
    //calculate index position through hmac of counter value
    unsigned char k1[CashCrypt::Ksize];
    int append = 1;
    crypto->deriveKey(&append, sizeof(int), keyword, keywordSize, k1);
    unsigned char k2[CashCrypt::Ksize];
    append = 2;
    crypto->deriveKey(&append, sizeof(int), keyword, keywordSize, k2);
    vector<unsigned char> encCounter = crypto->encCounter(k1, (unsigned char*)&counter, sizeof(int));
    //encrypt docid with aes
    unsigned char idBuff[sizeof(int)];
    int pos = 0;
    addIntToArr(docId, (char*)idBuff, &pos);
    vector<unsigned char> encData = crypto->encData(k2, idBuff, pos);
    
    //encrypt tf with paillier
    paillier_plaintext_t plainTF;
    mpz_init_set_ui(plainTF.m, tf);
    paillier_ciphertext_t encTF;
    mpz_init(encTF.c);
    paillier_enc(&encTF, homPub, &plainTF, paillier_get_rand_devurandom);
    mpz_clear(plainTF.m);
    const int ciphertextSize = paillier_get_ciphertext_size(homPub);
    unsigned char encTFBuff[ciphertextSize];
    mpz_export(encTFBuff, 0, 1, 1, 0, 0, encTF.c);
    mpz_clear(encTF.c);
//    paillier_ciphertext_to_bytes(ciphertextSize, &encTF, encTFBuff);

    //concatenate id and tf
    int encIdSize = (int)encData.size();
    encData.resize(encIdSize + ciphertextSize);
    for (int i = 0; i < ciphertextSize; i++)
        encData[encIdSize+i] = encTFBuff[i];
//    free(encTFBuff);
    
    if (index->find(encCounter) != index->end())
        pee("Found an unexpected collision in CashClient::encryptAndIndex");
    (*index)[encCounter] = encData;
    
    //    if (keywordSize > 4) {
    //        LOGI("vw 0 k1: %s\n",getHexRepresentation(k1,CashCrypt::Ksize).c_str());
    //        LOGI("vw 0 k2: %s\n",getHexRepresentation(k2,CashCrypt::Ksize).c_str());
    //        LOGI("vw 0 counter 0 EncCounter: %s\n",getHexRepresentation(encCounter.data(),CashCrypt::Ksize).c_str());
    //        LOGI("vw 0 counter 0 EncData: %s\n",getHexRepresentation(encData.data(),16).c_str());
    //    }
}

vector<QueryResult> PaillierCashClient::receiveResults(int sockfd) {
//    LOGI("Receiving img results...\n");
    set<QueryResult,cmp_QueryResult> imgQueryResults = calculateQueryResults(sockfd);
//    LOGI("Receiving text results...\n");
    set<QueryResult,cmp_QueryResult> textQueryResults = calculateQueryResults(sockfd);
//    LOGI("Merging img and text results...\n");
    
    timespec start = getTime(); 
    set<QueryResult,cmp_QueryResult> mergedResults = mergeSearchResults(&imgQueryResults, &textQueryResults);
//    LOGI("Finished merging...\n");
    
    vector<QueryResult> results;
    size_t resultsSize = mergedResults.size() < 20 ? mergedResults.size() : 20;
    results.resize(resultsSize);
    int i = 0;
    for (set<QueryResult,cmp_QueryResult>::iterator it = mergedResults.begin(); it != mergedResults.end(); ++it) {
        if (i == resultsSize)
            break;
        results[i] = *it;
        i++;
    }
    indexTime += diffSec(start, getTime());
    return results;
}

set<QueryResult,cmp_QueryResult> PaillierCashClient::calculateQueryResults(int sockfd) {
    timespec start = getTime();
    const int homEncSize = paillier_get_ciphertext_size(homPub);
    char resultsSizeBuff[sizeof(int)];
    socketReceive(sockfd, resultsSizeBuff, sizeof(int));
    int pos = 0;
    const int nResults = readIntFromArr(resultsSizeBuff, &pos);

    map<int,double> queryResults;
    int buffSize = nResults * (sizeof(int) + homEncSize);
    char* buff = new char[buffSize];
    if (buff == NULL) pee("malloc error in PaillierCashClient::receiveResults");
    socketReceive(sockfd, buff, buffSize);
    pos = 0;
    cloudTime += diffSec(start, getTime());
    start = getTime();
    
    paillier_ciphertext_t encTf;
    paillier_plaintext_t plaintextTf;
    mpz_init(encTf.c);
    mpz_init(plaintextTf.m);
    char* encTFBuff = new char[homEncSize];
    char* decTFBuff = new char [homEncSize];
    for (int i = 0; i < nResults; i++) {
        const int docId = readIntFromArr(buff, &pos);
        readFromArr(encTFBuff, homEncSize, buff, &pos);
        mpz_import(encTf.c, homEncSize, 1, 1, 0, 0, encTFBuff/*buff+pos*/);
//        pos += homEncSize;
        //decrypt paillier scores
        paillier_dec(&plaintextTf, homPub, homPriv, &encTf);
        int tf = 0;
        mpz_export(decTFBuff, 0, 1, 1, 0, 0, plaintextTf.m);
        memcpy(&tf, decTFBuff, sizeof(int));
        queryResults[docId] = tf; 
    }
    mpz_clear(encTf.c);
    mpz_clear(plaintextTf.m);
    delete[] buff;
    delete[] encTFBuff;
    delete[] decTFBuff;
    cryptoTime += diffSec(start, getTime());
    start = getTime();
    
    std::set<QueryResult,cmp_QueryResult> sortedResults = sort(&queryResults);
    indexTime += diffSec(start, getTime());
    
    return sortedResults;

}
