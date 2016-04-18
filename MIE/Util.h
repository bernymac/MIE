//
//  Util.h
//  MIE
//
//  Created by Bernardo Ferreira on 04/05/15.
//  Copyright (c) 2015 NovaSYS. All rights reserved.
//
#ifndef __MIE__Util__
#define __MIE__Util__

#include <iostream>
#include <fstream>
#include <dirent.h>
#include <set>
#include <map>
#include <vector>
#include <stdint.h>
#include <math.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include "zlib.h"
#include "portable_endian.h"

//desktop
static const char* dataPath = "/Users/bernardo/Data";
static const char* datasetsPath = "/Users/bernardo/Datasets";
static const char* serverIP = "127.0.0.1";//"54.194.253.119";
#define  LOGI(...)  fprintf(stdout,__VA_ARGS__)
//mobile
//static const char* dataPath = "/sdcard/Data";
//static const char* datasetsPath = "/sdcard/Datasets";
//static const char* serverIP = "52.30.61.162"; //"10.171.239.42";
//#include <android/log.h>
//#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,"nonfree_jni_demo",__VA_ARGS__)

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
    int textRank;
    int imgRank;
};

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

uint64_t pack754(long double f, unsigned bits, unsigned expbits);

long double unpack754(uint64_t i, unsigned bits, unsigned expbits);

int denormalize(float val, int size);

struct timespec getTime();

struct timespec diff(struct timespec start, struct timespec end);

double diffSec(struct timespec start, struct timespec end);

unsigned char *spc_rand(unsigned char *buf, int l);

unsigned int spc_rand_uint();

float spc_rand_real(void);

float spc_rand_real_range(float min, float max);

std::string getHexRepresentation(const unsigned char * Bytes, size_t Length);

void pee(const char *msg);

int sendall(int s, char *buf, long len);

int connectAndSend (char* buff, long size);

void socketSend (int sockfd, char* buff, long size);

void socketReceive(int sockfd, char* buff, long size);

void socketReceiveAck(int sockfd);

int receiveAll (int s, char* buff, long len);

void addToArr (void* val, int size, char* arr, int* pos);

void addIntToArr (int val, char* arr, int* pos);

void addFloatToArr (float val, char* arr, int* pos);

void readFromArr (void* val, int size, char* arr, int* pos);

int readIntFromArr (char* arr, int* pos);

float readFloatFromArr (char* arr, int* pos);

bool wangIsRelevant(int queryID, int resultID);

float getTfIdf (float tf, float idf);

float getIdf (float nDocs, float df);

std::set<QueryResult,cmp_QueryResult> sort (std::map<int,float>* queryResults);

std::set<QueryResult,cmp_QueryResult> mergeSearchResults(std::set<QueryResult,cmp_QueryResult>* imgResults,
                                                         std::set<QueryResult,cmp_QueryResult>* textResults);

std::vector<QueryResult> receiveQueryResults(int sockfd);

std::string exec(const char* cmd);

void zipAndSend(int sockfd, char* buff, long size);

long receiveAndUnzip(int sockfd, char* data);

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems);

void processHolidayDataset (int nImgs, std::map<int,std::string>& imgs);

void processFlickrImgsDataset (int nImgs, std::map<int,std::string>& imgs);

void processFlickrTagsDataset (int nImgs, std::map<int,std::string>& docs);

void printHolidayResults (std::string fPath, std::map<int,std::vector<QueryResult> > results);

#endif /* defined(__MIE__Util__) */
