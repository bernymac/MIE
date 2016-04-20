//
//  SSEClient.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 28/04/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "SSEClient.h"

#define CLUSTERS 1000
#define DOCS 1000

SSEClient::SSEClient() {
    cryptoTime = 0;
    cloudTime = 0;
    indexTime = 0;
    trainTime = 0;
    FeatureDetector::create( /*"Dense"*/ /*"PyramidDense"*/ "SURF" );//detector = xfeatures2d::SurfFeatureDetector::create();
    DescriptorExtractor::create( "SURF" );//extractor = xfeatures2d::SurfDescriptorExtractor::create();
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    bowExtractor = new BOWImgDescriptorExtractor( extractor, matcher );
    analyzer = new EnglishAnalyzer;
    textCrypto = new TextCrypt;
    aesCrypto = new SSECrypt;
}

SSEClient::~SSEClient() {
    detector.release();
    extractor.release();
    bowExtractor.release();
}

void SSEClient::train() {
    string s = homePath;
    s += "Data/SSE/dictionary.yml";
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
        if (fname == NULL) pee("malloc error in SSEClient::train()");
        for (unsigned i = 0; i < DOCS; i++) {
            if (rng.uniform(0.f,1.f) <= 0.1f) {
                bzero(fname, 120);
                sprintf(fname, "%sDatasets/wang/%d.jpg", homePath, i);
//                String fname = homePath; fname += "Datasets/wang/"; fname += to_string(i); fname += ".jpg";
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

void SSEClient::addDocs(const char* imgDataset, const char* textDataset, bool firstUpdate, int first, int last, int prefix) {
    //create or retrieve index
    vector< map<int,int> > imgIndex;
    imgIndex.resize(CLUSTERS);
    map<vector<unsigned char>,map<int,int> > textIndex;
    int sockfd = -1;
    if (!firstUpdate) {
        char buffer[1];
        buffer[0] = 'd';
        sockfd = connectAndSend(buffer, 1);
        receivePostingLists(sockfd, &imgIndex, &textIndex);
        socketReceiveAck(sockfd);
    }
    
    //index imgs
    char* fname = (char*)malloc(120);
    if (fname == NULL) pee("malloc error in SSEClient::addDocs fname");
    for (unsigned i=first; i<=last; i++) {
        bzero(fname, 120);
//        string fname = homePath; fname += "Datasets/"; fname += imgDataset; fname += "/im"; fname += to_string(i); fname += ".jpg";
        sprintf(fname, "%sDatasets/%s/im%d.jpg", homePath, imgDataset, i);
        Mat image = imread(fname);
        vector<KeyPoint> keypoints;
        Mat bowDesc;
        detector->detect( image, keypoints );
        timespec start = getTime();     //start crypto benchmark
        bowExtractor->compute( image, keypoints, bowDesc );
        cryptoTime += diffSec(start, getTime()); //end benchmark
        start = getTime();     //start index benchmark
        for (unsigned j=0; j<CLUSTERS; j++) {
            int val = denormalize(bowDesc.at<float>(j),(int)keypoints.size());
            if (val > 0)
                imgIndex[j][i+prefix] = val;
        }
        indexTime += diffSec(start, getTime()); //end benchmark
    }
    //index text
    for (unsigned i=first; i<=last; i++) {
        bzero(fname, 120);
        sprintf(fname, "%sDatasets/%s/tags%d.txt", homePath, textDataset, i);
//        string fname = homePath; fname += "Datasets/"; fname += imgDataset; fname += "/tags"; fname += to_string(i); fname += ".txt";
        vector<string> keywords = analyzer->extractFile(fname);
        for (int j = 0; j < keywords.size(); j++) {
            timespec start = getTime();     //start crypto benchmark
            vector<unsigned char> encKeyword = textCrypto->encode(keywords[j]);
            cryptoTime += diffSec(start, getTime()); //end benchmark
            start = getTime();               //start index benchmark
            map<int,int>* postingList = &textIndex[encKeyword];
            map<int,int>::iterator posting = postingList->find(i+prefix);
            if (posting == postingList->end())
                (*postingList)[i+prefix] = 1;
            else
                posting->second++;
            indexTime += diffSec(start, getTime());         //end benchmark
        }
    }
    free(fname);
    
    //encrypt img index
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
            if (buff == NULL) pee("malloc error in SSEClient::addDocs encImgIndex");
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
        if (buff == NULL) pee("malloc error in SSEClient::addDocs enTextIndex");
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
    
    //mandar para a cloud
    start = getTime();                          //start cloud benchmark
    long buffSize = 3*sizeof(int);
    for (int i = 0; i < encImgIndex.size(); i++)
        buffSize += sizeof(int) + encImgIndex[i].size()*sizeof(unsigned char);
    for (map<vector<unsigned char>,vector<unsigned char> >::iterator it=encTextIndex.begin(); it!=encTextIndex.end(); ++it)
        buffSize += TextCrypt::keysize*sizeof(unsigned char) + sizeof(int) + it->second.size()*sizeof(unsigned char);
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in SSEClient::addDocs sendCloud");
    int pos = 0;
    addIntToArr ((int)encImgIndex.size(), buff, &pos);
    addIntToArr ((int)encTextIndex.size(), buff, &pos);
    addIntToArr (TextCrypt::keysize, buff, &pos);
    for (int i = 0; i < encImgIndex.size(); i++) {
        addIntToArr ((int)encImgIndex[i].size(), buff, &pos);
        for (int j = 0; j < encImgIndex[i].size(); j++)
            addToArr(&encImgIndex[i][j], sizeof(unsigned char), buff, &pos);
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

void SSEClient::receivePostingLists(int sockfd, vector<map<int,int> >* imgPostingLists,
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
        if (buff2 == NULL) pee("malloc error in SSEClient::calculateQueryResults");
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
        if (buff2 == NULL) pee("malloc error in SSEClient::calculateQueryResults");
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

set<QueryResult,cmp_QueryResult> SSEClient::search(int id) {
    //process img object
    map<int,int> vws;
    char* fname = (char*)malloc(120);
    sprintf(fname, "%sDatasets/wang/%d.jpg", homePath, id);
//    string fname = homePath; fname += "Datasets/wang/"; fname += to_string(id); fname += ".jpg";
    Mat image = imread(fname);
    vector<KeyPoint> keypoints;
    Mat bowDesc;
    detector->detect( image, keypoints );
    timespec start = getTime();              //start crypto time benchmark
    bowExtractor->compute( image, keypoints, bowDesc );
    cryptoTime += diffSec(start, getTime()); //end benchmark
    start = getTime();                        //start index time benchmark
    for (unsigned i=0; i<CLUSTERS; i++) {
        const int queryTf = denormalize(bowDesc.at<float>(i),(int)keypoints.size());
        if (queryTf > 0)
            vws[i] = queryTf;
    }
    indexTime += diffSec(start, getTime());               //end benchmark
    //process text object
    map<vector<unsigned char>,int> encKeywords;
    bzero(fname, 120);
    sprintf(fname, "%sDatasets/docs/tags%d.txt", homePath, id+1);
//    fname = homePath; fname += "Datasets/docs/tags"; fname += to_string(id+1); fname += ".txt";
    vector<string> keywords = analyzer->extractFile(fname);
    for (int j = 0; j < keywords.size(); j++) {
        start = getTime();                  //start crypto time benchmark
        vector<unsigned char> encKeyword = textCrypto->encode(keywords[j]);
        cryptoTime += diffSec(start, getTime()); //end benchmark
        start = getTime();                   //start index time benchmark
        map<vector<unsigned char>,int>::iterator queryTf = encKeywords.find(encKeyword);
        if (queryTf == encKeywords.end()) {
            encKeywords[encKeyword] = 1;
        } else {
            queryTf->second++;
        }
        indexTime += diffSec(start, getTime());          //end benchmark
    }
    free(fname);
    
    //mandar para a cloud
    start = getTime();                      //start cloud time benchmark
    long buffSize = 1 + 3*sizeof(int) + vws.size()*sizeof(int) + encKeywords.size()*TextCrypt::keysize;
    char* buff = (char*)malloc(buffSize);
    if (buff == NULL) pee("malloc error in SSEClient::search sendCloud");
    int pos = 0;
    buff[pos++] = 's';       //cmd
    addIntToArr ((int)vws.size(), buff, &pos);
    addIntToArr ((int)encKeywords.size(), buff, &pos);
    addIntToArr (TextCrypt::keysize, buff, &pos);
    for (map<int,int>::iterator it=vws.begin(); it!=vws.end(); ++it)
        addIntToArr (it->first, buff, &pos);
    for (map<vector<unsigned char>,int>::iterator it=encKeywords.begin(); it!=encKeywords.end(); ++it) {
        for(int i = 0; i < it->first.size(); i++) {
            unsigned char x = it->first[i];
            addToArr(&x, sizeof(unsigned char), buff, &pos);
        }
    }
    int sockfd = connectAndSend(buff, buffSize);
//    const int x = (int)encKeywords.size()*TextCrypt::keysize+2*sizeof(int);
//    LOGI("Text Search network traffic part 1: %d\n",x);
    free(buff);
    cloudTime += diffSec(start, getTime());            //end benchmark
    
    //receive posting lists and calculate query results
    return calculateQueryResults(sockfd, &vws, &encKeywords);
}

set<QueryResult,cmp_QueryResult> SSEClient::calculateQueryResults(int sockfd, map<int,int>* vws,
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

map<int,int> SSEClient::decryptPostingList(unsigned char* encPostingList, int ciphertextSize) {
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
}

set<QueryResult,cmp_QueryResult> SSEClient::mergeSearchResults(set<QueryResult,cmp_QueryResult>* imgResults,
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
}

string SSEClient::printTime() {
    char char_time[120];
    sprintf(char_time, "cryptoTime:%f trainTime:%f indexTime:%f cloudTime:%f", cryptoTime, trainTime, indexTime, cloudTime);
    string time = char_time;
    return time;
}





#define READ_CODEBOOK 0 // set READ to 1 to read from disk. 0 to compute and write
#define READ_INDEX 0 // set READ to 1 to read from disk. 0 to compute and write
#define READ_QUERIES 0 // set READ to 1 to read from disk. 0 to compute and write

void sse_bovwDOCS() {
    Ptr<FeatureDetector> detector = FeatureDetector::create( "PyramidDense" ); //xfeatures2d::SurfFeatureDetector::create();
    Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create( "SURF" ); //xfeatures2d::SurfDescriptorExtractor::create();
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
    Ptr<BOWImgDescriptorExtractor> bowExtractor = new BOWImgDescriptorExtractor( extractor, matcher );
    string dataset = "/Users/bernardo/Dropbox/WorkspacePHD/MUSE/datasets/wang/";
    string extension = ".jpg";
    string data = "/Users/bernardo/Data/SSE";
    char numstr[21]; // enough to hold all numbers up to 64-bits
    FileStorage fs;
    
#if READ_CODEBOOK == 0
    TermCriteria terminate_criterion;
    terminate_criterion.epsilon = FLT_EPSILON;
    BOWKMeansTrainer bowTrainer ( CLUSTERS, terminate_criterion, 3, KMEANS_PP_CENTERS );
    //	const int elemSize = CV_ELEM_SIZE(extractor->descriptorType());
    //    const int descByteSize = extractor->descriptorSize() * elemSize;
    //    const int bytesInMB = 1048576;
    //    const int maxDescCount = (200 * bytesInMB) / descByteSize;
    LOGI("extract descriptors!\n");
    RNG& rng = theRNG();
    for (unsigned i = 0; i < DOCS; i++) {
        if (rng.uniform(0.f,1.f) <= 0.1f) {
            sprintf(numstr, "%d", i);
            Mat image = imread(dataset+numstr+extension);
            vector<KeyPoint> keypoints;
            Mat descriptors;
            detector->detect(image, keypoints);
            extractor->compute(image, keypoints, descriptors);
            bowTrainer.add(descriptors);
        }
    }
    LOGI("build codebook with %d descriptors!\n",bowTrainer.descripotorsCount());
    Mat dictionary = bowTrainer.cluster();
    
    fs.open(data+"dictionary.yml", FileStorage::WRITE);
    fs << "vocabulary" << dictionary;
    fs.release();
#else
    LOGI("reading codebook from disk!\n");
    Mat dictionary;
    fs.open(data+"dictionary.yml", FileStorage::READ);
    fs["vocabulary"] >> dictionary;
    fs.release();
#endif
#if READ_INDEX == 0
    LOGI("index!\n");
    bowExtractor->setVocabulary(dictionary);
    Mat bowImageDescriptors(DOCS,CLUSTERS,CV_32F);
    for (unsigned i=0; i<DOCS; i++) {
        sprintf(numstr, "%d", i);
        Mat image = imread(dataset+numstr+extension);
        vector<KeyPoint> keypoints;
        Mat bowDesc;
        detector->detect( image, keypoints );
        bowExtractor->compute( image, keypoints, bowDesc );
        for (unsigned j=0; j<CLUSTERS; j++)
            bowImageDescriptors.at<float>(i,j) = bowDesc.at<float>(j);
        //            bowImageDescriptors.at<float>(i,j) = round(bowDesc.at<float>(j) * keypoints.size());  //de-normalize histograms
    }
    vector<float> idfs;
    idfs.resize(CLUSTERS);
    for (unsigned i=0; i<CLUSTERS; i++) {
        float df = 0.f;
        for (unsigned j=0; j<DOCS; j++) {
            float x = bowImageDescriptors.at<float>(j,i);
            if (x > 0.f)
                df++;
        }
        idfs[i] = log10(1000.f / df);
        //        LOGI("idf %d: %f\n",i,idfs[i]);
    }
    
    fs.open(data+"index.yml", FileStorage::WRITE);
    fs << "tfs" << bowImageDescriptors;
    fs << "idfs" << idfs;
    fs.release();
#else
    LOGI("reading index from disk!\n");
    Mat bowImageDescriptors;
    vector<float> idfs;
    fs.open(data+"index.yml", FileStorage::READ);
    fs["tfs"] >> bowImageDescriptors;
    fs["idfs"] >> idfs;
    fs.release();
#endif
#if READ_QUERIES == 0
    //    LOGI("vocabulary rows: %d, cols: %d\n",dictionary.rows, dictionary.cols);
    //    LOGI("bow descriptor for img 0.jpg: rows %d, cols %d!\n",bowImageDescriptors[0].rows,bowImageDescriptors[0].cols);
    LOGI("run queries!\n");
    Ptr<DescriptorMatcher> searchMatcher = DescriptorMatcher::create( "BruteForce-L1" );
    for (unsigned i=0; i<DOCS; i++) {
        searchMatcher->add(vector<Mat>(1,bowImageDescriptors.row(i)));
        /*        vector<Mat> tfsIdfs(1,bowImageDescriptors.row(i));
         for (unsigned j=0; j<CLUSTERS; j++)
         tfsIdfs[0].at<float>(j) *= idfs[j];
         searchMatcher->add(tfsIdfs); */
    }
    vector< vector<int> > allMatches;
    allMatches.resize(DOCS);
    for (unsigned i=0; i<DOCS; i++) {
        vector< vector<DMatch> > matches;
        searchMatcher->radiusMatch(bowImageDescriptors.row(i),matches,2.f);
        allMatches[i].resize(matches[0].size());
        for (unsigned j=0; j<matches[0].size(); j++)
            allMatches[i][j] = matches[0][j].imgIdx;
    }
    fs.open(data+"queries.yml", FileStorage::WRITE);
    fs << "queries" << allMatches;
    fs.release();
#else
    LOGI("reading query results from disk!\n");
    vector< vector<int> > allMatches;
    fs.open(data+"queries.yml", FileStorage::READ);
    fs["queries"] >> allMatches;
    fs.release();
#endif
    LOGI("calculate Prec-Rec!\n");
    float totalRelevantDocuments = 100.f;
    vector<float> avgPrecVals;
    avgPrecVals.resize(DOCS);
    vector<float> avgRecVals;
    avgRecVals.resize(DOCS);
    for (unsigned querySet = 1; querySet <= DOCS; querySet++) {
        float sumPrec = 0.f, sumRec = 0.f;
        for (unsigned imgID = 0; imgID < DOCS; imgID++) {
            float relevantDocumentsRetrieved = 0.f;
            for (unsigned resultID = 0; resultID < querySet; resultID++) {
                if (resultID == allMatches[imgID].size())
                    break;
                int retrievedImgID = allMatches[imgID][resultID];
                if (wangIsRelevant(imgID, retrievedImgID))
                    relevantDocumentsRetrieved++;
            }
            sumPrec += (relevantDocumentsRetrieved / querySet);
            sumRec += (relevantDocumentsRetrieved / totalRelevantDocuments);
        }
        avgPrecVals[querySet-1] = (sumPrec/1000.f);
        avgRecVals[querySet-1] = (sumRec/1000.f);
    }
    LOGI("Recall:\n");
    for (unsigned i=0; i<DOCS; i++)
        LOGI("%f\n",avgRecVals[i]);
    LOGI("Precision:\n");
    for (unsigned i=0; i<DOCS; i++)
        LOGI("%f\n",avgPrecVals[i]);
}