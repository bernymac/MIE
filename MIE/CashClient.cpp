//
//  CashClient.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 25/09/15.
//  Copyright © 2015 NovaSYS. All rights reserved.
//
//  Implementation of the scheme by Cash et al. NDSS 2014
//  without random oracles and extended to multimodal ranked searching
//

#include "CashClient.hpp"


#define CLUSTERS 1000

CashClient::CashClient() {
    featureTime = 0;
    cryptoTime = 0;
    cloudTime = 0;
    indexTime = 0;
    trainTime = 0;
    detector = FeatureDetector::create( "PyramidDense" );
    extractor = DescriptorExtractor::create( "SURF" );
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    bowExtractor = new BOWImgDescriptorExtractor( extractor, matcher );
    analyzer = new EnglishAnalyzer;
    crypto = new CashCrypt;
    
    imgDcount = new vector<int>(CLUSTERS,0);
    textDcount = new map<string,int>;
}

CashClient::~CashClient() {
    detector.release();
    extractor.release();
    bowExtractor.release();
    free(imgDcount);
    free(textDcount);
}

void CashClient::train(const char* dataset, int first, int last) {
    string s = dataPath;
    s += "/Cash/dictionary.yml";
    if ( access(s.c_str(), F_OK ) != -1 ) {
        FileStorage fs;
        Mat codebook;
        fs.open(s, FileStorage::READ);
        fs["codebook"] >> codebook;
        fs.release();
        bowExtractor->setVocabulary(codebook);
        LOGI("Read Codebook!\n");
    } else {
        timespec start = getTime();                         //getTime
        TermCriteria terminate_criterion;
        terminate_criterion.epsilon = FLT_EPSILON;
        BOWKMeansTrainer bowTrainer ( CLUSTERS, terminate_criterion, 3, KMEANS_PP_CENTERS );
        RNG& rng = theRNG();
        char* fname = (char*)malloc(120);
        if (fname == NULL) pee("malloc error in CashClient::train()");
        for (unsigned i = first; i < last; i++) {
            if (rng.uniform(0.f,1.f) <= 0.1f) {
                bzero(fname, 120);
                sprintf(fname, "%s/%s/%imd.jpg", datasetsPath, dataset, i);
                Mat image = imread(fname);
                vector<KeyPoint> keypoints;
                Mat descriptors;
                detector->detect(image, keypoints);
                extractor->compute(image, keypoints, descriptors);
                bowTrainer.add(descriptors);
            }
        }
        free(fname);
        LOGI("build codebook with %d descriptors!\n",bowTrainer.descripotorsCount());
        Mat codebook = bowTrainer.cluster();
        bowExtractor->setVocabulary(codebook);
        trainTime += diffSec(start, getTime());         //getTime
        LOGI("Training Time: %f\n",trainTime);
        FileStorage fs;
        fs.open(s, FileStorage::WRITE);
        fs << "codebook" << codebook;
        fs.release();
    }
}

void CashClient::addDocs(const char* imgDataset, const char* textDataset, int first, int last, int prefix) {
    map<vector<unsigned char>,vector<unsigned char> > encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> > encTextIndex;
    int sockfd = -1;
    timespec start;
    
    //index imgs
    char* fname = (char*)malloc(120);
    if (fname == NULL) pee("malloc error in CashClient::addDocs fname");
    for (unsigned i=first; i<=last; i++) {
        start = getTime();                          //start feature extraction benchmark
        bzero(fname, 120);
        sprintf(fname, "%s/%s/im%d.jpg", datasetsPath, imgDataset, i);
        Mat image = imread(fname);
        vector<KeyPoint> keypoints;
        Mat bowDesc;
        detector->detect( image, keypoints );
        featureTime += diffSec(start, getTime());   //end benchmark
        start = getTime();                          //start index benchmark
        bowExtractor->compute( image, keypoints, bowDesc );
        indexTime += diffSec(start, getTime());     //end benchmark
        start = getTime();                          //start crypto benchmark
        for (int j=0; j<CLUSTERS; j++) {
            int val = denormalize(bowDesc.at<float>(j),(int)keypoints.size());
            if (val > 0) {
                int c = (*imgDcount)[j];
                encryptAndIndex(&j, sizeof(int), c, i+prefix, val, &encImgIndex);
                (*imgDcount)[j] = ++c;
            }
        }
        cryptoTime += diffSec(start, getTime());    //end benchmark
    }
    //index text
    for (unsigned i=first; i<=last; i++) {
        start = getTime();                          //start feature extraction benchmark
        bzero(fname, 120);
        sprintf(fname, "%s/%s/tags%d.txt", datasetsPath, textDataset, i);
        vector<string> keywords = analyzer->extractFile(fname);
        featureTime += diffSec(start, getTime());   //end benchmark
        start = getTime();                          //start index benchmark
        map<string,int> textTfs;
        for (int j = 0; j < keywords.size(); j++) {
            string keyword = keywords[j];
            map<string,int>::iterator it = textTfs.find(keyword);
            if (it == textTfs.end())
                textTfs[keyword] = 1;
            else
                it->second++;
        }
        indexTime += diffSec(start, getTime());     //end benchmark
        start = getTime();                          //start crypto benchmark
        for (map<string,int>::iterator it=textTfs.begin(); it!=textTfs.end(); ++it) {
            int c = 0;
            map<string,int>::iterator counterIt = textDcount->find(it->first);
            if (counterIt != textDcount->end())
                c = counterIt->second;
            encryptAndIndex((void*)it->first.c_str(), (int)it->first.size(), c, i+prefix, it->second, &encTextIndex);
            (*textDcount)[it->first] = ++c;
        }
        cryptoTime += diffSec(start, getTime()); //end benchmark
        
//            start = getTime();               //start index benchmark
//            map<int,int>* postingList = &textIndex[encKeyword];
//            map<int,int>::iterator posting = postingList->find(i+prefix);
//            if (posting == postingList->end())
//                (*postingList)[i+prefix] = 1;
//            else
//                posting->second++;
//            indexTime += diffSec(start, getTime());         //end benchmark
        
    }
    free(fname);
    
    //mandar para a cloud
    start = getTime();                          //start cloud benchmark
    long buffSize = 4*sizeof(int);
    for (map<vector<unsigned char>,vector<unsigned char> >::iterator it=encImgIndex.begin(); it!=encImgIndex.end(); ++it)
        buffSize += CashCrypt::Ksize*sizeof(unsigned char) + sizeof(int) + it->second.size()*sizeof(unsigned char);
    for (map<vector<unsigned char>,vector<unsigned char> >::iterator it=encTextIndex.begin(); it!=encTextIndex.end(); ++it)
        buffSize += CashCrypt::Ksize*sizeof(unsigned char) + sizeof(int) + it->second.size()*sizeof(unsigned char);
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in CashClient::addDocs sendCloud");
    int pos = 0;
    addIntToArr (last-first+1, buff, &pos); //send N of docs (should be sending the enc docs actually)
    addIntToArr ((int)encImgIndex.size(), buff, &pos);
    addIntToArr ((int)encTextIndex.size(), buff, &pos);
    addIntToArr (CashCrypt::Ksize, buff, &pos);
    for (map<vector<unsigned char>,vector<unsigned char> >::iterator it=encImgIndex.begin(); it!=encImgIndex.end(); ++it) {
        addIntToArr ((int)it->second.size(), buff, &pos);
        for (int i = 0; i < it->first.size(); i++) {
            unsigned char x = it->first[i];
            addToArr(&x, sizeof(unsigned char), buff, &pos);
        }
        for (int i = 0; i < it->second.size(); i++)
            addToArr(&(it->second[i]), sizeof(unsigned char), buff, &pos);
    }
    for (map<vector<unsigned char>,vector<unsigned char> >::iterator it=encTextIndex.begin(); it!=encTextIndex.end(); ++it) {
        addIntToArr ((int)it->second.size(), buff, &pos);
        for (int i = 0; i < it->first.size(); i++) {
            unsigned char x = it->first[i];
            addToArr(&x, sizeof(unsigned char), buff, &pos);
        }
        for (int i = 0; i < it->second.size(); i++)
            addToArr(&(it->second[i]), sizeof(unsigned char), buff, &pos);
    }
    char buffer[1];
    buffer[0] = 'a';
    sockfd = connectAndSend(buffer, 1);
    socketSend(sockfd, buff, buffSize);
    free(buff);
    cloudTime += diffSec(start, getTime());                 //end benchmark
    
    socketReceiveAck(sockfd);
}


void CashClient::encryptAndIndex(void* keyword, int keywordSize, int counter, int docId, int tf,
                                 map<vector<unsigned char>, vector<unsigned char> >* index) {
    unsigned char k1[CashCrypt::Ksize];
    int append = 1;
    crypto->deriveKey(&append, sizeof(int), keyword, keywordSize, k1);
    unsigned char k2[CashCrypt::Ksize];
    append = 2;
    crypto->deriveKey(&append, sizeof(int), keyword, keywordSize, k2);
    vector<unsigned char> encCounter = crypto->encCounter(k1, (unsigned char*)&counter, sizeof(int));
    unsigned char idAndScore[2*sizeof(int)];
    int pos = 0;
    addIntToArr(docId, (char*)idAndScore, &pos);
    addIntToArr(tf, (char*)idAndScore, &pos);
    vector<unsigned char> encData = crypto->encData(k2, idAndScore, pos);
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


vector<QueryResult> CashClient::search(const char* imgDataset, const char* textDataset, int id) {
    //process img object
    timespec start = getTime();                     //start feature extraction benchmark
    map<int,int> vws;
    char* fname = (char*)malloc(120);
    sprintf(fname, "%s/%s/im%d.jpg", datasetsPath, imgDataset, id);
    Mat image = imread(fname);
    vector<KeyPoint> keypoints;
    Mat bowDesc;
    detector->detect( image, keypoints );
    featureTime += diffSec(start, getTime());       //end benchmark
    start = getTime();                              //start index time benchmark
    bowExtractor->compute( image, keypoints, bowDesc );
    for (unsigned i=0; i<CLUSTERS; i++) {
        const int queryTf = denormalize(bowDesc.at<float>(i),(int)keypoints.size());
        if (queryTf > 0) {
            vws[i] = queryTf;
        }
    }
    indexTime += diffSec(start, getTime());         //end benchmark
    //process text object
    start = getTime();                              //start feature extraction benchmark
    map<string,int> kws;
    bzero(fname, 120);
    sprintf(fname, "%s/%s/tags%d.txt", datasetsPath, textDataset, id);
    vector<string> keywords = analyzer->extractFile(fname);
    featureTime += diffSec(start, getTime());       //end benchmark
    start = getTime();                              //start index time benchmark
    for (int j = 0; j < keywords.size(); j++) {
        map<string,int>::iterator queryTf = kws.find(keywords[j]);
        if (queryTf == kws.end())
            kws[keywords[j]] = 1;
        else
            queryTf->second++;
    }
    indexTime += diffSec(start, getTime());         //end benchmark
    free(fname);
    
    //mandar para a cloud
    start = getTime();                              //start cloud time benchmark
    long buffSize = 1 + 3*sizeof(int) + vws.size()*(2*CashCrypt::Ksize+sizeof(int)) + kws.size()*(2*CashCrypt::Ksize+sizeof(int));
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in CashClient::search sendCloud");
    int pos = 0;
    buff[pos++] = 's';       //cmd
    addIntToArr ((int)vws.size(), buff, &pos);
    addIntToArr ((int)kws.size(), buff, &pos);
    addIntToArr (CashCrypt::Ksize, buff, &pos);
    cloudTime += diffSec(start, getTime());         //end benchmark
    start = getTime();                              //start crypto time benchmark
    for (map<int,int>::iterator it=vws.begin(); it!=vws.end(); ++it) {
        int vw = it->first;
        int append = 1;
        unsigned char k1[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), &vw, sizeof(int), k1);
        addToArr(k1, CashCrypt::Ksize * sizeof(unsigned char), buff, &pos);
        append = 2;
        unsigned char k2[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), &vw, sizeof(int), k2);
        addToArr(k2, CashCrypt::Ksize * sizeof(unsigned char), buff, &pos);
        addIntToArr (it->second, buff, &pos);
//        if (vw == 0) {
//            LOGI("vw 0 k1: %s\n",getHexRepresentation(k1,CashCrypt::Ksize).c_str());
//            LOGI("vw 0 k2: %s\n",getHexRepresentation(k2,CashCrypt::Ksize).c_str());
//        }
    }
    for (map<string,int>::iterator it=kws.begin(); it!=kws.end(); ++it) {
        string kw = it->first;
        int append = 1;
        unsigned char k1[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), (void*)kw.c_str(), (int)kw.size(), k1);
        addToArr(k1, CashCrypt::Ksize * sizeof(unsigned char), buff, &pos);
        append = 2;
        unsigned char k2[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), (void*)kw.c_str(), (int)kw.size(), k2);
        addToArr(k2, CashCrypt::Ksize * sizeof(unsigned char), buff, &pos);
        addIntToArr (it->second, buff, &pos);
    }
    cryptoTime += diffSec(start, getTime());        //end benchmark
    start = getTime();                              //start cloud time benchmark
    int sockfd = connectAndSend(buff, buffSize);
    //    const int x = (int)encKeywords.size()*TextCrypt::keysize+2*sizeof(int);
    //    LOGI("Text Search network traffic part 1: %d\n",x);
    
    free(buff);
    vector<QueryResult> queryResults = receiveQueryResults(sockfd);
    cloudTime += diffSec(start, getTime());            //end benchmark
    
    socketReceiveAck(sockfd);
    return queryResults;
}

string CashClient::printTime() {
    char char_time[120];
    sprintf(char_time, "featureTime:%f cryptoTime:%f trainTime:%f indexTime:%f cloudTime:%f",
            featureTime, cryptoTime, trainTime, indexTime, cloudTime);
    string time = char_time;
    return time;
}

