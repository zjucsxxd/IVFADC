/**
@file Vocab_Gen.h
@brief this file provides utilities for generating the vocabulary
@author wzhang
@date May 8th, 2012
*/

#ifndef VOCAB_GEN_H_INCLUDED
#define VOCAB_GEN_H_INCLUDED

#include <cassert>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "Vocab.h"
#include "util.h"
#include "Clustering.h"
#include "IO.h"
#include "config.h"
#include "timer.h"

extern Config con;

using std::string;
using std::vector;


/// class for vocabulary generation
class Vocab_Gen
{
public:
    /**
    @brief generate vocabulary using feature files in 'dir'
    @param dir directory of feature files
    @param k branching factor of vocab
    @param l number of layers of vocab
    */
    static void genVoc(std::string dir, int k, int l)
    {
        // init the working directory
        //string working_dir = "/home/nebula/mylab/copyTask/search/search/" + con.dataId + "/";
        string working_dir = "./"+con.dataId+".out/";

        std::cout << working_dir << std::endl;
        init(working_dir);


        // sampling feature matrix
        int ptsPerCenter = 50; // ptsPerCenter * numOfCenters points will be sampled for generating the vocab
        string mtrx = working_dir + "matrix/M.l0.n0";
        std::cout << "k:" << k << "\tl:" << l << "\tpow:" << pow(k, l) << "\tround:" << std::endl;
        IO::genMtrx (dir, mtrx, ptsPerCenter*ROUND(pow(k, l)));


        // run k-means hierarchically to get hie-vocab. see vocab.h for storage of vocabulary
        Vocab* voc = new Vocab(k, l, con.dim); // new the location to keep the centers
        int  n = 0, d = 0; // row & col of the matrix file


        for(int i = 0; i < l; i ++) // each layer [0, l-1], except layer l.
        {
            for(unsigned int j = 0; j < ROUND(pow(k, i)); j++) // each center on layer-i
            {
                // kmeans on M.li.nj to get k centers
                float* data = IO::loadFMat(working_dir + "matrix/M.l" + Util::num2str(i) + ".n" + Util::num2str(j), n, d, con.T);
                kmeans_par k_par = {con.verbose, data, n, d, k, con.iter, con.attempts, con.nt, voc->vec + voc->sp[i+1] + j*k*d};
                //printf("\rClustering (layer-%d, num-%d): %e", i, j, Clustering::kmeans(&k_par)); fflush(stdout);
                printf("Clustering (layer-%d, num-%d)\n", i, j);
                Clustering::kmeans(&k_par);
                delete[] data;

                if ( i < l-1 ) // for the last layer, there's no need to spilt it
                    IO::divMatByCenters_MT(working_dir + "matrix/M", i, j, voc->vec + voc->sp[i+1] + j*k*d, k, d, con.nt);

                if ( i != 0 ) // remove the matrix file after using
                    IO::rm(working_dir + "matrix/M.l" + Util::num2str(i) + ".n" + Util::num2str(j));
            }
        }
        printf("\n");


        voc->write2Disk(working_dir + "vk_words/");
        delete voc;
    }

private:

    /**
    @brief prepare the directories for vocabulary generation
    @param working_dir the direcotory to be prepared
    */
    static void init(std::string working_dir)
    {
        IO::mkdir(working_dir);
        std::cout << working_dir + "matrix/" << std::endl;
        IO::mkdir(working_dir + "matrix/");
        std::cout << working_dir + "vk_words/" << std::endl;
        IO::mkdir(working_dir + "vk_words/");

        // empty the folder of matrix and vk_words
        vector<string> filelist = IO::getFileList(working_dir + "matrix/", "M.l", 0, 0);
        for(unsigned int i = 0; i < filelist.size(); i++)
            IO::rm(filelist[i]);
        filelist.clear();

        filelist = IO::getFileList(working_dir + "vk_words/", "vocab", 0, 0);
        for(unsigned int i = 0; i< filelist.size(); i++)
            IO::rm(filelist[i]);
        filelist.clear();
    }
};

#endif // VOCAB_GEN_H_INCLUDED
