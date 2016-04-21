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




CashCrypt* CashClient::crypto;
pthread_mutex_t* CashClient::lock;

CashClient::CashClient() {
    featureTime = 0;
    cryptoTime = 0;
    cloudTime = 0;
    indexTime = 0;
    trainTime = 0;
    detector = 
    //detector = xfeatures2d::SurfFeatureDetector::create();
    FeatureDetector::create( /*"Dense"*/ /*"PyramidDense"*/ "SURF" );
    //extractor = xfeatures2d::SurfDescriptorExtractor::create();
    DescriptorExtractor::create( "SURF" );
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    bowExtractor = new BOWImgDescriptorExtractor( extractor, matcher );
    analyzer = new EnglishAnalyzer;
    crypto = new CashCrypt;
    lock = new pthread_mutex_t;
    pthread_mutex_init(lock, NULL);
//    imgDcount = new vector<int>(CLUSTERS,0);
    memset(imgDcount, 0, CLUSTERS);
    textDcount = new map<string,int>;
}

CashClient::~CashClient() {
    detector.release();
    extractor.release();
    bowExtractor.release();
    delete textDcount;
    delete lock;
}

void CashClient::train(const char* dataset, int first, int last) {
    string s = homePath;
    s += "Data/Client/Cash/dictionary.yml";
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
                sprintf(fname, "%sDatasets/%s/im%d.jpg", homePath, dataset, i);
//                string fname = homePath; fname += "Datasets/"; fname += dataset; fname += "/im"; fname += to_string(i); fname += ".jpg";
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

/*void CashClient::addDocsMT(const char* imgDataset, const char* textDataset, int first, int last, int prefix) {
    map<vector<unsigned char>,vector<unsigned char> > encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> > encTextIndex;
    int sockfd = -1;
    timespec start;
    
    //index imgs
    char* fname = (char*)malloc(120);
    if (fname == NULL) pee("malloc error in CashClient::addDocs fname");
    for (unsigned i=first; i<=last; i++) {
        vector<int> imgCounters;
        vector<string> textCounters;
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
            if (val > 0)
                imgCounters.push_back(j);
        }
        cryptoTime += diffSec(start, getTime());    //end benchmark

        //index text
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
    
    //    socketReceiveAck(sockfd);
    close(sockfd);
}*/

void CashClient::addDocs(const char* imgDataset, const char* textDataset, int first, int last, int prefix) {
    map<vector<unsigned char>,vector<unsigned char> > encImgIndex;
    map<vector<unsigned char>,vector<unsigned char> > encTextIndex;
    int sockfd = -1;
    timespec start;
    char* fname = (char*)malloc(120);
    if (fname == NULL) pee("malloc error in CashClient::addDocs fname");
    for (unsigned id=first; id<=last; id++) {
        //extract img features
        start = getTime();                          //start feature extraction benchmark
        bzero(fname, 120);
        sprintf(fname, "%sDatasets/%s/im%d.jpg", homePath, imgDataset, id);
//        string fname = homePath; fname += "Datasets/"; fname += imgDataset; fname += "/im"; fname += itoa(id); fname += ".jpg";
        Mat image = imread(fname);
        vector<KeyPoint> keypoints;
        Mat bowDesc;
        detector->detect( image, keypoints );
        featureTime += diffSec(start, getTime());   //end benchmark
        start = getTime();                          //start index benchmark
        bowExtractor->compute( image, keypoints, bowDesc );
        indexTime += diffSec(start, getTime());     //end benchmark
        
        //extract text features
        start = getTime();                          //start feature extraction benchmark
        bzero(fname, 120);
        sprintf(fname, "%sDatasets/%s/tags%d.txt", homePath, textDataset, id);
//        s = homePath; s += "Datasets/"; s += textDataset; s += "/tags"; s += to_string(id); s += ".txt";
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
        
        //index img features
        start = getTime();                          //start crypto benchmark
        const long numCPU = sysconf(_SC_NPROCESSORS_ONLN)-1;    //hand made threads equal to cpus
        const int descPerCPU = ceil((float)CLUSTERS/(float)numCPU);
        pthread_t encThreads[numCPU];
        struct encThreadData encThreadsData[numCPU];
        for (int i = 0; i < numCPU; i++) {
            struct encThreadData data;
            data.obj = this;
            data.first = i * descPerCPU;
            const int last = i*descPerCPU + descPerCPU;
            if (last < CLUSTERS)
                data.last = last;
            else
                data.last = CLUSTERS;
            data.index = &encImgIndex;
            data.bowDesc = &bowDesc;
            data.nDescriptors = (int)keypoints.size();
            data.docId = id+prefix;
            encThreadsData[i] = data;
            if (pthread_create(&encThreads[i], NULL, encryptAndIndexImgsThread, (void*)&encThreadsData[i]))
                pee("Error: unable to create sbeEncryptionThread");
        }
/* //Single Thread
        for (int j=0; j<CLUSTERS; j++) {
            int val = denormalize(bowDesc.at<float>(j),(int)keypoints.size());
            if (val > 0) {
                int c = (*imgDcount)[j];
                encryptAndIndex(&j, sizeof(int), c, i+prefix, val, &encImgIndex);
                (*imgDcount)[j] = ++c;
            }
        }
*/
        //index text features
        for (map<string,int>::iterator it=textTfs.begin(); it!=textTfs.end(); ++it) {
            int c = 0;
            map<string,int>::iterator counterIt = textDcount->find(it->first);
            if (counterIt != textDcount->end())
                c = counterIt->second;
            encryptAndIndex((void*)it->first.c_str(), (int)it->first.size(), c, id+prefix, it->second, &encTextIndex);
            (*textDcount)[it->first] = ++c;
        }
        for (int i = 0; i < numCPU; i++)
            if (pthread_join (encThreads[i], NULL)) pee("Error:unable to join thread");
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
    
    //send to cloud
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
    
//    socketReceiveAck(sockfd);
    close(sockfd);
}

void* CashClient::encryptAndIndexImgsThread(void* threadData) {
    struct encThreadData* data = (struct encThreadData*) threadData;
    for (int j=data->first; j<data->last; j++) {
        int val = denormalize(data->bowDesc->at<float>(j),data->nDescriptors);
        if (val > 0) {
            int c = data->obj->imgDcount[j];
            data->obj->encryptAndIndex(&j, sizeof(int), c, data->docId, val, data->index);
            data->obj->imgDcount[j] = ++c;
        }
    }
    pthread_exit(NULL);
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
    pthread_mutex_lock (lock);
    if (index->find(encCounter) != index->end())
        pee("Found an unexpected collision in CashClient::encryptAndIndex");
    (*index)[encCounter] = encData;
    pthread_mutex_unlock (lock);

//    if (keywordSize > 4) {
//        LOGI("vw 0 k1: %s\n",getHexRepresentation(k1,CashCrypt::Ksize).c_str());
//        LOGI("vw 0 k2: %s\n",getHexRepresentation(k2,CashCrypt::Ksize).c_str());
//        LOGI("vw 0 counter 0 EncCounter: %s\n",getHexRepresentation(encCounter.data(),CashCrypt::Ksize).c_str());
//        LOGI("vw 0 counter 0 EncData: %s\n",getHexRepresentation(encData.data(),16).c_str());
//    }
}


vector<QueryResult> CashClient::search(const char* imgDataset, const char* textDataset, int id, bool randomOracle) {
    //process img object
    timespec start = getTime();                     //start feature extraction benchmark
    map<int,int> vws;
    char* fname = (char*)malloc(120);
    sprintf(fname, "%sDatasets/%s/im%d.jpg", homePath, imgDataset, id);
//    string s = homePath; s += "Datasets/"; s += imgDataset; s += "/im"; s += to_string(id); s += ".jpg";
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
    sprintf(fname, "%sDatasets/%s/tags%d.txt", homePath, textDataset, id);
//    s = homePath; s += "Datasets/"; s += textDataset; s += "/tags"; s += to_string(id); s += ".txt";
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
    if (randomOracle)
        return queryRO(&vws, &kws);
    else
        return queryStd(&vws, &kws);
}

    
vector<QueryResult> CashClient::queryRO(map<int,int>* vws, map<string,int>* kws) {
    timespec start = getTime();                              //start cloud time benchmark
    long buffSize = 1 + 3*sizeof(int) + vws->size()*(2*CashCrypt::Ksize+sizeof(int)) + kws->size()*(2*CashCrypt::Ksize+sizeof(int));
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in CashClient::search sendCloud");
    int pos = 0;
    buff[pos++] = 's';       //cmd
    addIntToArr ((int)vws->size(), buff, &pos);
    addIntToArr ((int)kws->size(), buff, &pos);
    addIntToArr (CashCrypt::Ksize, buff, &pos);
    cloudTime += diffSec(start, getTime());         //end benchmark
    start = getTime();                              //start crypto time benchmark
    for (map<int,int>::iterator it=vws->begin(); it!=vws->end(); ++it) {
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
    for (map<string,int>::iterator it=kws->begin(); it!=kws->end(); ++it) {
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
    vector<QueryResult> queryResults = this->receiveResults(sockfd);
    cloudTime += diffSec(start, getTime());            //end benchmark
    close(sockfd);
    return queryResults;
}

vector<QueryResult> CashClient::queryStd(map<int,int>* vws, map<string,int>* kws) {
    timespec start = getTime();                              //start cloud time benchmark
    long buffSize = 1 + 3*sizeof(int);
    for (map<int,int>::iterator it=vws->begin(); it!=vws->end(); ++it) {
        const int counter = imgDcount[it->first];
        buffSize += sizeof(int) + counter*CashCrypt::Ksize + CashCrypt::Ksize + sizeof(int);
    }
    for (map<string,int>::iterator it=kws->begin(); it!=kws->end(); ++it) {
        const int counter = (*textDcount)[it->first];
        buffSize += sizeof(int) + counter*CashCrypt::Ksize + CashCrypt::Ksize + sizeof(int);
    }
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in CashClient::search sendCloud");
    int pos = 0;
    buff[pos++] = 's';       //cmd
    addIntToArr ((int)vws->size(), buff, &pos);
    addIntToArr ((int)kws->size(), buff, &pos);
    addIntToArr (CashCrypt::Ksize, buff, &pos);
    cloudTime += diffSec(start, getTime());            //end benchmark
    start = getTime();                              //start crypto time benchmark
    for (map<int,int>::iterator it=vws->begin(); it!=vws->end(); ++it) {
        int vw = it->first;
        int counter = imgDcount[vw];
        addIntToArr(counter, buff, &pos);
        int append = 2;
        unsigned char k2[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), &vw, sizeof(int), k2);
        addToArr(k2, CashCrypt::Ksize * sizeof(unsigned char), buff, &pos);
        addIntToArr (it->second, buff, &pos);
        append = 1;
        unsigned char k1[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), &vw, sizeof(int), k1);
        for (int c = 0; c < counter; c++) {
            vector<unsigned char> encCounter = crypto->encCounter(k1, (unsigned char*)&c, sizeof(int));
            for (int j = 0; j < encCounter.size(); j++)
                addToArr(&encCounter[j], sizeof(unsigned char), buff, &pos);
        }
    }
    for (map<string,int>::iterator it=kws->begin(); it!=kws->end(); ++it) {
        string kw = it->first;
        int counter = (*textDcount)[kw];
        addIntToArr(counter, buff, &pos);
        int append = 2;
        unsigned char k2[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), (void*)kw.c_str(), (int)kw.size(), k2);
        addToArr(k2, CashCrypt::Ksize * sizeof(unsigned char), buff, &pos);
        addIntToArr (it->second, buff, &pos);
        append = 1;
        unsigned char k1[CashCrypt::Ksize];
        crypto->deriveKey(&append, sizeof(int), (void*)kw.c_str(), (int)kw.size(), k1);
        for (int c = 0; c < counter; c++) {
            vector<unsigned char> encCounter = crypto->encCounter(k1, (unsigned char*)&c, sizeof(int));
            for (int j = 0; j < encCounter.size(); j++)
                addToArr(&encCounter[j], sizeof(unsigned char), buff, &pos);
        }
    }
//    const int x = (int)encKeywords.size()*TextCrypt::keysize+2*sizeof(int);
//    LOGI("Text Search network traffic part 1: %d\n",x);
    cryptoTime += diffSec(start, getTime());        //end benchmark
    start = getTime();                              //start cloud time benchmark
    int sockfd = connectAndSend(buff, buffSize);
    free(buff);
    vector<QueryResult> queryResults = this->receiveResults(sockfd);
    close(sockfd);
    cloudTime += diffSec(start, getTime());            //end benchmark
    return queryResults;
}

vector<QueryResult> CashClient::receiveResults(int sockfd) {
    return receiveQueryResults(sockfd);
}


string CashClient::printTime() {
    char char_time[120];
    sprintf(char_time, "featureTime:%f cryptoTime:%f trainTime:%f indexTime:%f cloudTime:%f",
            featureTime, cryptoTime, trainTime, indexTime, cloudTime);
    string time = char_time;
    return time;
}

void CashClient::cleanTime() {
    featureTime = 0;
    cryptoTime = 0;
    cloudTime = 0;
    indexTime = 0;
    trainTime = 0;
}

