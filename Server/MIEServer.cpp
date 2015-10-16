//
//  main.cpp
//  Server
//
//  Created by Bernardo Ferreira on 06/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "MIEServer.h"

using namespace std;
using namespace cv;


MIEServer::MIEServer() {
    startServer();
}

void MIEServer::startServer() {
    map<int,Mat> imgFeatures;
    map<int,vector<vector<unsigned char> > > textFeatures;
    vector<map<int,int> > imgIndex;
    imgIndex.resize(clusters);
    map<vector<unsigned char>,map<int,int> > textIndex;
    int nImgs = 0, nTextDocs = 0;
    
    BOWImgDescriptorExtractor bowExtr (DescriptorMatcher::create("BruteForce-L1"));
    if ( access((dataPath+"/codebook.yml").c_str(), F_OK ) != -1 ) {
        FileStorage fs;
        Mat codebook;
        fs.open(dataPath+"/codebook.yml", FileStorage::READ);
        fs["codebook"] >> codebook;
        fs.release();
        bowExtr.setVocabulary(codebook);
        printf("Read Codebook!\n");
    }
    
    int sockfd = initServer();
    printf("MIE server started!\n");
    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            pee("ERROR on accept");
        char buffer[1];
        bzero(buffer,1);
        if (read(newsockfd,buffer,1) < 0)
            pee("ERROR reading from socket");
        //        printf("received %c cmd",buffer[0]);
        switch (buffer[0]) {
            case 'a':
                addDoc(newsockfd,&imgFeatures,&textFeatures);
                break;
            case 'i':
                //try to read index from disk
                if (!readIndex(dataPath, &imgIndex, &textIndex, &nImgs, &nTextDocs))
                    //try to read features if not in memory, persist them otherwise
                    if (readOrPersistFeatures(dataPath, &imgFeatures, &textFeatures)) {
                        indexImgs(&imgFeatures, &imgIndex, &bowExtr, &nImgs);
                        indexText(&textFeatures,&textIndex, &nTextDocs);
                        persistIndex(dataPath, &imgIndex, &textIndex, nImgs, nTextDocs);
                    }
                break;
            case 's':
                search(newsockfd, &bowExtr, &imgIndex, &nImgs, &textIndex, &nTextDocs);
                break;
            default:
                pee("unkonwn command!\n");
        }
        //ack(newsockfd);
        close(newsockfd);
    }
}

void MIEServer::addDoc(int newsockfd, map<int,Mat>* imgFeatures,
            map<int,vector<vector<unsigned char> > >* textFeatures) {
    int id = -1;
    Mat mat;
    vector<vector<unsigned char> > keywords;
    receiveDoc(newsockfd, &id, &mat, &keywords);
    (*imgFeatures)[id] = mat;
    (*textFeatures)[id] = keywords;
//    printf("received %d\n",id);
}

void MIEServer::receiveDoc(int newsockfd, int* id, Mat* mat, vector<vector<unsigned char> >* keywords) {
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

bool MIEServer::readOrPersistFeatures(string dataPath, map<int,Mat>* imgFeatures,
                           map<int,vector<vector<unsigned char> > >* textFeatures) {
    // read/persist images
    FILE* f = fopen((dataPath+"/imgFeatures").c_str(), "rb");
    if (imgFeatures->size() == 0 && f != NULL) {
        char buff[3*sizeof(int)];
        fread (buff, 1, 3*sizeof(int), f);
        int nImgs=0, nFeatures=0, featureSize=0, pos = 0;
        readFromArr(&nImgs, sizeof(int), buff, &pos);
        readFromArr(&nFeatures, sizeof(int), buff, &pos);
        readFromArr(&featureSize, sizeof(int), buff, &pos);
        size_t buffSize = featureSize*sizeof(float);
        char* buff2 = (char*)malloc(buffSize);
        bzero(buff2,buffSize);
        for (int i = 0; i < nImgs; i++) {
            Mat aux2(nFeatures,featureSize,CV_32F);
            (*imgFeatures)[i] = aux2;
            for (int j = 0; j < nFeatures; j++) {
                fread (buff2, 1, buffSize, f);
                pos = 0;
                for (int k = 0; k < featureSize; k++)
                    readFromArr(&(*imgFeatures)[i].at<float>(j,k), sizeof(float), buff2, &pos);
            }
        }
        free(buff2);
    } else if (imgFeatures->size() > 0) {
        if (f != NULL)
            fclose(f);
        f = fopen((dataPath+"/imgFeatures").c_str(), "wb");
        int nImgs = (int)imgFeatures->size();
        int nFeatures = (*imgFeatures)[0].rows;
        int featureSize = (*imgFeatures)[0].cols;
        char buff[3*sizeof(int)];
        int pos = 0;
        addToArr(&nImgs, sizeof(int), buff, &pos);
        addToArr(&nFeatures, sizeof(int), buff, &pos);
        addToArr(&featureSize, sizeof(int), buff, &pos);
        fwrite(buff, 1, 3*sizeof(int), f);
        size_t buffSize = featureSize*sizeof(float);
        char* buff2 = (char*)malloc(buffSize);
        bzero(buff2,buffSize);
        for (int i = 0; i < nImgs; i++)
            for (int j = 0; j < nFeatures; j++) {
                pos = 0;
                for (int k = 0; k < featureSize; k++)
                    addToArr(&(*imgFeatures)[i].at<float>(j,k), sizeof(float), buff2, &pos);
                fwrite(buff2, 1, buffSize, f);
            }
        free(buff2);
    }
    fclose(f);
    // read/persist text
    f = fopen((dataPath+"/textFeatures").c_str(), "rb");
    if (textFeatures->size() == 0 && f != NULL) {
        int nDocs=0, nKeywords=0, keywordSize=0, pos=0;
        char buff[3*sizeof(int)];
        fread(buff, 1, 3*sizeof(int), f);
        readFromArr(&nDocs, sizeof(int), buff, &pos);
        readFromArr(&keywordSize, sizeof(int), buff, &pos);
        readFromArr(&nKeywords, sizeof(int), buff, &pos);
        for (int i = 0; i < nDocs; i++) {
            vector<vector<unsigned char> > aux2;
            aux2.resize(nKeywords);
            (*textFeatures)[i] = aux2;
            size_t buffSize = nKeywords*keywordSize*sizeof(unsigned char) + sizeof(int);
            char* buff2 = (char*)malloc(buffSize);
            bzero(buff2,buffSize);
            fread(buff2, 1, buffSize, f);
            pos = 0;
            for (int j = 0; j < nKeywords; j++) {
                vector<unsigned char> aux3;
                aux3.resize(keywordSize);
                (*textFeatures)[i][j] = aux3;
                for (int k = 0; k < keywordSize; k++)
                    readFromArr(&(*textFeatures)[i][j][k], sizeof(unsigned char), buff2, &pos);
            }
            readFromArr(&nKeywords, sizeof(int), buff2, &pos);
            free(buff2);
        }
    } else if (textFeatures->size() > 0) {
        int nDocs = (int)textFeatures->size();
        int keywordSize = (int)(*textFeatures)[0][0].size();
        char buff[2*sizeof(int)];
        int pos = 0;
        addToArr(&nDocs, sizeof(int), buff, &pos);
        addToArr(&keywordSize, sizeof(int), buff, &pos);
        if (f != NULL)
            fclose(f);
        f = fopen((dataPath+"/textFeatures").c_str(), "wb");
        fwrite(buff, 1, 2*sizeof(int), f);
        for (int i = 0; i < nDocs; i++) {
            int nKeywords = (int)(*textFeatures)[i].size();
            size_t buffSize = sizeof(int) + nKeywords*keywordSize*sizeof(unsigned char);
            char* buff2 = (char*)malloc(buffSize);
            bzero(buff2,buffSize);
            pos = 0;
            addToArr(&nKeywords, sizeof(int), buff2, &pos);
            for (int j = 0; j < nKeywords; j++)
                for (int k = 0; k < keywordSize; k++)
                    addToArr(&(*textFeatures)[i][j][k], sizeof(unsigned char), buff2, &pos);
            fwrite(buff2, 1, buffSize, f);
            free(buff2);
        }
    }
    fclose(f);
    return imgFeatures->size() > 0 && textFeatures->size() > 0 ? true : false;
}


void MIEServer::indexImgs(map<int,Mat>* imgFeatures, vector<map<int,int> >* imgIndex,
               BOWImgDescriptorExtractor* bowExtr, int* nImgs) {
    *nImgs = (int)imgFeatures->size();
    //try to read codebook from disk
    if (bowExtr->getVocabulary().rows == 0) {
        TermCriteria terminate_criterion;
        terminate_criterion.epsilon = FLT_EPSILON;
        BOWKMeansTrainer bowTrainer (clusters, terminate_criterion, 3, KMEANS_PP_CENTERS );
        RNG& rng = theRNG();
        for (int i = 0; i < imgFeatures->size(); i++)
            if (rng.uniform(0.f,1.f) <= 0.1f) {
                bowTrainer.add((*imgFeatures)[i]);
            }
        printf("build codebook with %d descriptors!\n",bowTrainer.descriptorsCount());
        Mat codebook = bowTrainer.cluster();
        FileStorage fs;
        fs.open(dataPath+"/codebook.yml", FileStorage::WRITE);
        fs << "codebook" << codebook;
        fs.release();
        bowExtr->setVocabulary(codebook);
    }
    //index images
    for (map<int,Mat>::iterator it=imgFeatures->begin(); it!=imgFeatures->end(); ++it) {
        Mat bowDesc;
        bowExtr->compute(it->second,bowDesc);
        for (int i = 0; i < clusters; i++) {
            int val = denormalize(bowDesc.at<float>(i),it->second.rows);
            if (val > 0)
                (*imgIndex)[i][it->first] = val;
        }
    }
}

void MIEServer::indexText(map<int,vector<vector<unsigned char> > >* textFeatures,
               map<vector<unsigned char>,map<int,int> >* textIndex, int* nTextDocs) {
    *nTextDocs = (int)textFeatures->size();
    for (map<int,vector<vector<unsigned char> > >::iterator it=textFeatures->begin(); it!=textFeatures->end(); ++it) {
        for (int i = 0; i < it->second.size(); i++) {
            map<vector<unsigned char>,map<int,int> >::iterator postingList = textIndex->find(it->second[i]);
            if (postingList == textIndex->end()) {
                map<int,int> newPostingList;
                newPostingList[it->first] = 1;
                (*textIndex)[it->second[i]] = newPostingList;
            } else {
                map<int,int>::iterator posting = postingList->second.find(it->first);
                if (posting == postingList->second.end())
                    postingList->second[it->first] = 1;
                else
                    posting->second++;
            }
        }
    }
}

void MIEServer::persistImgIndex(string dataPath, vector<map<int,int> >* imgIndex, int nImgs) {
    FILE* f = fopen((dataPath+"/imgIndex").c_str(), "wb");
    int indexSize = (int)imgIndex->size();
    char buff[2*sizeof(int)];
    int pos = 0;
    addToArr(&nImgs, sizeof(int), buff, &pos);
    addToArr(&indexSize, sizeof(int), buff, &pos);
    fwrite(buff, 1, 2*sizeof(int), f);
    for (int i = 0; i < indexSize; i++) {
        int postingListSize = (int)(*imgIndex)[i].size();
        size_t buffSize = sizeof(int) + postingListSize * 2 * sizeof(int);
        char* buff2 = (char*)malloc(buffSize);
        pos = 0;
        addToArr(&postingListSize, sizeof(int), buff2, &pos);
        for (map<int,int>::iterator it=(*imgIndex)[i].begin(); it!=(*imgIndex)[i].end(); ++it) {
            int first = it->first;
            addToArr(&first, sizeof(int), buff2, &pos);
            addToArr(&(it->second), sizeof(int), buff2, &pos);
        }
        fwrite(buff2, 1, buffSize, f);
        free(buff2);
    }
    fclose(f);
}

void MIEServer::persistTextIndex(string dataPath, map<vector<unsigned char>,map<int,int> >* textIndex, int nTextDocs) {
    // read/persist text
    FILE* f = fopen((dataPath+"/textIndex").c_str(), "wb");
    int indexSize = (int)textIndex->size();
    int keywordSize = (int)textIndex->begin()->first.size();
    char buff[3*sizeof(int)];
    int pos = 0;
    addToArr(&nTextDocs, sizeof(int), buff, &pos);
    addToArr(&indexSize, sizeof(int), buff, &pos);
    addToArr(&keywordSize, sizeof(int), buff, &pos);
    fwrite(buff, 1, 3*sizeof(int), f);
    for (map<vector<unsigned char>,map<int,int> >::iterator it=textIndex->begin(); it!=textIndex->end(); ++it) {
        int postingListSize = (int)it->second.size();
        size_t buffSize = keywordSize * sizeof(unsigned char) + sizeof(int) + postingListSize*2*sizeof(int);
        char* buff2 = (char*)malloc(buffSize);
        pos = 0;
        addToArr(&postingListSize, sizeof(int), buff2, &pos);
        for (int i = 0; i < keywordSize; i++) {
            unsigned char val = it->first[i];
            addToArr(&val, sizeof(unsigned char), buff2, &pos);
        }
        for (map<int,int>::iterator it2=it->second.begin(); it2!=it->second.end(); ++it2) {
            int first = it2->first;
            addToArr(&first, sizeof(int), buff2, &pos);
            addToArr(&(it2->second), sizeof(int), buff2, &pos);
        }
        fwrite(buff2, 1, buffSize, f);
        free(buff2);
    }
    fclose(f);
}

void MIEServer::persistIndex(string dataPath, vector<map<int,int> >* imgIndex,
                  map<vector<unsigned char>,map<int,int> >* textIndex, int nImgs, int nTextDocs) {
    persistImgIndex(dataPath, imgIndex, nImgs);
    persistTextIndex(dataPath, textIndex, nTextDocs);
}

bool MIEServer::readIndex(string dataPath, vector<map<int,int> >* imgIndex,
                  map<vector<unsigned char>,map<int,int> >* textIndex, int* nImgs, int* nTextDocs) {
    FILE* f1 = fopen((dataPath+"/imgIndex").c_str(), "rb");
    FILE* f2 = fopen((dataPath+"/textIndex").c_str(), "rb");
    if (f1==NULL || f2==NULL)
        return false;
    if (f1 != NULL) {
        char buff[3*sizeof(int)];
        fread (buff, 1, 3*sizeof(int), f1);
        int indexSize=0, postingListSize=0, pos=0;
        readFromArr(nImgs, sizeof(int), buff, &pos);
        readFromArr(&indexSize, sizeof(int), buff, &pos);
        readFromArr(&postingListSize, sizeof(int), buff, &pos);
        for (int i = 0; i < indexSize; i++) {
            map<int,int> aux;
            (*imgIndex)[i] = aux;
            size_t buffSize = postingListSize * 2 * sizeof(int) + sizeof(int);
            char* buff2 = (char*)malloc(buffSize);
            bzero(buff2, buffSize);
            fread (buff2, 1, buffSize, f1);
            pos = 0;
            for (int j = 0; j < postingListSize; j++) {
                int docId=0, score=0;
                readFromArr(&docId, sizeof(int), buff2, &pos);
                readFromArr(&score, sizeof(int), buff2, &pos);
                (*imgIndex)[i][docId] = score;
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
        readFromArr(nTextDocs, sizeof(int), buff, &pos);
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
            (*textIndex)[key] = aux;
            readFromArr(&postingListSize, sizeof(int), buff2, &pos);
            free(buff2);
        }
        fclose(f2);
    }
    return true;
}

set<QueryResult,cmp_QueryResult> MIEServer::imgSearch (Mat* features, BOWImgDescriptorExtractor* bowExtr,
                                                 vector<map<int,int> >* imgIndex, int* nImgs) {
    map<int,float> queryResults;
    Mat bowDesc;
    bowExtr->compute(*features,bowDesc);
    for (int i = 0; i < clusters; i++) {
        int queryTf = denormalize(bowDesc.at<float>(i),features->rows);
        if (queryTf > 0) {
            map<int,int> postingList = (*imgIndex)[i];
            float idf = getIdf(*nImgs, postingList.size());
            for (map<int,int>::iterator it=postingList.begin(); it!=postingList.end(); ++it) {
                float score = getTfIdf(queryTf, it->second, idf);
                if (queryResults.count(it->first) == 0)
                    queryResults[it->first] = score;
                else
                    queryResults[it->first] += score;
            }
        }
    }
    return sort(queryResults);
}

set<QueryResult,cmp_QueryResult> MIEServer::textSearch(vector<vector<unsigned char> >* keywords,
                                                 map<vector<unsigned char>,map<int,int> >* textIndex, int* nTextDocs) {
    map<vector<unsigned char>,int> query;
    for (int i = 0; i < keywords->size(); i++) {
        if (query.count((*keywords)[i]) == 0)
            query[(*keywords)[i]] = 1;
        else
            query[(*keywords)[i]]++;
    }
    map<int,float> queryResults;
    for (map<vector<unsigned char>,int>::iterator queryTerm = query.begin(); queryTerm != query.end(); ++queryTerm) {
        map<int,int> postingList = (*textIndex)[queryTerm->first];
        float idf = getIdf(*nTextDocs, postingList.size());
        for (map<int,int>::iterator posting = postingList.begin(); posting != postingList.end(); ++posting) {
            //float score = bm25L(posting->second, queryTerm->second, idf, docLength, avgDocLength);
            float score =  getTfIdf(queryTerm->second, posting->second, idf);
            if (queryResults.count(posting->first) == 0)
                queryResults[posting->first] = score;
            else
                queryResults[posting->first] += score;
        }
    }
    return sort(queryResults);
}

/*set<QueryResult,cmp_QueryResult> MIEServer::mergeSearchResults(set<QueryResult,cmp_QueryResult>* imgResults,
                                                         set<QueryResult,cmp_QueryResult>* textResults) {
    const float sigma = 0.01f;
    //prepare ranks
    map<int,Rank> ranks;
    if (textResults != NULL) {
        int i = 1;
        for (set<QueryResult,cmp_QueryResult>::iterator it=textResults->begin(); it!=textResults->end(); ++it) {
            struct Rank qr;
            qr.textRank = i++;
            qr.imgRank = 0;
            ranks[it->docId] = qr;
        }
    }
    if (imgResults != NULL) {
        int i = 1;
        for (set<QueryResult,cmp_QueryResult>::iterator it=imgResults->begin(); it!=imgResults->end(); ++it) {
            map<int,Rank>::iterator rank = ranks.find(it->docId);
            if (rank == ranks.end()) {
                struct Rank qr;
                qr.textRank = 0;
                qr.imgRank = i++;
                ranks[it->docId] = qr;
            } else
                rank->second.imgRank = i++;
        }
    }
    map<int,float> queryResults;
    for (map<int,Rank>::iterator it=ranks.begin(); it!=ranks.end(); ++it) {
        float score = 0.f, df = 0.f;
        if (it->second.textRank > 0) {
            score =  1 / pow(it->second.textRank,2);
            df++;
        }
        if (it->second.imgRank > 0) {
            score +=  1 / pow(it->second.imgRank,2);
            df++;
        }
        score *= log(df+sigma);
        queryResults[it->first] = score;
    }
    return sort(queryResults);
}*/

/*void MIEServer::sendQueryResponse(int newsockfd, set<QueryResult,cmp_QueryResult>* mergedResults) {
    int resultsSize = (int)mergedResults->size();
    long size = sizeof(int) + resultsSize * (sizeof(int) + sizeof(uint64_t));
    char* buff = (char*)malloc(size);
    bzero(buff,size);
    int pos = 0;
    addIntToArr (resultsSize, buff, &pos);
    for (set<QueryResult,cmp_QueryResult>::iterator it = mergedResults->begin(); it != mergedResults->end(); ++it) {
        int docId = it->docId;
        float score = it->score;
        addIntToArr(docId, buff, &pos);
        addFloatToArr(score, buff, &pos);
    }
    socketSend (newsockfd, buff, size);
    printf("Search Network Traffic part 2: %ld\n",size);
    free(buff);
}*/

void MIEServer::search(int newsockfd, BOWImgDescriptorExtractor* bowExtr, vector<map<int,int> >* imgIndex,
            int* nImgs, map<vector<unsigned char>,map<int,int> >* textIndex, int* nTextDocs) {
    int id;
    Mat mat;
    vector<vector<unsigned char> > keywords;
    receiveDoc(newsockfd, &id, &mat, &keywords);
    set<QueryResult,cmp_QueryResult> imgResults = imgSearch(&mat,bowExtr,imgIndex,nImgs);
    set<QueryResult,cmp_QueryResult> textResults = textSearch(&keywords,textIndex, nTextDocs);
    set<QueryResult,cmp_QueryResult> mergedResults = mergeSearchResults(&imgResults, &textResults);
    sendQueryResponse(newsockfd, &mergedResults);
}

