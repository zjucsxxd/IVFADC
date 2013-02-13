//
//  PQCluster.cpp
//  IVFADC4
//
//  Created by Yudong Jiang on 2/12/13.
//  Copyright (c) 2013 Yudong Jiang. All rights reserved.
//

#include "PQCluster.h"
#include <cmath>
#include <iostream>
#include "entry.h"
#include "util.h"
#include "IO.h"

PQCluster::PQCluster(int nsqbits_l, int nsq_l, int d)
{
    ks = ROUND(pow(2,nsqbits_l));
    ds = d/nsq_l;
    nsq = nsq_l;
    clusters = new float[ks*ds*nsq_l];
}


float* PQCluster::subvec(int i)
{
    return (clusters+i*ds*ks);
}

void PQCluster::write2Disk(string centroids_dir, int i)
{
    IO::writeMat(clusters + i*ds*ks, ks, ds, centroids_dir + "vocab.l" + Util::num2str(i));
}

void PQCluster::loadFromDisk(string centroids_dir)
{
    printf("Loading vocabulary: '%s'...\n", centroids_dir.c_str());
    int row, col;
    for(int i = 0; i < nsq; i++)
    {
        float* tmpmat = IO::loadFMat(centroids_dir + "vocab.l" + Util::num2str(i), row, col, -1);
        memcpy(clusters + ks*ds*i, tmpmat, row*col*sizeof(float));
        delete[] tmpmat;
    }
}

int PQCluster::get_nsq()
{
    return nsq;
}

void PQCluster::quantize2leaf(float* voc, int* result, int n)
{
    result = new int [n*nsq];
    memset(result, 0, sizeof(int)*n); // init out to 0
    int* out = NULL;
    //std::cout << "n:" << n << "m:" << m << std::endl;
    for(int i = 0; i < nsq; i++)
    {
        quantize_once(voc + i*ds, out);
        result[i] = *out;
        delete out;
    }
}

void PQCluster::quantize_once(float* vec, int* out)
{
    out = new int;
    for(int i = 0; i < nsq; i++) // for each subvector
    {
        float dist_best = 1e100;
        for(int j = 0; j < ks; j++) // for each centroids of the subvector.f
        {
            float dist = Util::dist_l2_sq(vec, clusters+i*ds, ds);
            if(dist < dist_best)
            {
                dist_best = dist;
                *out = j;
            }
        }
    }
}

PQCluster::~PQCluster()
{
    delete[] clusters;
}