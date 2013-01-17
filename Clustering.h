/**
@file Clustering.h
@brief this file implements an efficient hierarchical k-means, which utilizes
multiple cores of a machine.
@author wzhang
@date May 7th, 2012
*/

#ifndef CLUSTERING_H_INCLUDED
#define CLUSTERING_H_INCLUDED

#include <cstring>
#include <pthread.h>
#include <ctime>
#include <cstdio>
#include <stdio.h>
#include <iostream>

using namespace std;

#include "MultiThd.h"
#include "util.h"

/// argument used to conduct nearest neighbour search
struct nn_par
{
    // input
    const float* centers;
    const float* data;
    int k;
    int d;
    // output
    int *assignment;   // assignment of each points to center id
    float* cost;
    int iterator;
};

struct nn_par2
{
    // input
    const float* centers;
    const float* data;
    int k;
    int d;
    // output
    int *assignment;   // assignment of each points to center id
    float* cost;
    int iterator;
    float* residual;
};


/// argument of k-means
struct kmeans_par
{
    // input
    /// data to be clustered. there are 'nxd' floating points in data
    float   *data;
    /// number of data
    int     n;
    /// dimension of data
    int     d;
    /// number of centroids
    int     k;
    /// number of iterations
    int     iter;
    /// number of try. run with min cost will be returned
    int     attempts;
    /// number of threads to use
    int     nt;

    // output
    /// pointer to centers to be generated. size (k*d) must be pre-allocated
    float   *centers;
};

/// cluster data utility with k-means supported
class Clustering
{
public:

    /**
    k-means clustering routine
    @brief main interface of k-means
    @param para k-menas parameter of type kmeans_par
    @return the cost of clustering. (average squared distance)
    */
    static float kmeans(kmeans_par* para)
    {
        printf("K-MEANS -- data: %u x %u  k: %u  iter: %u  attempts: %u.\n", para->n, para->d, para->k, para->iter, para->attempts);

        float cost = 1e100;
        float* centers_tmp = new float[para->k * para->d];
        for(int i = 0; i < para->attempts; i++)
        {
            float cost_tmp = 0.0f;
            cout << "start kmeans once" << endl;
            kmeans_once(para->data, centers_tmp, para->n, para->d, para->k, para->iter, cost_tmp, para->nt);
            printf("Attempts: %u, cost: %e\n", i, cost_tmp);

            if( cost_tmp < cost )
            {
                printf("*********************************\n");
                printf("copy min cost to para->centers.\n");
                printf("*********************************\n");
                cost = cost_tmp;
                memcpy(para->centers, centers_tmp, sizeof(float) * para->k * para->d);
            }
        }
        //delete[] centers_tmp;
        return cost;
    }

    static void nn_task2(void* arg, int tid, int i, pthread_mutex_t& mutex)
    {
        nn_par2* t = (nn_par2*) arg;
        //printf("tid: %d, i: %d\n", tid, i);

        float dist_best = 1e100;
        for(int m = 0; m < t->k; m++) // loop for k centers, decides the nearest one
        {
            // print process cluster number per 100.
            //if(m%1000 == 0)
            //printf("iterator times:%d thread:%d centors:%d process %d clusters dist.\n", t->iterator, tid,t->k, m);
            float dist = Util::dist_l2_sq(t->centers + m*t->d, t->data + i*t->d, t->d);
            if(dist < dist_best)
            {
                dist_best = dist;
                t->assignment[i] = m;
            }
        }
        t->cost[i] = dist_best;
        for(int x=0; x < t->d; x++)
        {
            *(t->residual+i*t->d+x) = *(t->data+i*t->d+x) - *(t->centers + t->assignment[i]*t->d+x);
        }
    }


    
private:
    /**
    One-time clustering with kmeans
    @brief conduct one-time k-means clustering
    @param verbose verbosity level
    @param data data to be clustered. there should be 'n*d' floating points in data
    @param centers pointer to centers to be generated. size (k*d) should be pre-allocated
    @param n number of items
    @param d dimension of item
    @param k number of centroids
    @param iter number of iterations
    @param cost total cost of this run
    @param nt number of core to use for computing
    @return void
    */
    static void kmeans_once(const float* data, float* centers, int n, int d, int k, int iter, float& cost, int nt)
    {
        //int* initial_center_idx = init_rand(n, k);
        int* initial_center_idx = init_kpp(n, k, d, data);

        cout << "init centers" << endl;
        // init centers with random generated index
        for(int i = 0; i < k; i++)
            for(int j = 0; j<d; j++)
                centers[i*d + j] = data[initial_center_idx[i]*d + j];

        delete[] initial_center_idx;


        int* ownership = new int[n];
        float* cost_tmp = new float[n];

        //printf("debug:n= %d, nt=%d\n",n,nt);
        // begin iteration
        for(int iteration = 0; iteration < iter; iteration ++)
        {
            cost = 0;
            // re-assignment of center_id to each data
            /** assign points to clusters with multi-threading */
            nn_par ti = {centers, data, k, d, ownership, cost_tmp, iteration};
            MultiThd::compute_tasks(n, nt, &nn_task, &ti);
            for(int j = 0; j < n; j ++)
                cost += cost_tmp[j];

            printf("Iter: %d   Cost: %.4f\n", iteration, cost);

            // re-calc centers
            memset(centers, 0, sizeof(float) * d * k);
            for(int i = 0; i<k; i++) // each center
            {
                int cnt = 0;
                //if(i%1000 == 0)
                //    printf("iter:%d center:%d\n",iteration, i);
                for(int j = 0; j<n; j++) // each point
                {
                    if(ownership[j] == i) // assigned to center-i
                    {
                        cnt ++;
                        for(int p = 0; p<d; p++)
                        {
                            centers[i*d + p] += data[j*d + p];
                        }
                    }
                }

                for(int j = 0; j<d; j++)
                {
                    centers[i*d + j] /= cnt;
                }
            }

        }// end of iteration

        //delete[] cost_tmp;
        //delete[] ownership;
    }

    
    /**
    random number generator [pure random]
    @brief generate k random number from [0, n-1]
    @param n indicates the range to be [0, n-1]
    @param k number of rands to return
    @return pointer to k random numbers
    */
    static int* init_rand(int n, int k)
    {
        int* perm = Util::rand_perm(n, time(NULL));
        int* ret = new int[k];

        // return first k numbers after permutation
        memcpy(ret, perm, sizeof(int)*k);

        delete[] perm;
        return ret;
    }

    /**
    random number generator with kmeans ++
    @brief return k seeds using k-means ++
    @remark for more information, check http://en.wikipedia.org/wiki/K-means%2B%2B.
    */
    static int* init_kpp (int n, int k, int d, const float * data)
    {
        int* sel_id = new int[k];

        float * dist_nn = new float[n]; // keeping shortest distance from each point to selected centers
        float * dist_tmp = new float[n];
        for(int i = 0; i<n; i++)
        {
            dist_nn[i] = 1e100;
        }

        // init the first center in uniformly random
		srand(time(NULL));
        sel_id[0] = rand() % n;

        // init rest of the centers propotional to distance to seleced centers
        for (int i = 1 ; i < k ; i++)
        {
            int j;
            for (j = 0 ; j < n ; j++)
            {
                dist_tmp[j] = Util::dist_l2_sq(&data[j*d], &data[sel_id[i-1]*d], d); // get the dist from data[j] and last assigned center
                dist_nn[j] = std::min(dist_tmp[j], dist_nn[j]);
            }

            // normalize with l1_norm so as to act like proportional probability
            memcpy (dist_tmp, dist_nn, n * sizeof(float));
            float l1_norm = 0.0f;
            for(j = 0; j<n; j++)
                l1_norm += dist_tmp[j];

            for(j = 0; j<n; j++)
                dist_tmp[j] /= l1_norm;

            srand(time(NULL));
            double rd = rand()/(RAND_MAX + 1.0);

            for (j = 0 ; j < n - 1 ; j++)
            {
                rd -= dist_tmp[j];
                if (rd < 0)
                    break;
            }

            sel_id[i] = j;
        }

        delete[] dist_nn;
        delete[] dist_tmp;

        return sel_id;
    }

    /**
    help function of kmeans_once
    @brief assignment nearest center id to i-th point.
    it is the single computation for multi-threading computation.
    @param arg type of nn_par*, contains the input and output needed for computation
    @param tid thread id
    @param i the index of this computation task against all tasks
    @param mutex mutex for updating shared data
    @return void
    */
    static void nn_task(void* arg, int tid, int i, pthread_mutex_t& mutex)
    {
        nn_par* t = (nn_par*) arg;
        //printf("tid: %d, i: %d\n", tid, i);

        float dist_best = 1e100;
        for(int m = 0; m < t->k; m++) // loop for k centers, decides the nearest one
        {
            // print process cluster number per 100.
            //if(m%1000 == 0)
            //printf("iterator times:%d thread:%d centors:%d process %d clusters dist.\n", t->iterator, tid,t->k, m);
            float dist = Util::dist_l2_sq(t->centers + m*t->d, t->data + i*t->d, t->d);
            if(dist < dist_best)
            {
                dist_best = dist;
                t->assignment[i] = m;
            }
        }
        t->cost[i] = dist_best;
    }



};

#endif // CLUSTERING_H_INCLUDED
