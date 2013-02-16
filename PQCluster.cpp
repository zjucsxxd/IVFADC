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

unsigned int PQCluster::get_nsq()
{
    return (unsigned int)(nsq);
}

void PQCluster::quantize2leaf(float* voc, int* result, int n)
{
    memset(result, 0, sizeof(int)*n*nsq); // init out to 0
    int out;
    //std::cout << "n:" << n << "m:" << m << std::endl;
    for(int i = 0; i < nsq; i++)
    {
        quantize_once(voc + i*ds, &out, i);
        result[i] = out;
    }
}

void PQCluster::quantize_once(float* vec, int* out, int nsq_num)
{
    float dist_best = 1e100;
    for(int i = 0; i < ks; i++) // for each centroids 
    {
        
            float dist = Util::dist_l2_sq(vec, clusters+nsq_num*ds*ks+i*ds, ds);
            if(dist < dist_best)
            {
                dist_best = dist;
                *out = i;
            }
    }
}

void PQCluster::print_clusters()
{
    for(int x=0; x < nsq; x++)
    {
        printf("codebook: %d\n",x);
        for(int i = 0; i < ks; i++)
        {
            for(int j=0; j < ds; j++)
            {
                printf("%.8f ",clusters[x*ks*ds+i*ds+j]);
            }
            printf("\n");
        }
    }
}

PQCluster::~PQCluster()
{
    delete[] clusters;
}