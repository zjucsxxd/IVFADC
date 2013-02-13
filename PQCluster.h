//
//  PQCluster.h
//  IVFADC4
//
//  Created by Yudong Jiang on 2/12/13.
//  Copyright (c) 2013 Yudong Jiang. All rights reserved.
//

#ifndef __IVFADC4__PQCluster__
#define __IVFADC4__PQCluster__

#include <iostream>
#include <string>
using namespace std;
class PQCluster
{
private:
    float* clusters;
    int nsq;
    int ks; // number of centroids for subquantizer.
    int ds; // dimension of the subvectors to quantize.
public:
    PQCluster(int nsqbits, int nsq, int d);
    float* subvec(int i);
    void write2Disk(string centroids_dir, int i);
    void loadFromDisk(string centroids_dir);
    void quantize2leaf(float* voc, int* result, int n);
    void quantize_once(float* vec, int* out);
    
    int get_nsq();
    ~PQCluster();
};

#endif /* defined(__IVFADC4__PQCluster__) */
