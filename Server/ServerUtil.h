//
//  Util.h
//  MIE
//
//  Created by Bernardo Ferreira on 04/05/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//
#ifndef __MIE__ServerUtil__
#define __MIE__ServerUtil__

#include <set>
#include <map>
#include <vector>
#include <stdint.h>
#include <math.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include "zlib.h"
#include "portable_endian.h"


static const std::string dataPath = "/Users/bernardo/Data/";
//static const std::string dataPath = "/home/ubuntu/Data/";
static const int clusters = 1000;

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
struct cmp_QueryResult {
 bool operator() (const QueryResult& lhs, const QueryResult& rhs) const {return lhs.score>rhs.score;}
 };
*/

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

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

uint64_t pack754(long double f, unsigned bits, unsigned expbits);

long double unpack754(uint64_t i, unsigned bits, unsigned expbits);

//#define  LOGI(...)  fprintf(stdout,__VA_ARGS__)
void LOGI(const char* msg);

int initServer();

void ack(int newsockfd);

std::string getHexRepresentation(const unsigned char * Bytes, size_t Length);

void pee(const char *msg);

int sendall(int s, char *buf, long len);

//int connectAndSend (char* buff, long size);

void socketSend (int sockfd, char* buff, long size);

void socketReceive(int sockfd, char* buff, long size);

void socketReceiveAck(int sockfd);

int receiveAll (int s, char* buff, long len);

int receiveAll (int s, unsigned char* buff, long len);

void addToArr (void* val, int size, char* arr, int* pos);

void addIntToArr (int val, char* arr, int* pos);

void addFloatToArr (float val, char* arr, int* pos);

void readFromArr (void* val, int size, char* arr, int* pos);

int readIntFromArr (char* arr, int* pos);

float readFloatFromArr (char* arr, int* pos);

float getTfIdf (float qtf, float tf, float idf);

float getIdf (float nDocs, float df);

float bm25L(float rawTF, float queryTF, float idf, float docLength, float avgDocLength);

int denormalize(float val, int size);

std::set<QueryResult,cmp_QueryResult> sort (std::map<int,float>* queryResults);

std::set<QueryResult,cmp_QueryResult> mergeSearchResults(std::set<QueryResult,cmp_QueryResult>* imgResults,
                                                         std::set<QueryResult,cmp_QueryResult>* textResults);

void sendQueryResponse(int newsockfd, std::set<QueryResult,cmp_QueryResult>* mergedResults, int resultsSize);

void zipAndSend(int sockfd, char* buff, long size);

void receiveAndUnzip(int newsockfd, char* data, unsigned long* dataSize, unsigned long zipSize);

std::vector<unsigned char> f(unsigned char* key, int keySize, unsigned char* data, int dataSize);

int dec (unsigned char* key, unsigned char* ciphertext, int ciphertextSize, unsigned char* plaintext);

//struct timespec getTime();
//struct timespec diff(struct timespec start, struct timespec end);
//double diffSec(struct timespec start, struct timespec end);
//unsigned char *spc_rand(unsigned char *buf, int l);
//unsigned int spc_rand_uint();
//float spc_rand_real(void);
//float spc_rand_real_range(float min, float max);
//bool wangIsRelevant(int queryID, int resultID);
//std::string exec(const char* cmd);

#endif /* defined(__MIE__ServerUtil__) */
