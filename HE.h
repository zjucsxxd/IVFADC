/**
Check
Herve Jegou, Matthijs Douze and Cordelia Schmid.
Hamming Embedding and Weak geometry consistency for large scale image search.
Proceedings of the 10th European conference on Computer vision, October, 2008.
for details.
@file HE.h
@brief this file defines the utilities used for Hamming Embedding.
@author wzhang
@date May 7th, 2012
*/

#ifndef HE_H_INCLUDED
#define HE_H_INCLUDED


#include <string>
#include <cmath>
#include <cstdio>
#include <cassert>

#include "MultiThd.h"
#include "util.h"
#include "Vocab.h"

using std::string;
using std::vector;



/// class for Hamming Embedding training and Hamming signature generation
class HE
{
public:
	///keeps the median of hamming median
    float* he_median;
    ///the projection matrix: he_len x feat_len
    float* pmat;
    /// hamming distance weighting. size: he_len + 1
    float* weights;
    /// hamming distance threshold
    int ht;
    /// length of hamming code
    int he_len;
    /// length of original feature
    int feat_len;
    /// size of vocab
    int voc_size;


	/**
	@brief constructor
	@param he_len_assign length of hamming signature
	@param feat_len_assign length of feature vector
	@param pmat_file file path of random projection matrix
	@param vocab_size vocabulary size
	@param thres threshold for hamming distance
	*/
    HE (int he_len_assign, int feat_len_assign, string pmat_file, int vocab_size, int thres);

	/**
	@brief destructor
	*/
    ~HE();


    //------------------------------------------------------------------------------------

	/**
	@brief load Hamming median to memory
	@param file file name of median file
	@return void
	@remark you do not have to train hamming median each time doing search. This function gives
	the possibility of loading existing median which is trained before.
	*/
    void loadMedian(string file);


	/**
	@brief function to train Hamming median values
	@param vocab pointer to a Vocab (vocabulary)
	@param mtrx matrix file (containing rows of features) to train median
	@param nt number of cores (threads) to use
	@remark when the vocabulary is relatively small, we train median values. When the vocabulary and mtrx is
	too large to fit in the memory, we use mean to approximate median for hamming trainning.
	*/
    void train (Vocab* vocab, string mtrx, int nt);




    /**
    @brief generate the hamming code for 'vec' (size: feat_len)
    @param vec feat_len[m:end] is used
    @param idx the quantized index of vec against vocabulary
    @param m skip the first m columns of vec since it's [x y scale ori ...]
    @return the hamming signature of vec
    */
    unsigned int genCode(float* vec, int idx, int m);


private:

	/**
	@brief generate the assignment of the points
	@param voc pointer to vocabulary
	@param mtrx matrix file
	@param nt number of threads to use
	@return the quantization results
	*/
    int* quantizeTrainingMtrx(Vocab* voc, string mtrx, int nt);

	/**
    @brief single computation for quantization of multi-threading
    */
    static void quanti_task(void* arg, int tid, int i, pthread_mutex_t& mutex);


    // ----------------------------------------------------------------------------

	/**
	@brief helper function of genMedian for multi-threading
	*/
    static void train_mean_task(void* arg, int tid, int i, pthread_mutex_t& mutex);

	/**
	@brief generate hamming median file
	@param mtrx matrix file with rows of features
	@param output path for generated median file
	@param voc pointer to vocabulary
	@param nt number of threads to use
	@return void
	@remark this function actually trains Hamming mean because of memory and computation bottleneck.
	*/
    void genMedian(string mtrx, string output, Vocab* voc, int nt);

    //--------------------------------------------------------------------------------------------
	/**
	@brief generate hamming median file totally relying on main memory
	@param m_ori matrix file with rows of features
	@param assignment pointer to the assignment (to visual words) of each feature in m_ori
	@param output the path of generated median file
	@param voc pointer to vocabulary used
	@return void
	@remark this function is called only when main memory is sufficient for m_ori. Of course this is faster
	than genMedian
	*/
    void genMedian_speedup(string m_ori, int* assignment, string output, const Vocab* voc);

};

#endif // HE_H_INCLUDED
