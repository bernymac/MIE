//
//  main.cpp
//  Server
//
//  Created by Bernardo Ferreira on 06/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "MIEServerMT.h"

using namespace std;
using namespace cv;

//needed for linker because they are static
map<int,Mat> MIEServerMT::imgFeatures;
map<int,vector<vector<unsigned char> > > MIEServerMT::textFeatures;
vector<map<int,int> > MIEServerMT::imgIndex;
map<vector<unsigned char>,map<int,int> > MIEServerMT::textIndex;
atomic<int> MIEServerMT::nImgs, MIEServerMT::nDocs;
BOWImgDescriptorExtractor* MIEServerMT::bowExtr;
mutex MIEServerMT::imgFeaturesLock, MIEServerMT::textFeaturesLock, MIEServerMT::imgIndexLock, MIEServerMT::textIndexLock;

MIEServerMT::MIEServerMT() {
//    startServer();
}

MIEServerMT::~MIEServerMT() {
    free(bowExtr);
}

void MIEServerMT::startServer() {
    imgIndex.resize(clusters);
    nImgs = 0;
    nDocs = 0;
    bowExtr = new BOWImgDescriptorExtractor (DescriptorMatcher::create("BruteForce-L1"));
    if ( access((homePath+"Data/Server/MIE/codebook.yml").c_str(), F_OK ) != -1 ) {
        FileStorage fs;
        Mat codebook;
        fs.open(homePath+"Data/Server/MIE/codebook.yml", FileStorage::READ);
        fs["codebook"] >> codebook;
        fs.release();
        bowExtr->setVocabulary(codebook);
        printf("Read Codebook!\n");
    }
    
    int sockfd = initServer();
    ThreadPool pool(sysconf(_SC_NPROCESSORS_ONLN));
    printf("mieMT server started!\n");
    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        pool.enqueue(clientThread, newsockfd);
    }
}

void MIEServerMT::clientThread(int newsockfd) {
    if (newsockfd < 0)
        pee("ERROR on accept");
    char buffer[1];
    bzero(buffer,1);
    if (read(newsockfd,buffer,1) < 0)
        pee("ERROR reading from socket");
    //        printf("received %c cmd",buffer[0]);
    switch (buffer[0]) {
        case 'a':
            addDoc(newsockfd);
            break;
        case 'i':
            //try to read index from disk
            if (!readIndex())
                //try to read features if not in memory, persist them otherwise
                if (readOrPersistFeatures()) {
                    indexImgs();
                    indexText();
                    persistIndex();
                }
            break;
        case 's':
            search(newsockfd);
            break;
        default:
            pee("unkonwn command!\n");
    }
    //ack(newsockfd);
    close(newsockfd);
}

void MIEServerMT::addDoc(int newsockfd) {
    int id = -1;
    Mat mat;
    vector<vector<unsigned char> > keywords;
    receiveDoc(newsockfd, &id, &mat, &keywords);
    imgFeaturesLock.lock();
    imgFeatures[id] = mat;
    imgFeaturesLock.unlock();
    textFeaturesLock.lock();
    textFeatures[id] = keywords;
    textFeaturesLock.unlock();
//    printf("received %d\n",id);
}

void MIEServerMT::receiveDoc(int newsockfd, int* id, Mat* mat, vector<vector<unsigned char> >* keywords) {
    //receive and unzip data
    char buff[2*sizeof(uint64_t)];
    socketReceive(newsockfd, buff, 2*sizeof(uint64_t));
    unsigned long zipSize, dataSize;
    memcpy(&zipSize, buff, sizeof(uint64_t));
    memcpy(&dataSize, buff + sizeof(uint64_t), sizeof(uint64_t));
    zipSize = be64toh(zipSize);
    dataSize = be64toh(dataSize);
    char* data = (char*)malloc(dataSize);
    receiveAndUnzip(newsockfd, data, &dataSize, zipSize);   //with zip
    
//    char data[5*sizeof(int)];
//    socketReceive(newsockfd, data, 5*sizeof(int));        //without zip
    
    int pos = 0;
    *id = readIntFromArr(data, &pos);
    int nFeatures = readIntFromArr(data, &pos);
    int featureSize = readIntFromArr(data, &pos);
    int nKeywords = readIntFromArr(data, &pos);
    int keywordSize = readIntFromArr(data, &pos);
    
//    int dataSize = nFeatures*featureSize*sizeof(int) + nKeywords*keywordSize*sizeof(unsigned char);
//    char* data2 = (char*)malloc(dataSize);
//    socketReceive(newsockfd, data2, dataSize);            //without zip
    
    //store img features
    mat->create(nFeatures, featureSize, CV_32F);
    for (int i = 0; i < nFeatures; i++)
        for (int j = 0; j < featureSize; j++)
            mat->at<float>(i,j) = float(readIntFromArr(data, &pos));
    //store text features
    keywords->resize(nKeywords);
    for (int i = 0; i < nKeywords; i++) {
        (*keywords)[i].resize(keywordSize);
        for (int j = 0; j < keywordSize; j++)
            readFromArr(&(*keywords)[i][j], sizeof(unsigned char), data, &pos);
    }
    free(data);
}

bool MIEServerMT::readOrPersistFeatures() {
    // read/persist images
    FILE* f = fopen((homePath+"Data/Server/MIE/imgFeatures").c_str(), "rb");
    imgFeaturesLock.lock();
    nImgs = (int)imgFeatures.size();
    imgFeaturesLock.unlock();
    if (nImgs == 0 && f != NULL) {
        char buff[3*sizeof(int)];
        fread (buff, 1, 3*sizeof(int), f);
        int nFeatures=0, featureSize=0, pos = 0;
        readFromArr(&nImgs, sizeof(int), buff, &pos);
        readFromArr(&nFeatures, sizeof(int), buff, &pos);
        readFromArr(&featureSize, sizeof(int), buff, &pos);
        size_t buffSize = featureSize*sizeof(float);
        char* buff2 = (char*)malloc(buffSize);
        bzero(buff2,buffSize);
        imgFeaturesLock.lock();
        for (int i = 0; i < nImgs; i++) {
            Mat aux2(nFeatures,featureSize,CV_32F);
            imgFeatures[i] = aux2;
            for (int j = 0; j < nFeatures; j++) {
                fread (buff2, 1, buffSize, f);
                pos = 0;
                for (int k = 0; k < featureSize; k++)
                    readFromArr(&imgFeatures[i].at<float>(j,k), sizeof(float), buff2, &pos);
            }

        }
        imgFeaturesLock.unlock();
        free(buff2);
    } else if (nImgs > 0) {
        if (f != NULL)
            fclose(f);
        f = fopen((homePath+"Data/Server/MIE/imgFeatures").c_str(), "wb");
//        int nImgs = (int)imgFeatures.size();
        imgFeaturesLock.lock();
        int nFeatures = imgFeatures[0].rows;
        int featureSize = imgFeatures[0].cols;
        imgFeaturesLock.unlock();
        char buff[3*sizeof(int)];
        int pos = 0;
        addToArr(&nImgs, sizeof(int), buff, &pos);
        addToArr(&nFeatures, sizeof(int), buff, &pos);
        addToArr(&featureSize, sizeof(int), buff, &pos);
        fwrite(buff, 1, 3*sizeof(int), f);
        size_t buffSize = featureSize*sizeof(float);
        char* buff2 = (char*)malloc(buffSize);
        bzero(buff2,buffSize);
        imgFeaturesLock.lock();
        for (int i = 0; i < nImgs; i++)
            for (int j = 0; j < nFeatures; j++) {
                pos = 0;
                for (int k = 0; k < featureSize; k++)
                    addToArr(&imgFeatures[i].at<float>(j,k), sizeof(float), buff2, &pos);
                fwrite(buff2, 1, buffSize, f);
            }
        imgFeaturesLock.unlock();
        free(buff2);
    }
    fclose(f);
    // read/persist text
    f = fopen((homePath+"Data/Server/MIE/textFeatures").c_str(), "rb");
    textFeaturesLock.lock();
    nDocs = (int)textFeatures.size();
    textFeaturesLock.unlock();
    if (nDocs == 0 && f != NULL) {
        int nKeywords=0, keywordSize=0, pos=0;
        char buff[3*sizeof(int)];
        fread(buff, 1, 3*sizeof(int), f);
        readFromArr(&nDocs, sizeof(int), buff, &pos);
        readFromArr(&keywordSize, sizeof(int), buff, &pos);
        readFromArr(&nKeywords, sizeof(int), buff, &pos);
        for (int i = 0; i < nDocs; i++) {
            vector<vector<unsigned char> > aux2;
            aux2.resize(nKeywords);
            textFeaturesLock.lock();
            textFeatures[i] = aux2;
            textFeaturesLock.unlock();
            size_t buffSize = nKeywords*keywordSize*sizeof(unsigned char) + sizeof(int);
            char* buff2 = (char*)malloc(buffSize);
            bzero(buff2,buffSize);
            fread(buff2, 1, buffSize, f);
            pos = 0;
            for (int j = 0; j < nKeywords; j++) {
                vector<unsigned char> aux3;
                aux3.resize(keywordSize);
                textFeaturesLock.lock();
                textFeatures[i][j] = aux3;
                for (int k = 0; k < keywordSize; k++)
                    readFromArr(&textFeatures[i][j][k], sizeof(unsigned char), buff2, &pos);
                textFeaturesLock.unlock();
            }
            readFromArr(&nKeywords, sizeof(int), buff2, &pos);
            free(buff2);
        }
    } else if (nDocs > 0) {
        textFeaturesLock.lock();
        int keywordSize = (int)textFeatures[0][0].size();
        textFeaturesLock.unlock();
        char buff[2*sizeof(int)];
        int pos = 0;
        addToArr(&nDocs, sizeof(int), buff, &pos);
        addToArr(&keywordSize, sizeof(int), buff, &pos);
        if (f != NULL)
            fclose(f);
        f = fopen((homePath+"Data/Server/MIE/textFeatures").c_str(), "wb");
        fwrite(buff, 1, 2*sizeof(int), f);
        textFeaturesLock.lock();
        for (int i = 0; i < nDocs; i++) {
            int nKeywords = (int)textFeatures[i].size();
            size_t buffSize = sizeof(int) + nKeywords*keywordSize*sizeof(unsigned char);
            char* buff2 = (char*)malloc(buffSize);
            bzero(buff2,buffSize);
            pos = 0;
            addToArr(&nKeywords, sizeof(int), buff2, &pos);
            for (int j = 0; j < nKeywords; j++)
                for (int k = 0; k < keywordSize; k++)
                    addToArr(&textFeatures[i][j][k], sizeof(unsigned char), buff2, &pos);
            fwrite(buff2, 1, buffSize, f);
            free(buff2);
        }
        textFeaturesLock.unlock();
    }
    fclose(f);
    if (nImgs > 0 && nDocs > 0)
        return true;    //return true if features existed or if they didnt exist but was able to read them from disk
    return false;
}


void MIEServerMT::indexImgs() {
    imgFeaturesLock.lock();
//    nImgs = (int)imgFeatures.size();
    int vocabSize = bowExtr->getVocabulary().rows;
    imgFeaturesLock.unlock();
    if (vocabSize == 0) {
        TermCriteria terminate_criterion;
        terminate_criterion.epsilon = FLT_EPSILON;
        BOWKMeansTrainer bowTrainer (clusters, terminate_criterion, 3, KMEANS_PP_CENTERS );
        RNG& rng = theRNG();
        imgFeaturesLock.lock();
        for (int i = 0; i < nImgs; i++)
            if (rng.uniform(0.f,1.f) <= 0.1f)
                bowTrainer.add(imgFeatures[i]);
        imgFeaturesLock.unlock();
        printf("build codebook with %d descriptors!\n",bowTrainer.descriptorsCount());
        Mat codebook = bowTrainer.cluster();
        FileStorage fs;
        fs.open(homePath+"Data/Server/MIE/codebook.yml", FileStorage::WRITE);
        fs << "codebook" << codebook;
        fs.release();
        imgFeaturesLock.lock();
        bowExtr->setVocabulary(codebook);
        imgFeaturesLock.unlock();
    }
    //index images
    imgFeaturesLock.lock();
    for (map<int,Mat>::iterator it=imgFeatures.begin(); it!=imgFeatures.end(); ++it) {
        Mat bowDesc;
        bowExtr->compute(it->second,bowDesc);
        for (int i = 0; i < clusters; i++) {
            int val = denormalize(bowDesc.at<float>(i),it->second.rows);
            if (val > 0) {
                imgIndexLock.lock();
                imgIndex[i][it->first] = val;
                imgIndexLock.unlock();
            }
        }
    }
    imgFeaturesLock.unlock();
}

void MIEServerMT::indexText() {
//    *nTextDocs = (int)textFeatures.size();
    textFeaturesLock.lock();
    textIndexLock.lock();
    for (map<int,vector<vector<unsigned char> > >::iterator it=textFeatures.begin(); it!=textFeatures.end(); ++it) {
        for (int i = 0; i < it->second.size(); i++) {
            map<vector<unsigned char>,map<int,int> >::iterator postingList = textIndex.find(it->second[i]);
            if (postingList == textIndex.end()) {
                map<int,int> newPostingList;
                newPostingList[it->first] = 1;
                textIndex[it->second[i]] = newPostingList;
            } else {
                map<int,int>::iterator posting = postingList->second.find(it->first);
                if (posting == postingList->second.end())
                    postingList->second[it->first] = 1;
                else
                    posting->second++;
            }
        }
    }
    textFeaturesLock.unlock();
    textIndexLock.unlock();
}

void MIEServerMT::persistImgIndex() {
    imgIndexLock.lock();
    FILE* f = fopen((homePath+"Data/Server/MIE/imgIndex").c_str(), "wb");
    int indexSize = (int)imgIndex.size();
    char buff[2*sizeof(int)];
    int pos = 0;
    addToArr(&nImgs, sizeof(int), buff, &pos);
    addToArr(&indexSize, sizeof(int), buff, &pos);
    fwrite(buff, 1, 2*sizeof(int), f);
    for (int i = 0; i < indexSize; i++) {
        int postingListSize = (int)imgIndex[i].size();
        size_t buffSize = sizeof(int) + postingListSize * 2 * sizeof(int);
        char* buff2 = (char*)malloc(buffSize);
        pos = 0;
        addToArr(&postingListSize, sizeof(int), buff2, &pos);
        for (map<int,int>::iterator it=imgIndex[i].begin(); it!=imgIndex[i].end(); ++it) {
            int first = it->first;
            addToArr(&first, sizeof(int), buff2, &pos);
            addToArr(&(it->second), sizeof(int), buff2, &pos);
        }
        fwrite(buff2, 1, buffSize, f);
        free(buff2);
    }
    fclose(f);
    imgIndexLock.unlock();
}

void MIEServerMT::persistTextIndex() {
    // read/persist text
    textIndexLock.lock();
    FILE* f = fopen((homePath+"Data/Server/MIE/textIndex").c_str(), "wb");
    int indexSize = (int)textIndex.size();
    int keywordSize = (int)textIndex.begin()->first.size();
    char buff[3*sizeof(int)];
    int pos = 0;
    addIntToArr(nDocs, buff, &pos);
    addIntToArr(indexSize, buff, &pos);
    addIntToArr(keywordSize, buff, &pos);
    fwrite(buff, 1, 3*sizeof(int), f);
    for (map<vector<unsigned char>,map<int,int> >::iterator it=textIndex.begin(); it!=textIndex.end(); ++it) {
        int postingListSize = (int)it->second.size();
        size_t buffSize = keywordSize * sizeof(unsigned char) + sizeof(int) + postingListSize*2*sizeof(int);
        char* buff2 = (char*)malloc(buffSize);
        pos = 0;
        addIntToArr(postingListSize, buff2, &pos);
        for (int i = 0; i < keywordSize; i++) {
            unsigned char val = it->first[i];
            addToArr(&val, sizeof(unsigned char), buff2, &pos);
        }
        for (map<int,int>::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2) {
            int first = it2->first;
            addIntToArr(first, buff2, &pos);
            addIntToArr(it2->second, buff2, &pos);
        }
        fwrite(buff2, 1, buffSize, f);
        free(buff2);
    }
    fclose(f);
    textIndexLock.unlock();
}

void MIEServerMT::persistIndex() {
    persistImgIndex();
    persistTextIndex();
}

bool MIEServerMT::readIndex() {
    FILE* f1 = fopen((homePath+"Data/Server/MIE/imgIndex").c_str(), "rb");
    FILE* f2 = fopen((homePath+"Data/Server/MIE/textIndex").c_str(), "rb");
    if (f1==NULL || f2==NULL)
        return false;
    if (f1 != NULL) {
        char buff[3*sizeof(int)];
        fread (buff, 1, 3*sizeof(int), f1);
        int indexSize=0, postingListSize=0, pos=0;
        readFromArr(&nImgs, sizeof(int), buff, &pos);
        readFromArr(&indexSize, sizeof(int), buff, &pos);
        readFromArr(&postingListSize, sizeof(int), buff, &pos);
        for (int i = 0; i < indexSize; i++) {
            map<int,int> aux;
            imgIndexLock.lock();
            imgIndex[i] = aux;
            imgIndexLock.unlock();
            size_t buffSize = postingListSize * 2 * sizeof(int) + sizeof(int);
            char* buff2 = (char*)malloc(buffSize);
            bzero(buff2, buffSize);
            fread (buff2, 1, buffSize, f1);
            pos = 0;
            for (int j = 0; j < postingListSize; j++) {
                int docId=0, score=0;
                readFromArr(&docId, sizeof(int), buff2, &pos);
                readFromArr(&score, sizeof(int), buff2, &pos);
                imgIndexLock.lock();
                imgIndex[i][docId] = score;
                imgIndexLock.unlock();
            }
            readFromArr(&postingListSize, sizeof(int), buff2, &pos);
            free(buff2);
        }
        fclose(f1);
    }
    if (f2 != NULL) {

        char buff[4*sizeof(int)];
        fread (buff, 1, 4*sizeof(int), f2);
        int indexSize=0, postingListSize=0, keywordSize=0, pos=0;
        readFromArr(&nDocs, sizeof(int), buff, &pos);
        readFromArr(&indexSize, sizeof(int), buff, &pos);
        readFromArr(&keywordSize, sizeof(int), buff, &pos);
        readFromArr(&postingListSize, sizeof(int), buff, &pos);
        for (int i = 0; i < indexSize; i++) {
            size_t buffSize = keywordSize * sizeof(unsigned char) + postingListSize*2*sizeof(int) + sizeof(int);
            char* buff2 = (char*)malloc(buffSize);
            bzero(buff2, buffSize);
            fread (buff2, 1, buffSize, f2);
            pos = 0;
            vector<unsigned char> key;
            key.resize(keywordSize);
            for (int j = 0; j < keywordSize; j++)
                readFromArr(&key[j], sizeof(unsigned char), buff2, &pos);
            map<int,int> aux;
            for (int j = 0; j < postingListSize; j++) {
                int docId=0, score=0;
                readFromArr(&docId, sizeof(int), buff2, &pos);
                readFromArr(&score, sizeof(int), buff2, &pos);
                aux[docId] = score;
            }
            textIndexLock.lock();
            textIndex[key] = aux;
            textIndexLock.unlock();
            readFromArr(&postingListSize, sizeof(int), buff2, &pos);
            free(buff2);
        }
        fclose(f2);

    }
    return true;
}

set<QueryResult,cmp_QueryResult> MIEServerMT::imgSearch (Mat* features) {
    map<int,double> queryResults;
    Mat bowDesc;
    bowExtr->compute(*features,bowDesc);
    for (int i = 0; i < clusters; i++) {
        int queryTf = denormalize(bowDesc.at<float>(i),features->rows);
        if (queryTf > 0) {
            imgIndexLock.lock();
            map<int,int> postingList = imgIndex[i];
            imgIndexLock.unlock();
            double idf = getIdf(nImgs, postingList.size());
            for (map<int,int>::iterator it=postingList.begin(); it!=postingList.end(); ++it) {
                double score = getTfIdf(queryTf, it->second, idf);
                if (queryResults.count(it->first) == 0)
                    queryResults[it->first] = score;
                else
                    queryResults[it->first] += score;
            }
        }
    }
    return sort(&queryResults);
}

set<QueryResult,cmp_QueryResult> MIEServerMT::textSearch(vector<vector<unsigned char> >* keywords) {
    map<vector<unsigned char>,int> query;
    for (int i = 0; i < keywords->size(); i++) {
        if (query.count((*keywords)[i]) == 0)
            query[(*keywords)[i]] = 1;
        else
            query[(*keywords)[i]]++;
    }
    map<int,double> queryResults;
    for (map<vector<unsigned char>,int>::iterator queryTerm = query.begin(); queryTerm != query.end(); ++queryTerm) {
        textIndexLock.lock();
        map<int,int> postingList = textIndex[queryTerm->first];
        textIndexLock.unlock();
        double idf = getIdf(nDocs, postingList.size());
        for (map<int,int>::iterator posting = postingList.begin(); posting != postingList.end(); ++posting) {
            //float score = bm25L(posting->second, queryTerm->second, idf, docLength, avgDocLength);
            double score =  getTfIdf(queryTerm->second, posting->second, idf);
            if (queryResults.count(posting->first) == 0)
                queryResults[posting->first] = score;
            else
                queryResults[posting->first] += score;
        }
    }
    return sort(&queryResults);
}

void MIEServerMT::search(int newsockfd) {
    int id;
    Mat mat;
    vector<vector<unsigned char> > keywords;
    receiveDoc(newsockfd, &id, &mat, &keywords);
    set<QueryResult,cmp_QueryResult> imgResults = imgSearch(&mat);
    set<QueryResult,cmp_QueryResult> textResults = textSearch(&keywords);
    set<QueryResult,cmp_QueryResult> mergedResults = mergeSearchResults(&imgResults, &textResults);
    sendQueryResponse(newsockfd, &mergedResults, 20);
}

