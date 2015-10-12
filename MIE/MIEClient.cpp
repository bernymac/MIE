//
//  MIEClient.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 27/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "MIEClient.h"

MIEClient::MIEClient() {
    cryptoTime = 0.;
    cloudTime = 0.;
    detector = FeatureDetector::create( /*"Dense"*/ "PyramidDense" );
    extractor = DescriptorExtractor::create( "SURF" );
    analyzer = new EnglishAnalyzer;
    sbe = new SBE(extractor->descriptorSize());
    textCrypto = new TextCrypt;
}

MIEClient::~MIEClient() {
    detector.release();
    extractor.release();
    free(analyzer);
    free(sbe);
    free(textCrypto);
}

void MIEClient::addDocs(const char* imgDataset, const char* textDataset, int first, int last, int prefix) {
    for (int id = first; id <= last; id++) {
        vector< vector<float> > imgFeatures;
        vector< vector<unsigned char> > encKeywords;
        processDoc(id, imgDataset, textDataset, &imgFeatures, &encKeywords);
        
        timespec start = getTime();
        int sockfd = sendDoc('a', id+prefix, &imgFeatures, &encKeywords);
        cloudTime += diffSec(start, getTime());
        
        socketReceiveAck(sockfd);
    }
}

void MIEClient::processDoc(int id, const char* imgDataset, const char* textDataset, vector< vector<float> >* features, vector<vector<unsigned char> >* encKeywords) {
    //extract text features
    char* fname = (char*)malloc(120);
    if (fname == NULL) pee("malloc error in MIEClient::processDoc");
    bzero(fname, 120);
    sprintf(fname, "%s/%s/%d.jpg", datasetsPath, imgDataset, id);
    Mat image = imread(fname);
    vector<KeyPoint> keypoints;
    Mat descriptors;
    detector->detect(image, keypoints);
    extractor->compute(image, keypoints, descriptors);
    
    //extract text features
    bzero(fname, 120);
    sprintf(fname, "%s/%s/tags%d.txt", datasetsPath, textDataset, id+1);
    vector<string> keywords = analyzer->extractFile(fname);
    
    timespec start = getTime();     //start crypto benchmark
    //encrypt image features
    features->resize(descriptors.rows);
    for (int i = 0; i < descriptors.rows; i++) {
        vector<float> feature;
        feature.resize(descriptors.cols);
        for (int j = 0; j < descriptors.cols; j++)
            feature[j] = descriptors.at<float>(i, j);
        (*features)[i] = sbe->encode(feature);
    }
    //encrypt text features
    encKeywords->resize(keywords.size());
    for (int i = 0; i < keywords.size(); i++)
        (*encKeywords)[i] = textCrypto->encode(keywords[i]);
    cryptoTime += diffSec(start, getTime());     //end crypto benchmark
    free(fname);
}

int MIEClient::sendDoc(char op, int id, vector< vector<float> >* features, vector< vector<unsigned char> >* encKeywords) {
    long size = 5*sizeof(int) + sizeof(int)*features->size()*(*features)[0].size() + encKeywords->size()*TextCrypt::keysize;
    char* buff = (char*)malloc(size);
    if (buff == NULL) pee("malloc error in MIEClient::sendDoc");
    int pos = 0;
    addIntToArr (id, buff, &pos);
    addIntToArr (int(features->size()), buff, &pos);
    addIntToArr (int((*features)[0].size()), buff, &pos);
    addIntToArr (int(encKeywords->size()), buff, &pos);
    addIntToArr (TextCrypt::keysize, buff, &pos);
    for (int i = 0; i < features->size(); i++)
        for (int j = 0; j < (*features)[i].size(); j++)
            addIntToArr ((int)(*features)[i][j], buff, &pos);
    //and text features
    for (int i = 0; i < encKeywords->size(); i++) {
        for (int j = 0; j < (*encKeywords)[i].size(); j++) {
            unsigned char c = (*encKeywords)[i][j];
            addToArr(&c, sizeof(unsigned char), buff, &pos);
            //        memcpy(&buff[pos], (*encKeywords)[i].data(), TextCrypt::keysize);
            //        pos += TextCrypt::keysize;
        }
    }
    //upload
    char cmd[1];
    cmd[0] = op;
    int sockfd = connectAndSend(cmd, 1);
    zipAndSend(sockfd, buff, size);
//    int sockfd = connectAndSend (buff, size) ;
//    LOGI("Search network traffic part 1: %ld\n",size);
    free(buff);
    return sockfd;
}

void MIEClient::index() {
    char buff[1];
    buff[0] = 'i';
    int sockfd = connectAndSend(buff, 1);
    socketReceiveAck(sockfd);
}

vector<QueryResult> MIEClient::search(int id, const char* imgDataset, const char* textDataset) {
    vector< vector<float> > features;
    vector< vector<unsigned char> > encKeywords;
    processDoc(id, imgDataset, textDataset, &features, &encKeywords);
    
    timespec start = getTime();     //start cloud time benchmark
    int sockfd = sendDoc('s', id, &features, &encKeywords);
    vector<QueryResult> queryResults = receiveQueryResults(sockfd);
    cloudTime += diffSec(start, getTime()); //end
    
    socketReceiveAck(sockfd);
    return queryResults;
}

/*vector<QueryResult> MIEClient::receiveQueryResults(int sockfd) {
    char buff[sizeof(int)];
    socketReceive(sockfd, buff, sizeof(int));
    int pos = 0;
    int resultsSize = readIntFromArr(buff, &pos);
    vector<QueryResult> queryResults;
    queryResults.resize(resultsSize);
    
    int resultsBuffSize = resultsSize * (sizeof(int) + sizeof(uint64_t));
    char* buff2 = (char*) malloc(resultsBuffSize);
    if (buff2 == NULL) pee("malloc error in MIEClient::receiveQueryResults");
    int r = 0;
    while (r < resultsBuffSize) {
        ssize_t n = read(sockfd,&buff2[r],resultsBuffSize-r);
        if (n < 0) pee("ERROR reading from socket");
        r+=n;
    }
    pos = 0;
    for (int i = 0; i < resultsSize; i++) {
        struct QueryResult qr;
        qr.docId = readIntFromArr(buff2, &pos);
        qr.score = readFloatFromArr(buff2, &pos);
        queryResults[i] = qr;
    }
    free(buff2);
    return queryResults;
}*/

string MIEClient::printTime() {
    char char_time[120];
    sprintf(char_time, "cryptoTime: %f cloudTime: %f", cryptoTime, cloudTime);
    string time = char_time;
    return time;
}



