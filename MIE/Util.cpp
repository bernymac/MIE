//
//  Util.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 05/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "Util.h"

uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit
    
    if (f == 0.0) return 0; // get this special case out of the way
    
    // check sign and begin normalization
    if (f < 0) { sign = 1; fnorm = -f; }
    else { sign = 0; fnorm = f; }
    
    // get the normalized form of f and track the exponent
    shift = 0;
    while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
    while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
    fnorm = fnorm - 1.0;
    
    // calculate the binary form (non-float) of the significand data
    significand = fnorm * ((1LL<<significandbits) + 0.5f);
    
    // get the biased exponent
    exp = shift + ((1<<(expbits-1)) - 1); // shift + bias
    
    // return the final answer
    return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit
    
    if (i == 0) return 0.0;
    
    // pull the significand
    result = (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits); // convert back to float
    result += 1.0f; // add the one back on
    
    // deal with the exponent
    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }
    
    // sign it
    result *= (i>>(bits-1))&1? -1.0: 1.0;
    
    return result;
}

int denormalize(float val, int size) {
    return round(val * size);
}

struct timespec getTime() {
    struct timespec ts;
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    return ts;
}

struct timespec diff(struct timespec start, struct timespec end) {
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

double diffSec(struct timespec start, struct timespec end) {
    double startNano = start.tv_sec+(start.tv_nsec/1000000000.0);
    double endNano = end.tv_sec+(end.tv_nsec/1000000000.0);
    return endNano - startNano;
}

unsigned char *spc_rand(unsigned char *buf, int l) {
    if (!RAND_bytes(buf, l)) {
        fprintf(stderr, "The PRNG is not seeded!\n");
        abort(  );
    }
    return buf;
}

unsigned int spc_rand_uint() {
    unsigned int res;
    spc_rand((unsigned char *)&res, sizeof(unsigned int));
    return res;
}

float spc_rand_real(void) {
    return ((float)spc_rand_uint()) / (float)UINT_MAX;
}

float spc_rand_real_range(float min, float max) {
    if (max < min) abort();
    return spc_rand_real() * (max - min) + min;
}

std::string getHexRepresentation(const unsigned char * Bytes, size_t Length)
{
    std::ostringstream os;
    os.fill('0');
    os<<std::hex;
    for(const unsigned char * ptr=Bytes;ptr<Bytes+Length;ptr++)
        os<<std::setw(2)<<(unsigned int)*ptr;
    return os.str();
}

void pee(const char *msg)
{
    perror(msg);
    exit(0);
}

int connectAndSend (char* buff, long size) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *server = gethostbyname(serverIP);
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(9978);
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
        pee("ERROR connecting");
    socketSend (sockfd, buff, size);
    return sockfd;
}

void socketSend (int sockfd, char* buff, long size) {
    if (sendall(sockfd, buff, size) < 0)
        pee("ERROR writing to socket");
}

int sendall(int s, char *buf, long len)
{
    long total = 0;        // how many bytes we've sent
    long bytesleft = len; // how many we have left to send
    long n = 0;
    
    while(total < len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    
    return n==-1||total!=len ? -1 : 0; // return -1 on failure, 0 on success
}

void socketReceive(int sockfd, char* buff, long size) {
    if (receiveAll(sockfd, buff, size) < 0)
        pee("ERROR reading from socket");
}

int receiveAll (int socket, char* buff, long len) {
    int r = 0;
    while (r < len) {
        ssize_t n = read(socket,&buff[r],len-r);
        if (n < 0) pee("ERROR reading from socket");
        r+=n;
    }
    return r;
}

void socketReceiveAck(int sockfd) {
    char buff[3];
    socketReceive(sockfd, buff, 3);
    //    string s (buff);
    //    if (s != "ack")
    //        pee(("ERROR expected ack, received: "+s).data());
    close(sockfd);
}

void addToArr (void* val, int size, char* arr, int* pos) {
    memcpy(&arr[*pos], val, size);
    *pos += size;
}

void addIntToArr (int val, char* arr, int* pos) {
    uint32_t x = htonl(val);
    addToArr (&x, sizeof(uint32_t), arr, pos);
}

void addFloatToArr (float val, char* arr, int* pos) {
    uint64_t x = pack754_32(val);
    addToArr (&x, sizeof(uint64_t), arr, pos);
}

void readFromArr (void* val, int size, char* arr, int* pos) {
    memcpy(val, &arr[*pos], size);
    *pos += size;
}

int readIntFromArr (char* arr, int* pos) {
    uint32_t x;
    readFromArr(&x, sizeof(uint32_t), arr, pos);
    return ntohl(x);
}

float readFloatFromArr (char* arr, int* pos) {
    uint64_t x;
    readFromArr(&x, sizeof(uint64_t), arr, pos);
    return (float)unpack754_32(x);
}

bool wangIsRelevant(int queryID, int resultID) {
    if (queryID >= 0 && queryID < 100) {
        if (resultID >= 0 && resultID < 100)
            return true;
    } else if (queryID >= 100 && queryID < 200) {
        if (resultID >= 100 && resultID < 200)
            return true;
    } else if (queryID >= 200 && queryID < 300) {
        if (resultID >= 200 && resultID < 300)
            return true;
    } else if (queryID >= 300 && queryID < 400) {
        if (resultID >= 300 && resultID < 400)
            return true;
    } else if (queryID >= 400 && queryID < 500) {
        if (resultID >= 400 && resultID < 500)
            return true;
    } else if (queryID >= 500 && queryID < 600) {
        if (resultID >= 500 && resultID < 600)
            return true;
    } else if (queryID >= 600 && queryID < 700) {
        if (resultID >= 600 && resultID < 700)
            return true;
    } else if (queryID >= 700 && queryID < 800) {
        if (resultID >= 700 && resultID < 800)
            return true;
    } else if (queryID >= 800 && queryID < 900) {
        if (resultID >= 800 && resultID < 900)
            return true;
    } else if (queryID >= 900 && queryID < 1000) {
        if (resultID >= 900 && resultID < 1000)
            return true;
    }
    return false;
}

float getTfIdf (float tf, float idf) {
    /*    if (tf != 0)
     return (1+log10(tf))* idf;
     else
     return 0;
     */
    return tf*idf;
}

float getIdf (float nDocs, float df) {
    return log10(nDocs / df);
}

std::set<QueryResult,cmp_QueryResult> sort (std::map<int,float>* queryResults) {
    std::set<QueryResult,cmp_QueryResult> orderedResults;
    for (std::map<int,float>::iterator it=queryResults->begin(); it!=queryResults->end(); ++it) {
        struct QueryResult qr;
        qr.docId = it->first;
        qr.score = it->second;
        orderedResults.insert(qr);
    }
    return orderedResults;
}

std::set<QueryResult,cmp_QueryResult> mergeSearchResults(std::set<QueryResult,cmp_QueryResult>* imgResults,
                                                         std::set<QueryResult,cmp_QueryResult>* textResults) {
    const float sigma = 0.01f;
    //prepare ranks
    std::map<int,Rank> ranks;
    if (textResults != NULL) {
        int i = 1;
        for (std::set<QueryResult,cmp_QueryResult>::iterator it=textResults->begin(); it!=textResults->end(); ++it) {
            struct Rank qr;
            qr.textRank = i++;
            qr.imgRank = 0;
            ranks[it->docId] = qr;
        }
    }
    if (imgResults != NULL) {
        int i = 1;
        for (std::set<QueryResult,cmp_QueryResult>::iterator it=imgResults->begin(); it!=imgResults->end(); ++it) {
            std::map<int,Rank>::iterator rank = ranks.find(it->docId);
            if (rank == ranks.end()) {
                struct Rank qr;
                qr.textRank = 0;
                qr.imgRank = i++;
                ranks[it->docId] = qr;
            } else
                rank->second.imgRank = i++;
        }
    }
    std::map<int,float> queryResults;
    for (std::map<int,Rank>::iterator it=ranks.begin(); it!=ranks.end(); ++it) {
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

std::vector<QueryResult> receiveQueryResults(int sockfd) {
    char buff[sizeof(int)];
    socketReceive(sockfd, buff, sizeof(int));
    int pos = 0;
    int resultsSize = readIntFromArr(buff, &pos);
    std::vector<QueryResult> queryResults;
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
}

std::string exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

void zipAndSend(int sockfd, char* buff, long size) {
    unsigned long zipSize = compressBound(size);
    char* zip = (char*)malloc(zipSize + 2*sizeof(uint64_t));
    int result = compress((unsigned char*)zip + 2*sizeof(uint64_t), &zipSize, (unsigned char*)buff, size);
    if (result != Z_OK)
        switch(result) {
            case Z_MEM_ERROR:
                pee("note enough memory for compression");
                break;
            case Z_BUF_ERROR:
                pee("note enough room in buffer to compress the data");
                break;
        }
    uint64_t serializedZipSize = htobe64(zipSize);
    memcpy(zip, &serializedZipSize, sizeof(uint64_t));
    
    uint64_t serializedDataSize = htobe64(size);
    memcpy(zip + sizeof(uint64_t), &serializedDataSize, sizeof(uint64_t));
    
    socketSend (sockfd, zip, zipSize + 2*sizeof(uint64_t)) ;
    free(zip);
//    LOGI("Search network traffic part 1: %ld\n",zipSize + 2*sizeof(uint64_t));
}

long receiveAndUnzip(int sockfd, char* data) {
    char buff[2*sizeof(uint64_t)];
    socketReceive(sockfd, buff, 2*sizeof(uint64_t));
    
    unsigned long zipSize;
    memcpy(&zipSize, buff, sizeof(uint64_t));
    zipSize = be64toh(zipSize);
    
    unsigned long dataSize;
    memcpy(&dataSize, buff + sizeof(uint64_t), sizeof(uint64_t));
    dataSize = be64toh(dataSize);

    char* zip = (char*)malloc(zipSize);
    socketReceive(sockfd, zip, zipSize);
    
    data = (char*)malloc(dataSize);
    int result = uncompress((unsigned char*)data, &dataSize, (unsigned char*)zip, zipSize);
    free(zip);
    if (result != Z_OK)
        switch(result) {
            case Z_MEM_ERROR:
                pee("note enough memory for uncompression");
                break;
            case Z_BUF_ERROR:
                pee("note enough room in buffer to uncompress the data");
                break;
            case Z_DATA_ERROR:
                pee("compressed data corrupted or incomplete");
                break;
        }
    return dataSize;
}

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


/*std::vector<int> holidayQueries (char* fName) {
    std::vector<int> result;
    result.resize(500);         //could also count nº of lines or use dynamic structure
    std::string line;
    std::ifstream fstream (fName);
    if (fstream.is_open()) {
        while ( getline (fstream,line) )                  //split file around lines
            if (line.size() > 0) {
                std::istringstream lstream(line);
                std::string queryFileName;
                getline (lstream,queryFileName);            //split line around spaces
                std::istringstream wstream(queryFileName);
                std::string queryId;
                std::getline(wstream, queryId, '.');   //split word around '.'
                result.push_back(atoi(queryId.c_str()));
            }
        fstream.close();
    } else
        pee("Util::holidayQueries Unable to open file");
    
    
    return result;
}*/

void processHolidayDataset (int nImgs, std::map<int,std::string>& imgs) {
    std::string holidayDir = homePath;
    holidayDir += "Datasets/inriaHolidays/";
    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir (holidayDir.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL && i < nImgs) {
            std::string fname = ent->d_name;
            const size_t pos = fname.find(".jpg");
            if (pos != std::string::npos) {
                std::string idString = fname.substr(0,pos);
                const int id = atoi(idString.c_str());
                std::string path = holidayDir;
                path += fname;
                imgs[id] = path;
                i++;
            }
        }
        closedir (dir);
    } else
        pee ("Util::processHolidayDataset couldn't open dir");
}

void processFlickrImgsDataset (int nImgs, std::map<int,std::string>& imgs) {
    std::string holidayDir = homePath;
    holidayDir += "Datasets/flickr_imgs/";
    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir (holidayDir.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL && i < nImgs) {
            std::string fname = ent->d_name;
            const size_t pos = fname.find(".jpg");
            if (pos != std::string::npos) {
                std::string idString = fname.substr(2,pos-2);
                const int id = atoi(idString.c_str());
                std::string path = holidayDir;
                path += fname;
                imgs[id] = path;
                i++;
            }
        }
        closedir (dir);
    } else
        pee ("Util::processFlickrImgsDataset couldn't open dir");
}

void processFlickrTagsDataset (int nImgs, std::map<int,std::string>& docs) {
    std::string holidayDir = homePath;
    holidayDir += "Datasets/flickr_tags/";
    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir (holidayDir.c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL && i < nImgs) {
            std::string fname = ent->d_name;
            const size_t pos = fname.find(".txt");
            if (pos != std::string::npos) {
                std::string idString = fname.substr(4,pos-4);
                const int id = atoi(idString.c_str());
                std::string path = holidayDir;
                path += fname;
                docs[id] = path;
                i++;
            }
        }
        closedir (dir);
    } else
        pee ("Util::processFlickrTagsDataset couldn't open dir");
}

void printHolidayResults (std::string fPath, std::map<int,std::vector<QueryResult> > results) {
    std::ofstream ofs (fPath.c_str());
    for (std::map<int,std::vector<QueryResult> >::iterator it = results.begin(); it != results.end(); ++it) {
        ofs << it->first << ".jpg";
        for (int i = 0; i < it->second.size(); i++) {
            ofs << " " << i << " " << it->second.at(i).docId << ".jpg";
        }
        ofs << std::endl;
    }
    ofs.close();
}

