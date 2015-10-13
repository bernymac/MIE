//
//  Util.cpp
//  MIE
//
//  Created by Bernardo Ferreira on 05/03/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//

#include "ServerUtil.h"

/*
static const std::string dataPath = "/Users/bernardo/Data/MIE";
static const int clusters = 1000;

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))
*/
 
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

void pee(const char *msg) {
    perror(msg);
    exit(0);
}


int initServer() {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        pee("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 9978;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind((int)sockfd, (const struct sockaddr *) &serv_addr,(socklen_t)sizeof(serv_addr)) < 0)
        pee("ERROR on binding");
    listen(sockfd,5);
    return sockfd;
}

void ack(int newsockfd) {
    if (write(newsockfd,"ack",3) < 0)
        pee("ERROR writing to socket");
    close(newsockfd);
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

/*int connectAndSend (char* buff, long size) {
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
}*/

void socketSend (int sockfd, char* buff, long size) {
    if (sendall(sockfd, buff, size) < 0)
        pee("ERROR writing to socket");
}

void socketReceive(int sockfd, char* buff, long size) {
    if (receiveAll(sockfd, buff, size) < 0)
        pee("ERROR reading from socket");
}

int receiveAll (int s, char* buff, long len) {
    int r = 0;
    while (r < len) {
        ssize_t n = read(s,&buff[r],len-r);
        if (n < 0) pee("ERROR reading from socket");
        r+=n;
    }
    return r;
}

int receiveAll (int s, unsigned char* buff, long len) {
    int r = 0;
    while (r < len) {
        ssize_t n = read(s,&buff[r],len-r);
        if (n < 0) pee("ERROR reading from socket");
        r+=n;
    }
    return r;
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
    int x = -1;
    readFromArr(&x, sizeof(int), arr, pos);
    return ntohl(x);
}

float readFloatFromArr (char* arr, int* pos) {
    float x = -1.f;
    readFromArr(&x, sizeof(float), arr, pos);
    return unpack754_32(x);
}

#include <math.h>

float getTfIdf (float qtf, float tf, float idf) {
/*    if (tf != 0)
        return qtf * (1+log10(tf))* idf;
    else
        return 0;
 */
    return qtf * tf * idf;
}

float getIdf (float nDocs, float df) {
    return log10(nDocs / df);
}

float bm25L(float rawTF, float queryTF, float idf, float docLength, float avgDocLength) {
    const float b = 0.4f;
    const float k1 = 1.2f;
    const float k3 = 1000.f;
    const float delta = 0.5f;
    
    const float k3TF = ((k3+1)*queryTF) / (k3+queryTF);
    
    const float normTF = rawTF / (1-b + b*docLength/avgDocLength);
    const float sublinearTF = normTF <= 0 ? 0 : ((k1+1)*(normTF + delta)) / (k1+(normTF + delta));
    
    return k3TF * sublinearTF * idf;
}

int denormalize(float val, int size) {
    return round(val * size);
}

std::set<QueryResult,cmp_QueryResult> sort (std::map<int,float> queryResults) {
    std::set<QueryResult,cmp_QueryResult> orderedResults;
    for (std::map<int,float>::iterator it=queryResults.begin(); it!=queryResults.end(); ++it) {
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
    return sort(queryResults);
}

void sendQueryResponse(int newsockfd, std::set<QueryResult,cmp_QueryResult>* mergedResults) {
    int resultsSize = (int)mergedResults->size();
    long size = sizeof(int) + resultsSize * (sizeof(int) + sizeof(uint64_t));
    char* buff = (char*)malloc(size);
    bzero(buff,size);
    int pos = 0;
    addIntToArr (resultsSize, buff, &pos);
    int i = 1;
    for (std::set<QueryResult,cmp_QueryResult>::iterator it = mergedResults->begin(); it != mergedResults->end(); ++it) {
        int docId = it->docId;
        float score = it->score;
        addIntToArr(docId, buff, &pos);
        addFloatToArr(score, buff, &pos);
        if (i == 20)      //Should be parameter k, number of query results
            break;
        else
            i++;
    }
    socketSend (newsockfd, buff, size);
    printf("Search Network Traffic part 2: %ld\n",size);
    free(buff);
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
}

void receiveAndUnzip(int newsockfd, char* data, unsigned long* dataSize, unsigned long zipSize) {
    char* zip = (char*)malloc(zipSize);
    if (zip == NULL) pee("malloc error in receiveAndUnzip");
    socketReceive(newsockfd, zip, zipSize);
    int result = uncompress((unsigned char*)data, dataSize, (unsigned char*)zip, zipSize);
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
}

std::vector<unsigned char> f(unsigned char* key, int keySize, unsigned char* data, int dataSize) {
    unsigned char md[keySize];
    unsigned int mdSize;
    HMAC(EVP_sha1(), key, keySize, data, dataSize, md, &mdSize);
    if (mdSize != keySize)
        pee("ServerUtil::f - md size of different from expected\n");
    std::vector<unsigned char> v;
    v.resize(keySize);
    for (int i = 0; i < keySize; i++)
        v[i] = md[i];
    return v;
}

int dec (unsigned char* key, unsigned char* ciphertext, int ciphertextSize, unsigned char* plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    unsigned char iv[16] = {0};
    
    if (!(ctx = EVP_CIPHER_CTX_new()))
        pee("ServerUtil::decrypt - could not create ctx\n");
    
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv)) //key will have 20 bytes but aes128 will only use the first 16
        pee("ServerUtil::decrypt - could not init decrypt\n");
    
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertextSize))
        pee("ServerUtil::decrypt - could not decrypt update\n");
    plaintext_len = len;
    
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        pee("ServerUtil::decrypt - could not decrypt final\n");
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    /*
     vector<unsigned char> v;
     v.resize(plaintext_len);
     for (int i = 0; i < plaintext_len; i++)
     v[i] = plaintext[i];
     free(plaintext);
     return v;
     */
    return plaintext_len;
}


/*
struct cmp_hash {
    bool operator()(std::vector<unsigned char> a, std::vector<unsigned char> b) {
        if (a.size() != b.size())
            return false;
        for (int i = 0; i < a.size(); i++)
            if (a[i] != b[i])
                return false;
        return true;
    }
};

struct QueryResult {
    int docId;
    float score;
};


struct cmp_QueryResult {
    bool operator() (const QueryResult& lhs, const QueryResult& rhs) const {
        return lhs.docId != rhs.docId && lhs.score >= rhs.score;
    }
};
 
struct Rank {
int textRank = 0;
int imgRank = 0;
};
 
 std::set<QueryResult,cmp_QueryResult> sort (std::map<int,float> queryResults) {
 std::set<QueryResult,cmp_QueryResult> orderedResults;
 for (std::map<int,float>::iterator it=queryResults.begin(); it!=queryResults.end(); ++it) {
 struct QueryResult qr;
 qr.docId = it->first;
 qr.score = it->second;
 orderedResults.insert(qr);
 }
 return orderedResults;
 }
 
*/
