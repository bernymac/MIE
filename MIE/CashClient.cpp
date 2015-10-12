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
#define DOCS 1000

CashClient::CashClient() {
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
//    aesCrypto = new SSECrypt;
    
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

void CashClient::train() {
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
        for (unsigned i = 0; i < DOCS; i++) {
            if (rng.uniform(0.f,1.f) <= 0.1f) {
                bzero(fname, 120);
                sprintf(fname, "%s/wang/%d.jpg", datasetsPath, i);
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

void CashClient::addDocs(const char* imgDataset, const char* textDataset, bool firstUpdate, int first, int last, int prefix) {
    //create or retrieve index
//    vector< map<int,int> > imgIndex;
//    imgIndex.resize(CLUSTERS);
//    map<vector<unsigned char>,map<int,int> > textIndex;
    map<vector<unsigned char>,vector<unsigned char> > encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> > encTextIndex;
    int sockfd = -1;
    timespec start;
    
//    if (!firstUpdate) {
//        char buffer[1];
//        buffer[0] = 'd';
//        sockfd = connectAndSend(buffer, 1);
//        receivePostingLists(sockfd, &imgIndex, &textIndex);
//        socketReceiveAck(sockfd);
//    }
    //index imgs
    char* fname = (char*)malloc(120);
    if (fname == NULL) pee("malloc error in CashClient::addDocs fname");
    for (unsigned i=first; i<=last; i++) {
        bzero(fname, 120);
        sprintf(fname, "%s/%s/%d.jpg", datasetsPath, imgDataset, i);
        Mat image = imread(fname);
        vector<KeyPoint> keypoints;
        Mat bowDesc;
        detector->detect( image, keypoints );
        start = getTime();     //start index benchmark
        bowExtractor->compute( image, keypoints, bowDesc );
        for (int j=0; j<CLUSTERS; j++) {
            int val = denormalize(bowDesc.at<float>(j),(int)keypoints.size());
            if (val > 0) {
//                imgIndex[j][i+prefix] = val;
                indexTime += diffSec(start, getTime()); //end benchmark
                start = getTime();     //start crypto benchmark
                int c = (*imgDcount)[j];
                encryptAndIndex(&j, sizeof(int), c, i+prefix, val, &encImgIndex);
                (*imgDcount)[j] = ++c;
                cryptoTime += diffSec(start, getTime()); //end benchmark
                start = getTime();     //start index benchmark
            }
        }
        indexTime += diffSec(start, getTime()); //end benchmark
    }
    //index text
    for (unsigned i=first; i<=last; i++) {
        bzero(fname, 120);
        sprintf(fname, "%s/%s/tags%d.txt", datasetsPath, textDataset, i+1);
        vector<string> keywords = analyzer->extractFile(fname);
        start = getTime();     //start index benchmark
        map<string,int> textTfs;
        for (int j = 0; j < keywords.size(); j++) {
            string keyword = keywords[j];
            map<string,int>::iterator it = textTfs.find(keyword);
            if (it == textTfs.end())
                textTfs[keyword] = 1;
            else
                it->second++;
        }
        indexTime += diffSec(start, getTime());         //end benchmark
        start = getTime();     //start crypto benchmark
        for (map<string,int>::iterator it=textTfs.begin(); it!=textTfs.end(); ++it) {
//            vector<unsigned char> encKeyword = textCrypto->encode(keywords[j]);
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
    
/*    //encrypt img index
    vector< vector<unsigned char> > encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> > encTextIndex;
    timespec start = getTime();                     //start crypto benchmark
    encImgIndex.resize(imgIndex.size());
    for (int i = 0; i < imgIndex.size(); i++) {
        const int postingListSize = (int)imgIndex[i].size();
        if (postingListSize == 0) {
            vector<unsigned char> v;
            encImgIndex[i] = v;
        } else {
            int buffSize = postingListSize * sizeof(int) * 2;
            char* buff = (char*)malloc(buffSize);
            if (buff == NULL) pee("malloc error in CashClient::addDocs encImgIndex");
            bzero(buff,buffSize);
            int pos = 0;
            for (map<int,int>::iterator it=imgIndex[i].begin(); it!=imgIndex[i].end(); ++it) {
                int docId = it->first;
                addToArr(&docId, sizeof(int), buff, &pos);
                addToArr(&(it->second), sizeof(int), buff, &pos);
            }
            encImgIndex[i] = aesCrypto->encrypt((unsigned char*)buff, buffSize);
            free(buff);
        }
    }
    //encrypt text index
    for (map< vector<unsigned char>,map<int,int> >::iterator it1=textIndex.begin(); it1!=textIndex.end(); ++it1) {
        int buffSize = (int)it1->second.size() * sizeof(int) * 2;
        char* buff = (char*)malloc(buffSize);
        if (buff == NULL) pee("malloc error in CashClient::addDocs enTextIndex");
        bzero(buff,buffSize);
        int pos = 0;
        for (std::map<int,int>::iterator it2=it1->second.begin(); it2!=it1->second.end(); ++it2) {
            int docId = it2->first;
            addToArr(&docId, sizeof(int), buff, &pos);
            addToArr(&(it2->second), sizeof(int), buff, &pos);
        }
        encTextIndex[it1->first] = aesCrypto->encrypt((unsigned char*)buff, buffSize);
        free(buff);
    }
    cryptoTime += diffSec(start, getTime());            //end benchmark
*/
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



/*void CashClient::receivePostingLists(int sockfd, vector<map<int,int> >* imgPostingLists,
                                    map<vector<unsigned char>,map<int,int> >* textPostingLists) {
    timespec start = getTime();         //start cloud time benchmark
    char buff[3*sizeof(int)];
    if (read(sockfd,buff,3*sizeof(int)) < 0)
        pee("ERROR reading from socket");
    int pos = 0;
    int nImgPostingLists = readIntFromArr(buff, &pos);
    int nTextPostingLists = readIntFromArr(buff, &pos);
    int postingListSize = readIntFromArr(buff, &pos);
    for (int i = 0; i < nImgPostingLists; i++) {
        int buffSize = sizeof(int) + postingListSize*sizeof(unsigned char) + sizeof(int);
        char* buff2 = (char*) malloc(buffSize);
        if (buff2 == NULL) pee("malloc error in CashClient::calculateQueryResults");
        receiveAll(sockfd, buff2, buffSize);
        pos = 0;
        int vw = readIntFromArr(buff2, &pos);
        unsigned char* encPostingList = (unsigned char*)malloc(postingListSize);
        for (int j = 0; j < postingListSize; j++)
            readFromArr(&encPostingList[j], sizeof(unsigned char), buff2, &pos);
        cloudTime += diffSec(start, getTime());         //end benchmark
        start = getTime();                //start crypto time benchmark
        (*imgPostingLists)[vw] = decryptPostingList(encPostingList, postingListSize);
        cryptoTime += diffSec(start, getTime());        //end benchmark
        start = getTime();                 //start cloud time benchmark
        postingListSize = readIntFromArr(buff2, &pos);
        free(encPostingList);
        free(buff2);
    }
    for (int i = 0; i < nTextPostingLists; i++) {
        int buffSize = TextCrypt::keysize*sizeof(unsigned char) + postingListSize*sizeof(unsigned char);
        if (i < nTextPostingLists-1)
            buffSize += sizeof(int);
        char* buff2 = (char*) malloc(buffSize);
        if (buff2 == NULL) pee("malloc error in CashClient::calculateQueryResults");
        receiveAll(sockfd, buff2, buffSize);
        pos = 0;
        vector<unsigned char> keyword;
        keyword.resize(TextCrypt::keysize);
        for (int j = 0; j < TextCrypt::keysize; j++)
            readFromArr(&keyword[j], sizeof(unsigned char), buff2, &pos);
        unsigned char* encPostingList = (unsigned char*)malloc(postingListSize);
        for (int j = 0; j < postingListSize; j++)
            readFromArr(&encPostingList[j], sizeof(unsigned char), buff2, &pos);
        cloudTime += diffSec(start, getTime());         //end benchmark
        start = getTime();                //start crypto time benchmark
        (*textPostingLists)[keyword] = decryptPostingList(encPostingList, postingListSize);
        cryptoTime += diffSec(start, getTime());        //end benchmark
        start = getTime();                 //start cloud time benchmark
        if (i < nTextPostingLists-1)
            postingListSize = readIntFromArr(buff2, &pos);
        free(encPostingList);
        free(buff2);
    }
    cloudTime += diffSec(start, getTime()); //end benchmark
}
*/

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
        printf("Found an unexpected collision in CashClient::encryptAndIndex");
    (*index)[encCounter] = encData;
    

//    if (keywordSize > 4) {
//        LOGI("vw 0 k1: %s\n",getHexRepresentation(k1,CashCrypt::Ksize).c_str());
//        LOGI("vw 0 k2: %s\n",getHexRepresentation(k2,CashCrypt::Ksize).c_str());
//        LOGI("vw 0 counter 0 EncCounter: %s\n",getHexRepresentation(encCounter.data(),CashCrypt::Ksize).c_str());
//        LOGI("vw 0 counter 0 EncData: %s\n",getHexRepresentation(encData.data(),16).c_str());
//    }
}


vector<QueryResult> CashClient::search(int id) {
    //process img object
    map<int,int> vws;
    char* fname = (char*)malloc(120);
    sprintf(fname, "%s/wang/%d.jpg", datasetsPath, id);
    Mat image = imread(fname);
    vector<KeyPoint> keypoints;
    Mat bowDesc;
    detector->detect( image, keypoints );
    timespec start = getTime();              //start index time benchmark
    bowExtractor->compute( image, keypoints, bowDesc );
    for (unsigned i=0; i<CLUSTERS; i++) {
        const int queryTf = denormalize(bowDesc.at<float>(i),(int)keypoints.size());
        if (queryTf > 0) {
            vws[i] = queryTf;
        }
    }
    indexTime += diffSec(start, getTime());   //end benchmark
    //process text object
    map<string,int> kws;
    bzero(fname, 120);
    sprintf(fname, "%s/docs/tags%d.txt", datasetsPath, id+1);
    vector<string> keywords = analyzer->extractFile(fname);
    start = getTime();                               //start index time benchmark
    for (int j = 0; j < keywords.size(); j++) {
//        start = getTime();                  //start crypto time benchmark
//        vector<unsigned char> encKeyword = textCrypto->encode(keywords[j]);
//        cryptoTime += diffSec(start, getTime()); //end benchmark
        map<string,int>::iterator queryTf = kws.find(keywords[j]);
        if (queryTf == kws.end())
            kws[keywords[j]] = 1;
        else
            queryTf->second++;
    }
    indexTime += diffSec(start, getTime());          //end benchmark
    free(fname);
    
    //mandar para a cloud
    start = getTime();                      //start cloud time benchmark
    long buffSize = 1 + 3*sizeof(int) + vws.size()*(2*CashCrypt::Ksize+sizeof(int)) + kws.size()*(2*CashCrypt::Ksize+sizeof(int));
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in CashClient::search sendCloud");
    int pos = 0;
    buff[pos++] = 's';       //cmd
    addIntToArr ((int)vws.size(), buff, &pos);
    addIntToArr ((int)kws.size(), buff, &pos);
    addIntToArr (CashCrypt::Ksize, buff, &pos);
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
    start = getTime();                      //start cloud time benchmark
    int sockfd = connectAndSend(buff, buffSize);
    //    const int x = (int)encKeywords.size()*TextCrypt::keysize+2*sizeof(int);
    //    LOGI("Text Search network traffic part 1: %d\n",x);
    
    free(buff);
    vector<QueryResult> queryResults = receiveQueryResults(sockfd);
    cloudTime += diffSec(start, getTime());            //end benchmark
    socketReceiveAck(sockfd);
    return queryResults;
    //receive posting lists and calculate query results
//    return calculateQueryResults(sockfd);, &vws, &encKeywords);
}

/*
set<QueryResult,cmp_QueryResult> CashClient::calculateQueryResults(int sockfd, map<int,int>* vws,
                                                                  map<vector<unsigned char>,int>* encKeywords) {
    //receive posting lists
    //map<int, map<int,int> > imgPostingLists;
    vector< map<int,int> > imgPostingLists;
    imgPostingLists.resize(CLUSTERS);
    map<vector<unsigned char>, map<int,int> > textPostingLists;
    receivePostingLists(sockfd, &imgPostingLists, &textPostingLists);
    socketReceiveAck(sockfd);
    
    //calculate img query results
    timespec start = getTime();                    //start index time benchmark
    map<int,float>* imgQueryResults = new map<int,float>;
    //    for (map<int,map<int,int> >::iterator it=imgPostingLists.begin(); it!=imgPostingLists.end(); ++it) {
    for (int i = 0; i < imgPostingLists.size(); i++) {
        //        float idf = getIdf(DOCS, it->size());
        if (imgPostingLists[i].size() > 0) {
            const float idf = getIdf(DOCS, imgPostingLists[i].size());
            //          for (map<int,int>::iterator it2 = it->begin(); it2!=it->end(); ++it2) {
            for (map<int,int>::iterator it2 = imgPostingLists[i].begin(); it2!=imgPostingLists[i].end(); ++it2) {
                //              const int queryTf = (*vws)[it->first];
                const int queryTf = (*vws)[i];
                const float score = queryTf * getTfIdf(it2->second, idf);
                if (imgQueryResults->count(it2->first) == 0)
                    (*imgQueryResults)[it2->first] = score;
                else
                    (*imgQueryResults)[it2->first] += score;
            }
        }
    }
    set<QueryResult,cmp_QueryResult> sortedImgResults = sort(imgQueryResults);
    free(imgQueryResults);
    
    //calculate text query results
    map<int,float>* textQueryResults = new map<int,float>;
    for (map<vector<unsigned char>,map<int,int> >::iterator it=textPostingLists.begin(); it!=textPostingLists.end(); ++it) {
        float idf = getIdf(DOCS, it->second.size());
        for (map<int,int>::iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
            const int queryTf = (*encKeywords)[it->first];
            const float score = queryTf * getTfIdf(it2->second, idf);
            if (textQueryResults->count(it2->first) == 0)
                (*textQueryResults)[it2->first] = score;
            else
                (*textQueryResults)[it2->first] += score;
        }
    }
    set<QueryResult,cmp_QueryResult> sortedTextResults = sort(textQueryResults);
    free(textQueryResults);
    
    //merge and return
    set<QueryResult,cmp_QueryResult> mergedResults = mergeSearchResults(&sortedImgResults, &sortedTextResults);
    indexTime += diffSec(start, getTime());            //end benchmark
    return mergedResults;
}
*/

/*map<int,int> CashClient::decryptPostingList(unsigned char* encPostingList, int ciphertextSize) {
    unsigned char* rawPostingList = (unsigned char*)malloc(ciphertextSize);
    const int plaintextSize = aesCrypto->decrypt(encPostingList, ciphertextSize, rawPostingList);
    map<int,int> postingList;
    const int postingListSize = plaintextSize / (2*sizeof(int));
    int pos = 0;
    for (int i = 0; i < postingListSize; i++) {
        int docId, tf;
        readFromArr(&docId, sizeof(int), (char*)rawPostingList, &pos);
        readFromArr(&tf, sizeof(int), (char*)rawPostingList, &pos);
        postingList[docId] = tf;
    }
    free(rawPostingList);
    return postingList;
}*/

/*set<QueryResult,cmp_QueryResult> CashClient::mergeSearchResults(set<QueryResult,cmp_QueryResult>* imgResults,
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
    return sort(&queryResults);
}*/


string CashClient::printTime() {
    char char_time[120];
    sprintf(char_time, "cryptoTime:%f trainTime:%f indexTime:%f cloudTime:%f", cryptoTime, trainTime, indexTime, cloudTime);
    string time = char_time;
    return time;
}





