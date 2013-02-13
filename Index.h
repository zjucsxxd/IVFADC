/**
@file Index.h
@brief this file defines the classes and structs to index images.
@author wzhang
@date May 7th 2012
*/
#ifndef INDEX_H_INCLUDED
#define INDEX_H_INCLUDED

#include <string>
#include <cstdio>
#include <vector>
#include <cassert>
#include <set>
#include <iostream>

#include "Vocab.h"
#include "IO.h"
#include "entry.h"
#include "PQCluster.h"
#include "MultiThd.h"

using std::string;
using std::vector;
using std::set;


/// arguments used to index files with multi-threading
struct index_args
{
    int dim;
    float* feature;
	/// name list of feature file to index
    vector<string*>& namelist;   
    /// vocabulary used to quantize feature
    Vocab* voc;                 
    PQCluster* rvoc;
    /// file to write the index
    FILE* fout_idx;             
    /// file to write the name list
    FILE* fout_nl;
    int     w;
};


/// class used to index files
class Index
{
public:

    /**
    @brief index the files in directory of 'feat_dir' using vocabulary voc
    @param voc pointer to vocab using
    @param feat_dir directory name
    @param file_extn this function will index all files under 'feat_dir' which ends with 'file_extn'
    @param idx_dir output location of index files
    @param nt number of cpus to use
    @return void
    */

    static void indexFiles(Vocab* voc, PQCluster* rvoc, string feat_dir, string file_extn, string idx_dir, int nt, int w);
private:

	/**
	@brief helper function for indexFiles. Index one file.
	*/
    static void index_task(void* args, int tid, int i, pthread_mutex_t& mutex);

	/**
	@brief generate aux files for the index
	*/
    static void gen_idx_sz_file(string idx_file, string idx_sz, int voc_size);
};
#endif // INDEX_H_INCLUDED
