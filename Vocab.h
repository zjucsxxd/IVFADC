/**
@file Vocab.h
@brief This file defines the vocabulary structure and operations used in Bow based image retrieval.
@author wzhang
@date May 8, 2012
*/

#ifndef HVOCAB_H_INCLUDED
#define HVOCAB_H_INCLUDED

#include <cstring>

#include "IO.h"
#include "entry.h"

using std::string;

class HE;


/// class for hierarchical visual words (vocabulary) 
class Vocab
{

public:

	/**
	pointer to vocabulary
	structure of storage (flat): k = 3, l = 2
	format: layer_id,number
	                  root (gabage, not used)
	     1,0           1,1            1,2
	  2,0 2,1 2,2  2,3 2,4 2,5    2,6 2,7 2,8
	  
	number of float : k^0*d + k^1*d + k^2*d + ... + k^l*d
	*/ 
    float* vec;
    /// brancking factor 
    int k; 
    /// number of layers
    int l; 
    /// dimension of feature
    int d; 
    /// number of leaf nodes
    int num_leaf; 
    /// starting position of each layer
    int *sp; 

private:
	/// total number of
    int total_len; 

public:

	/** 
	@brief constructor for vocabulary
	@param k_assign brancking factor
	@param l_assign number of layers
	@param dim dimension of feature
	*/
    Vocab(int k_assign, int l_assign, int dim);

    ~Vocab();



    /**
    quantize v (size: (n+m)xd) using this vocabulary. quantization index is kept in out
    @brief quantize a vector to hierarchical structure 
    @param v vector to quantize. size of (n+m) x d
    @param out keeps the hierarchical quantization of v against codebook of vec. size of n x l.
    @param n number of features to quantize
    @param m skip the first d columns of data in v, which is often [x y ori scale]
    @remark d for dimemsion is not passed cuz size(v) = nxd should be consistent with voc->d
    */
    void quantize2hie(float* v, int* out, int n, int m);


    /**
    @brief quantize a set of (n) vector v into the leaf layer
    @param v vector to quantize. size of n x d
    @param out keeps the quantization result. size of n x 1
    @param n number of features to quantize
    @param m skip the first m column of v.
    */
    void quantize2leaf(float* v, int* out, int n, int m);


	/**
	@brief quantize a set of features to leaf with multiple assignment
	@param v vector to quantize. size of n x d
    @param out keeps the quantization result. size of n x 1
    @param n number of features to quantize
    @param m skip the first m column of v.
    @param ma multiple assignments factor
	*/
    void quantize2leaf(float* v, int* out, int n, int m, int ma);


    /**
    @brief quantize a feature file to entry lists
    @param file file path of feature file to quantize
    @param len number of entries for the quantized feature file
    @param nt number of cores to use
    @param ma multiple assignments factor
    @return pointer to a entrylist
    */
    Entry* quantizeFile(float* feat,int& len, int nt, int ma, int d);


    /**
    @brief load the vocabulary 'vec' from disk
    */
    void loadFromDisk(string dir);
    
    /**
    @brief wirte the vocabulary 'vec' to disk
    */
    void write2Disk(string file);

private:

    /**
    @brief quantize single vector 'v' (size: 1 x d) to 'idx'
    @param v vector to quantize size of 1 x d vector
    @param idx quantization result of each layer. size of 1 x l.
    */
    void quantize_once(float* v, int* idx);
};

#endif // HVOCAB_H_INCLUDED
