/**
@file HE.cpp
@brief this file implements the HE.h
@author wzhang
@date May 7th, 2012
*/

#include <string>
#include <cmath>
#include <cstdio>
#include <cassert>

#include "MultiThd.h"
#include "util.h"
#include "HE.h"
#include "Vocab.h"

using std::string;
using std::vector;

/// structure of arguments used when quantizing features using multi-threading
struct quanti_args
{
    // in
    /// pointer to Vocab
    Vocab* voc;
    /// file pointer to read input
    FILE* fin;
    /// total number of tasks
    int total;

    /// process control, indicating the number of items dealing with
    int idx;

    // out
    /// the quantization result indicating the final assignment of each feature
    int* assignment;
};

/// structure of arguments used when training Hamming medians (for large vocabulary, we use mean to approximate median to save memory/time)
struct train_mean_args
{
    // in
    /// input file pointer
    FILE* fin;
    /// pointer to Vocab
    Vocab* voc;
    /// length of Hamming signature
    int he_len;
    /// pointer to projection matrix
    float* pmat;
    /// total number of tasks
    int total;

    // out
    /// counts the number of points for each visual word
    int* count;
    /// pointer to Hamming medians
    float* median;
};




	/**
	@brief constructor
	@param he_len_assign length of hamming signature
	@param feat_len_assign length of feature vector
	@param pmat_file file path of random projection matrix
	@param vocab_size vocabulary size
	@param thres threshold for hamming distance
	*/
    HE::HE (int he_len_assign, int feat_len_assign, string pmat_file, int vocab_size, int thres)
    {
        he_len = he_len_assign;
        feat_len = feat_len_assign;
        voc_size = vocab_size;
        he_median = NULL;

        int row, col;
        pmat = IO::loadFMat(pmat_file, row, col, -1);
        assert(row == he_len && col == feat_len);

        ht = thres;

        // init weights according to the Inria paper
        // "Improving bag-of-features for large scale image search" by Jegou Herve, etc.
        weights = new float[he_len + 1];
        float base = 0.0;
        for(int i = 0; i < he_len + 1; i++)
        {
            weights[i] = base + Util::combination(he_len, i);
            base = weights[i];
        }

        float norm = pow(2, he_len);
        for(int i = 0; i < he_len + 1; i++)
            weights[i] = -log2(weights[i]/norm);
    }

	/**
	@brief destructor
	*/
    HE::~HE()
    {
        delete[] he_median;
        delete[] pmat;
        delete[] weights;
    }


    //------------------------------------------------------------------------------------

	/**
	@brief load Hamming median to memory
	@param file file name of median file
	@return void
	@remark you do not have to train hamming median each time doing search. This function gives
	the possibility of loading existing median which is trained before.
	*/
    void HE::loadMedian(string file)
    {
        int row, col;
        he_median = IO::loadFMat(file, row, col, -1);
        assert(he_len == col && voc_size == row);
    }


	/**
	@brief function to train Hamming median values
	@param vocab pointer to a Vocab (vocabulary)
	@param mtrx matrix file (containing rows of features) to train median
	@param nt number of cores (threads) to use
	@remark when the vocabulary is relatively small, we train median values. When the vocabulary and mtrx is
	too large to fit in the memory, we use mean to approximate median for hamming trainning.
	*/
    void HE::train (Vocab* vocab, string mtrx, int nt)
    {
        printf("Hamming Training...\n");
        string m_original = mtrx;
        string medianFile = mtrx + ".median";

        // use speedup version when vocabulary is small, say 10k-
        //int mem_size = Util::getTotalSystemMemory();
        //int mem_to_use = vocab->num_leaf * 80.0 * vocab->d * 4 / 1024 / 1024;
        //if (mem_size < mem_to_use)
        //{
        //    printf("Limited memory (%d < %d MB). Will train mean instread.\n", mem_size, mem_to_use);
        //    genMedian(m_original, medianFile, vocab, nt);
        //}
        //else
        //{
            int* assignment = quantizeTrainingMtrx(vocab, m_original, nt);
            genMedian_speedup(m_original, assignment, medianFile, vocab);
            delete[] assignment;
        //}
    }




    /**
    @brief generate the hamming code for 'vec' (size: feat_len)
    @param vec feat_len[m:end] is used
    @param idx the quantized index of vec against vocabulary
    @param m skip the first m columns of vec since it's [x y scale ori ...]
    @return the hamming signature of vec
    */
    unsigned int HE::genCode(float* vec, int idx, int m)
    {
        float* vec_projected = new float[he_len];
        Util::project(pmat, he_len, feat_len, vec + m, vec_projected);

        unsigned int sig = 0;
        for(int j = 0; j < he_len; j++)
        {
            if(vec_projected[j] > he_median[idx*he_len + j])
                sig = sig | (1<<j);
        }

        delete[] vec_projected;

        return sig;
    }



	/**
	@brief generate the assignment of the points
	@param voc pointer to vocabulary
	@param mtrx matrix file
	@param nt number of threads to use
	@return the quantization results
	*/
    int* HE::quantizeTrainingMtrx(Vocab* voc, string mtrx, int nt)
    {
        printf("Quantizing the Hamming training matrix...\n");
        FILE* fin = fopen(mtrx.c_str(), "rb");
        IO::chkFileErr(fin, mtrx);

        int m, dim;
        assert( 1 == fread(&m, sizeof(int), 1, fin) );
        assert( 1 == fread(&dim, sizeof(int), 1, fin) );

        int* assignment = new int[m];

        quanti_args args = {voc, fin, m, 0, assignment};
        MultiThd::compute_tasks(m, nt, &quanti_task, &args);

        printf("\n"); fflush(stdout);

        fclose(fin);

        return assignment;
    }

	/**
    @brief single computation for quantization of multi-threading
    */
    void HE::quanti_task(void* arg, int tid, int i, pthread_mutex_t& mutex)
    {
        quanti_args* t = (quanti_args*) arg;

        int leaf_id;
        float* vec = new float[t->voc->d];

        pthread_mutex_lock(&mutex);
        assert( t->voc->d == (int)fread(vec, sizeof(float), t->voc->d, t->fin) );
        int index = t->idx ++;
        pthread_mutex_unlock(&mutex);

        t->voc->quantize2leaf(vec, &leaf_id, 1, 0);
        t->assignment[index] = leaf_id;

        delete[] vec;

        pthread_mutex_lock(&mutex);
        printf("\r%.2f%%", (index + 1.0)/t->total * 100); fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }


    // ----------------------------------------------------------------------------

	/**
	@brief helper function of genMedian for multi-threading
	*/
    void HE::train_mean_task(void* arg, int tid, int i, pthread_mutex_t& mutex)
    {
        train_mean_args* t = (train_mean_args*) arg;

        int leaf_id;

        float* vec = new float[t->voc->d];
        float* vec_proj = new float[t->he_len];

        pthread_mutex_lock(&mutex);
        assert( t->voc->d == (int)fread(vec, sizeof(float), t->voc->d, t->fin) );
        pthread_mutex_unlock(&mutex);

        t->voc->quantize2leaf(vec, &leaf_id, 1, 0);
        Util::project(t->pmat, t->he_len, t->voc->d, vec, vec_proj);

        pthread_mutex_lock(&mutex);
        // sum up the projected vector
        for(int m = 0; m < t->he_len; m++)
            t->median[leaf_id * t->he_len + m] += vec_proj[m];
        // update count
        t->count[leaf_id] ++;
        pthread_mutex_unlock(&mutex);

        delete[] vec;
        delete[] vec_proj;
        pthread_mutex_lock(&mutex);
        printf("\r%.2f%%", (i + 1.0)/t->total * 100);
        pthread_mutex_unlock(&mutex);
    }

	/**
	@brief generate hamming median file
	@param mtrx matrix file with rows of features
	@param output path for generated median file
	@param voc pointer to vocabulary
	@param nt number of threads to use
	@return void
	@remark this function actually trains Hamming mean because of memory and computation bottleneck.
	*/
    void HE::genMedian(string mtrx, string output, Vocab* voc, int nt)
    {
        float largeVal = 1e30;

        FILE* fin = fopen(mtrx.c_str(), "rb");
        IO::chkFileErr(fin, mtrx);

        int row, col;
        assert( 1 == fread(&row, sizeof(int), 1, fin) );
        assert( 1 == fread(&col, sizeof(int), 1, fin) );


        int size_voc = voc->num_leaf;
        int* count = new int[size_voc];
        memset(count, 0, sizeof(int)*size_voc);

        he_median = new float[size_voc*he_len];
        memset(he_median, 0, sizeof(float)*size_voc*he_len);


        train_mean_args args = {fin, voc, he_len, pmat, row, count, he_median};
        MultiThd::compute_tasks(row, nt, &train_mean_task, &args);
        fclose(fin);


        int failed = 0;
        for(int i = 0; i< size_voc; i++)
        {
            for(int j = 0; j<he_len; j++)
            {
                if (count[i] == 0)
                {
                    he_median[i*he_len +j] = largeVal;
                    failed ++;
                }
                else
                    he_median[i*he_len + j] /= count[i];
            }
        }

        delete[] count;

        printf("\nCoverage: %.1f%%\n", (size_voc - failed/he_len)*100.0/size_voc);

        IO::writeMat(he_median, size_voc, he_len, output);
    }

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
    void HE::genMedian_speedup(string m_ori, int* assignment, string output, const Vocab* voc)
    {
        float largeVal = 1e30;

        //write the hamming median file
        // this version runs much faster but comsumes larger memory, which may not work out for very large codebook
        FILE* fout = fopen(output.c_str(), "wb");
        IO::chkFileErr(fout, output);

        int size_voc = voc->num_leaf;
        fwrite(&size_voc, sizeof(int), 1, fout);
        fwrite(&he_len, sizeof(int), 1, fout);


        printf("Generating hamming median file:\n");
        FILE* fin_mtrx = fopen(m_ori.c_str(), "rb");
        IO::chkFileErr(fin_mtrx, m_ori);

        int n, dim;
        assert( 1 == fread(&n, sizeof(int), 1, fin_mtrx) );
        assert( 1 == fread(&dim, sizeof(int), 1, fin_mtrx) );
        assert(dim == voc->d);

        // featSet: size 'voc->num_leaf' vector.
        // featSet[each_voc_entry][he_len_entry][each_point];
        vector< vector<vector<float> > > featSet(size_voc);
        for( unsigned int i = 0; i<featSet.size(); i++)
            featSet[i].resize(he_len);


        // read all data to featSet
        float* vec = new float[voc->d];
        float* vec_projected = new float[he_len];
        for(int i = 0; i<n; i++)
        {
            int word_id = assignment[i];
            printf("\r%.2f%%", 100*(i+1.0)/n);

            assert( dim == (int)fread(vec, sizeof(float), dim, fin_mtrx) );

            Util::project(pmat, he_len, dim, vec, vec_projected);
            for(int k = 0; k < he_len; k++)
                featSet[word_id][k].push_back(vec_projected[k]);
        }
        printf("\n");
        fclose(fin_mtrx);

        delete[] vec;
        delete[] vec_projected;


        int failed = 0;
        for(int i = 0; i < size_voc; i++)
        {
            printf("\r%d", i+1);
            fflush(stdout);

            for(int j = 0; j<he_len; j++)
            {
                sort(featSet[i][j].begin(), featSet[i][j].end());
                int len = featSet[i][j].size();
                if (len == 0)
                {
                    failed ++;
                    fwrite(&largeVal, sizeof(float), 1, fout);
                }
                else if (len % 2 == 0)
                {
                    float median = (featSet[i][j][len/2] + featSet[i][j][len/2-1])/2.0;
                    fwrite(&median, sizeof(float), 1, fout);
                }
                else
                {
                    float median = featSet[i][j][len/2];
                    fwrite(&median, sizeof(float), 1, fout);
                }
            }
        }
        fclose(fout);
        printf("\nCoverage: %.1f%%\n", (size_voc - failed/he_len)*100.0/size_voc);


        //clean
        for(unsigned int i = 0; i<featSet.size(); i++)
        {
            for(int j = 0; j < HE::he_len; j++)
                featSet[i][j].clear();
            featSet[i].clear();
        }
        featSet.clear();
    }
