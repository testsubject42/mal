#include "spmvCore.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <utility>
#include <stdint.h>
#include <cassert>
#include <map>
#include <boost/chrono/include.hpp>
#include <boost/chrono.hpp>
#include "r3.h"

using namespace std;
using namespace boost::chrono;

void kSort(double* begin, int size, int* kNN, int k);

bool compPtr (double* i, double* j){
    return *i < *j;
}

vector<vector<pair<int, double> > > findNearestNeihbors(int trainM, int trainN, int trainNnz, vector<int> &trainI, vector<int> &trainJ, vector<double> &trainV, int testN, int testM, int testNnz, vector<int> &testI, vector<int> &testJ, vector<double> &testV){
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    double spmvTime = 0;
    int k = 30;
    if(trainM < k)
        k = trainM;
    vector<vector<pair<int, double> > > ret;
    vector<double> intermediateY;
    double* hardwareY = (double*)cny_cp_malloc(sizeof(double) * trainM);
    for(int i = 0; i < trainM; i++){
        hardwareY[i] = 0;
    }
    vector<double> hostY;
    hostY.resize(trainM);
    for(int i = 0; i < hostY.size(); i++){
        hostY[i] = 0;
    }
    double* xVectorHardware = (double*)cny_cp_malloc(sizeof(double) * trainN);
    int currTestRow = 0;
    spoonHeader* trainSpoon = cnySpoonFmt(&trainI[0], &trainJ[0], &trainV[0], trainM, trainN, trainNnz);
    currTestRow = 0;
    int firstRowElement = 0;
    for(int i = 0; i < trainN; i++){
        xVectorHardware[i] = 0;
    }
    vector<int> kNNIndices;
    for(int i = 0; i < k; i++)
        kNNIndices.push_back(-1);
    for(int i = 0; i < testNnz; i++){
        if(testI[i] != currTestRow){
            runR3(trainSpoon,&xVectorHardware[0], &hardwareY[0]);
            cny_cp_memcpy(&hostY[0], &hardwareY[0], trainM * sizeof(double));

            kSort(&hostY[0], hostY.size(), &kNNIndices[0], k);
            for(int j = 0; j < k; j++){
                ret[currTestRow].push_back(make_pair(kNNIndices[j],intermediateY[kNNIndices[j]]));
            }
            for(; firstRowElement < i; firstRowElement++){
                xVectorHardware[testJ[firstRowElement]] = 0;
            }
            for(int j = 0; j < k; j++)
                kNNIndices[j] = -1;
#ifdef DEBUG
            for(int j = 0; j < trainN; j++){
                assert(xVectorHardware[j] == 0);
            }
#endif
            currTestRow++;
        }
        xVectorHardware[testJ[i]] = testV[i];
    }
    runR3(trainSpoon,&xVectorHardware[0], &hardwareY[0]);
    cny_cp_memcpy(&hostY[0], &hardwareY[0], trainM * sizeof(double));

    kSort(&hostY[0], hostY.size(), &kNNIndices[0], k);
    for(int j = 0; j < k; j++){
        ret[currTestRow].push_back(make_pair(kNNIndices[j],intermediateY[kNNIndices[j]]));
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double> >(t2 - t1);
    cerr << "Classification took: " << time_span.count() << " seconds." << endl;
    cerr << "SpMV took: " << spmvTime << " seconds." << endl;
    return ret;
}

void kSort(double* begin, int size, int* kNN, int k){
    if(k > size)
        k = size;
    for(int i = 0; i < size; i++){
        if(kNN[k-1] == -1){
        }else if(begin[i] < begin[kNN[k-1]])
            continue;
        bool largest = true;
        for(int j = k - 2; j >= 0; j--){
            if(kNN[j] == -1){
            }else if(begin[i] > begin[kNN[j]]){
                kNN[j+1] = kNN[j];
            }else{
                kNN[j + 1] = i;
                largest = false;
                break;
            }
        }
        if(largest)
            kNN[0] = i;
    }
}
