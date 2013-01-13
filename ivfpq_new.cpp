#include "ivfpq_new.h"
#include "MultiThd.h"
#include "IO.h"
#include "util.h"
#include "Clustering.h"
#include <iostream>

ivfpq_new::ivfpq_new( Config& con_l )
{
    coarsek = con_l.coarsek;
    d = con_l.dim;
    iter = con_l.iter;
    attempts = con_l.attempts;
    nt = con_l.nt;
    limit_point = con_l.T;
    k = con_l.bf;
    l = con_l.num_layer;
    dataId = con_l.dataId;
    train_desc = con_l.train_desc;
}

/**
@brief prepare the directories for vocabulary generation
@param working_dir the direcotory to be prepared
*/
void ivfpq_new::init(std::string working_dir)
{
    IO::mkdir(working_dir);
    std::cout << working_dir + "matrix/" << std::endl;
    IO::mkdir(working_dir + "matrix/");
    //std::cout << working_dir + "matrix_residual/" << std::endl;
    //IO::mkdir(working_dir + "matrix_residual/");
    std::cout << working_dir + "vk_words/" << std::endl;
    IO::mkdir(working_dir + "vk_words/");
    std::cout << working_dir + "vk_words_residual/" << std::endl;
    IO::mkdir(working_dir + "vk_words_residual/");

    // empty the folder of matrix and vk_words
    vector<string> filelist = IO::getFileList(working_dir + "matrix/", "M.l", 0, 0);
    for(unsigned int i = 0; i < filelist.size(); i++)
        IO::rm(filelist[i]);
    filelist.clear();

    filelist = IO::getFileList(working_dir + "vk_words/", "vocab", 0, 0);
    for(unsigned int i = 0; i< filelist.size(); i++)
        IO::rm(filelist[i]);
    filelist.clear();

    filelist = IO::getFileList(working_dir + "vk_words_residual/", "vocab", 0, 0);
    for(unsigned int i = 0; i< filelist.size(); i++)
        IO::rm(filelist[i]);
    filelist.clear();

}


void ivfpq_new::train_coarse_codebook()
{
        string working_dir = "./"+dataId+".out/";
        std::cout << working_dir << std::endl;
        init(working_dir);

        // sampling feature matrix
        int ptsPerCenter = 50; // ptsPerCenter * numOfCenters points will be sampled for generating the vocab
        string mtrx = working_dir + "matrix/M.l0.n0";
        IO::genMtrx (train_desc, mtrx, ptsPerCenter*max(k, coarsek) );

        // kmeans on M.l0.n0 to get k centers
        int  n = 0, d = 0; // row & col of the matrix file
        float* data = IO::loadFMat(working_dir + "matrix/M.l0.n0", n, d, limit_point);

        //coa_centroids  = new float(d * coarsek);
        
        Vocab* voc = new Vocab(coarsek, 1, d); // new the location to keep the centers
        kmeans_par k_par = {data, n, d, coarsek, iter, attempts, nt, voc->vec};
        Clustering::kmeans(&k_par);
        coa_centroids = voc->vec;
        voc->write2Disk(working_dir + "vk_words/");
        delete[] data;
}

void ivfpq_new::train_residual_codebook()
{
    std::cout << "train residual codebook" << std::endl;
    int  n = 0, d = 0; // row & col of the matrix file
    string working_dir = "./"+dataId+".out/";
    //IO::genMtrx (train_desc, mtrx, ptsPerCenter*ROUND(pow(k, l)));
    float* data = IO::loadFMat(working_dir + "matrix/M.l0.n0", n, d, con.T);

    // do knn, assign the descriptors to cluster

    int* ownership = new int[n];
    float* cost_tmp = new float[n];
    float* residual = new float[n*d];
    nn_par2 ti = {coa_centroids, data, k, d, ownership, cost_tmp, iter, residual};
    MultiThd::compute_tasks(n, nt, &Clustering::nn_task2, &ti);
    delete[] data;

    // run k-means hierarchically to get hie-vocab. see vocab.h for storage of vocabulary
    std::cout << "residual k:" << k << std::endl;
    Vocab* voc = new Vocab(k, 1, d); // new the location to keep the centers

    kmeans_par k_par = {residual, n, d, k, iter, attempts, nt, voc->vec};
    Clustering::kmeans(&k_par);
    /*
    for(int i = 0; i < l; i ++) // each layer [0, l-1], except layer l.
    {
        for(unsigned int j = 0; j < ROUND(pow(k, i)); j++) // each center on layer-i
        {
            kmeans_par k_par = {residual, n, d, k, iter, attempts, nt, voc->vec + voc->sp[i+1] + j*k*d};
            Clustering::kmeans(&k_par);
        }
    }
    */
    voc->write2Disk(working_dir + "vk_words_residual/");
    delete[] residual;
}
