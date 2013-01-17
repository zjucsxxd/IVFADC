/**
@file SearchEngine.h
@brief this files defines methods to conduct efficient online search.
@author wzhang
@date May 7th, 2012
*/
#ifndef SEARCHENGINE_H_INCLUDED
#define SEARCHENGINE_H_INCLUDED

#include <cstring>
#include <vector>
#include <set>
#include <algorithm>

#include "Vocab.h"
#include "IO.h"
#include "result.h"


using std::string;
using std::vector;


/// this class defines the online search procedure
class SearchEngine
{
public:

	/// pointer to the vocabulary using
    Vocab* voc;
    Vocab* rvoc;

	/// keeps different index directories. This implementaion can load multiple indexes when searching.
    vector<string> idxList;
    /// main index. voc_size x num_entries_of_word_i
    Entry** index;
    /// keeping the namelist of each image
    vector<string> im_db;
    /// voc_size x 1. keeps # of entries in each word
    int* num_entries;
    /// total number of images indexed
    int tot_ims;
    /// voc_size x 1
    float* idf;
    /// im_size x 1
    float* norm;

	/// init variables
    SearchEngine(Vocab* vocab, Vocab* rvocab);

	///deletes things newed
    ~SearchEngine();

    /**
    @brief load all indexes under 'dir'
    @param dir path for the directory containing all indexes
    @return void
    @remark all indexes inside dir will be loaded for reference image.
    */
    void loadIndexes(string dir);

	/**
	@brief search all feature files (images) inside a directory
	@param dir directory of features to search
	@param out_file file name to hold the result
	@param topk top k results will be written to out_file
	@return void
	@remark this function will search images under dir one by one
	*/
    void search_dir(string dir, string out_file, int topk);


private:

	/// size of vocabulary using
    int size_voc;

	/**
	@brief load one index located under directory 'idx_dir'
	@param dir specific directory which contains one index
	@remark the detailed index is organized in a way
	dir/idx, dir/aux and dir/nl.
	*/
    void loadSingleIndex(string dir);

	/**
    @brief init idf and norms
    */
    void update();

    /// helper function of update. this function init idf[i]
    static void idf_task(void* args, int tid, int i, pthread_mutex_t& mutex);

    /// helper function of update. this function init nrom[i]
    static void norm_task(void* args, int tid, int i, pthread_mutex_t& mutex);


	/**
	@brief calculate norm for an image
	@param entrylist the quantized entrylist of the image
	@param n number of entries
	@return the norm of the entrylist
	*/
    float getNorm(Entry* entrylist, int n);

	/**
	@brief get number of ones in the binary form of an interger
	@param x the interger to evaluate
	@return number of ones in binary format of x
	*/
    inline int num_of_ones(int x);
};

#endif // SEARCHENGINE_H_INCLUDED
