/**
@file config.h
@brief This file defines the struct that holds the configuration of instance search
@author wzhang
@date May 7th, 2012
*/

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/*  format of feature file: *.feat
//  n m d
//  row col scale ori d1 d2 d3 d4 ...
//  row col scale ori d1 d2 d3 d4 ...
//  ...  */

#include <string>


using std::string;

/// holds the configurations of the whole project
struct Config
{
	/// which task are you in?
    int             mode;               
    /// level of verbosity
    int             verbose;

	/// specific id for the dataset working on
    string          dataId;             
    /// # of images returned by this retrieval system
    int             num_ret;            

	/// extension of feature file
    string          extn;               
    /// dimension of feature
    int             dim;                

	/// directory of feature files for training vocab
    string          train_desc;         
    /// directory of feature files of reference image
    string          index_desc;         
    /// directory of feature files of query image
    string          query_desc;         

    // clustering
    /// sample factor of percentage of points sampled per feature file
    float           num_per_file;       
    /// number of threads used
    int             nt;                 
    /// branching factor
    int             bf;                 
    /// # of layers
    int             num_layer;          
    /// # of iterations needed for kmeans
    int             iter;               
    /// limit maximal number of sampling for kmeans
    int             T;                  
    /// # of attempts of clustering
    int             attempts;           

	/// # of multiple assignment
    int             ma;                 

	/// length of hamming code
    int             he_len;             
    /// hamming distance threshold
    int             ht;                 
    /// max image size: im_sz x im_sz
    int             im_sz;              
    /// location of projection matrix file
    string          p_mat;              

	/// searching mode
    int             search_mode;        

    // number of subquantizers to be used, m in the paper
    int             nsq;
    // the number of bits per subquantizer
    int             nsqbits;
    /// number of elements to be returned
    int             k;
    // number of cell visited per query
    int             w;
    // number of centroids for the coarse quantizer
    int             coarsek;

    Config() // set default value to all configurations
    {
        k = 10;
        w = 4;
        nsq = 8;
        nsqbits = 8;
        mode = 0;
        verbose = 0;
        dataId = "tmp_id";
        num_ret = 10;

        extn = ".feat";
        dim = 128;

        train_desc = "";
        index_desc = "";
        query_desc = "";

        ma = 4;

        nt = 1;
        attempts = 3;
        iter = 20;
        T = 10000;
        bf = 100;
        num_layer = 2;
        num_per_file = 0.01;

        ht = 12;
        p_mat = "pmat_32.mat";
    }
};



#endif // CONFIG_H_INCLUDED
