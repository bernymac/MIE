//
//  MIEClient.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 27/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "MIEClient.h"

MIEClient::MIEClient() {
    featureTime = 0;
    indexTime = 0;
    cryptoTime = 0;
    cloudTime = 0;
    detector = FeatureDetector::create( /*"Dense"*/ "PyramidDense" );
    extractor = DescriptorExtractor::create( "SURF" );
    analyzer = new EnglishAnalyzer;
    sbe = new SBE(extractor->descriptorSize());
    textCrypto = new TextCrypt;
//    if (pthread_mutex_init(&lock, NULL) != 0) pee("mutex init failed\n");
}

MIEClient::~MIEClient() {
    detector.release();
    extractor.release();
    free(analyzer);
    free(sbe);
    free(textCrypto);
//    pthread_mutex_destroy(&lock);
}

void MIEClient::addDocs(const char* imgDataset, const char* textDataset, int first, int last, int prefix) {
    for (int id = first; id <= last; id++) {
        vector< vector<float> > imgFeatures;
        vector< vector<unsigned char> > encKeywords;
        processDoc(id, imgDataset, textDataset, &imgFeatures, &encKeywords);
        
        timespec start = getTime();
        int sockfd = sendDoc('a', id+prefix, &imgFeatures, &encKeywords);
        close(sockfd);
        cloudTime += diffSec(start, getTime());
//        socketReceiveAck(sockfd);
        
//        pthread_t t;
//        struct sendThreadData data;
//        data.op = 'a';
//        data.id = id+prefix;
//        data.imgFeatures = &imgFeatures;
//        data.textFeatures = &encKeywords;
//        data.cloudTime = &cloudTime;
//        data.lock = &lock;
//        if (pthread_create(&t, NULL, sendThread, (void*)&data))
//            pee("Error: unable to create sendThread");
    }
}

void MIEClient::processDoc(int id, const char* imgDataset, const char* textDataset, vector< vector<float> >* features, vector<vector<unsigned char> >* encKeywords) {
    //extract img features
    timespec start = getTime();                     //start feature benchmark
    char* fname = (char*)malloc(120);
    if (fname == NULL) pee("malloc error in MIEClient::processDoc");
    bzero(fname, 120);
    sprintf(fname, "%s/%s/im%d.jpg", datasetsPath, imgDataset, id);
    Mat image = imread(fname);
    vector<KeyPoint> keypoints;
    Mat descriptors;
    detector->detect(image, keypoints);
    featureTime += diffSec(start, getTime());       //end benchmark
    start = getTime();                              //start index benchmark
    extractor->compute(image, keypoints, descriptors);
    indexTime += diffSec(start, getTime());         //end benchmark
    
    //extract text features
    start = getTime();                              //start feature benchmark
    bzero(fname, 120);
    sprintf(fname, "%s/%s/tags%d.txt", datasetsPath, textDataset, id);
    vector<string> keywords = analyzer->extractFile(fname);
    featureTime += diffSec(start, getTime());       //end benchmark
    free(fname);
    
    //encrypt img features
    start = getTime();                              //start crypto benchmark
    features->resize(descriptors.rows);
//    for (int i = 0; i < descriptors.rows; i++) {          //single threaded
//        vector<float> feature;
//        feature.resize(descriptors.cols);
//        for (int j = 0; j < descriptors.cols; j++)
//            feature[j] = descriptors.at<float>(i, j);
//        (*features)[i] = sbe->encode(feature);
//    }
    const long numCPU = sysconf(_SC_NPROCESSORS_ONLN);    //hand made threads equal to cpus
    const int descPerCPU = ceil((float)descriptors.rows/(float)numCPU);
    pthread_t sbeThreads[numCPU];
    struct sbeThreadData sbeThreadsData[numCPU];
    for (int i = 0; i < numCPU; i++) {
        struct sbeThreadData data;
        data.first = i * descPerCPU;
        const int last = i*descPerCPU + descPerCPU;
        if (last < descriptors.rows)
            data.last = last;
        else
            data.last = descriptors.rows;
        data.descriptors = &descriptors;
        data.features = features;
        data.sbe = sbe;
        sbeThreadsData[i] = data;
        if (pthread_create(&sbeThreads[i], NULL, sbeEncryptionThread, (void*)&sbeThreadsData[i]))
            pee("Error: unable to create sbeEncryptionThread");
    }
    
    //encrypt text features
    encKeywords->resize(keywords.size());
    for (int i = 0; i < keywords.size(); i++)
        (*encKeywords)[i] = textCrypto->encode(keywords[i]);
    //wait for sbe threads
    for (int i = 0; i < numCPU; i++)
        if (pthread_join (sbeThreads[i], NULL)) pee("Error:unable to join thread");

    cryptoTime += diffSec(start, getTime());        //end crypto benchmark
}

void* MIEClient::sbeEncryptionThread(void* threadData) {
    struct sbeThreadData* data = (struct sbeThreadData*) threadData;
    for (int i = data->first; i < data->last; i++ ) {
        vector<float> feature;
        feature.resize(data->descriptors->cols);
        for (int j = 0; j < data->descriptors->cols; j++)
            feature[j] = data->descriptors->at<float>(i, j);
        (*data->features)[i] = data->sbe->encode(feature);
    }
    pthread_exit(NULL);
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
    for (int i = 0; i < features->size(); i++) {
        if ((*features)[i].size() != (*features)[0].size())         //debug check
            pee("Unexpected error; feature of uniform size");
        for (int j = 0; j < (*features)[i].size(); j++)
            addIntToArr ((int)(*features)[i][j], buff, &pos);
    }
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
//    socketSend(sockfd, buff, size);   //send without zip
    zipAndSend(sockfd, buff, size);     //send zipped
    
//    LOGI("Search network traffic part 1: %ld\n",size);
    free(buff);
    return sockfd;
}

//not being used
void* MIEClient::sendThread(void* threadData) {
    timespec start = getTime();     //start cloud benchmark
    struct sendThreadData* data = (struct sendThreadData*) threadData;
    vector< vector<float> >* features = data->imgFeatures;
    vector< vector<unsigned char> >* encKeywords = data->textFeatures;
    
    long size = 5*sizeof(int) + sizeof(int)*features->size()*(*features)[0].size() + encKeywords->size()*TextCrypt::keysize;
    char* buff = (char*)malloc(size);
    if (buff == NULL) pee("malloc error in MIEClient::sendDoc");
    int pos = 0;
    addIntToArr (data->id, buff, &pos);
    addIntToArr (int(features->size()), buff, &pos);
    addIntToArr (int((*features)[0].size()), buff, &pos);
    addIntToArr (int(encKeywords->size()), buff, &pos);
    addIntToArr (TextCrypt::keysize, buff, &pos);
    for (int i = 0; i < features->size(); i++)
        for (int j = 0; j < (*features)[i].size(); j++)
            addIntToArr ((int)(*features)[i][j], buff, &pos);
    for (int i = 0; i < encKeywords->size(); i++) {
        for (int j = 0; j < (*encKeywords)[i].size(); j++) {
            unsigned char c = (*encKeywords)[i][j];
            addToArr(&c, sizeof(unsigned char), buff, &pos);
        }
    }
    //upload
    //    pthread_mutex_lock(data->lock);
    char cmd[1];
    cmd[0] = data->op;
    int sockfd = connectAndSend(cmd, 1);
    zipAndSend(sockfd, buff, size);
    //    int sockfd = connectAndSend (buff, size) ;
    //    LOGI("Search network traffic part 1: %ld\n",size);
    free(buff);
    close(sockfd);
    timespec end = getTime();     //end benchmark
    *(data->cloudTime) += diffSec(start, end);
    //    pthread_mutex_unlock(data->lock);
    
    pthread_exit(NULL);
}


void MIEClient::index() {
    char buff[1];
    buff[0] = 'i';
    int sockfd = connectAndSend(buff, 1);
//    socketReceiveAck(sockfd);
    close(sockfd);
}

vector<QueryResult> MIEClient::search(int id, const char* imgDataset, const char* textDataset) {
    vector< vector<float> > features;
    vector< vector<unsigned char> > encKeywords;
    processDoc(id, imgDataset, textDataset, &features, &encKeywords);
    
    timespec start = getTime();     //start cloud time benchmark
    int sockfd = sendDoc('s', id, &features, &encKeywords);
    vector<QueryResult> queryResults = receiveQueryResults(sockfd);
    cloudTime += diffSec(start, getTime()); //end
    
//    socketReceiveAck(sockfd);
    close(sockfd);
    return queryResults;
}

string MIEClient::printTime() {
    char char_time[120];
    sprintf(char_time, "featureTime: %f indexTime: %f cryptoTime: %f cloudTime: %f", featureTime, indexTime, cryptoTime, cloudTime);
    string time = char_time;
    return time;
}
