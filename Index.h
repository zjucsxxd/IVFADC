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

#include "Vocab.h"
#include "IO.h"
#include "entry.h"
#include "HE.h"

#include "MultiThd.h"

using std::string;
using std::vector;
using std::set;


/// arguments used to index files with multi-threading
struct index_args
{
	/// name list of feature file to index
    vector<string>& namelist;   
    /// vocabulary used to quantize feature
    Vocab* voc;                 
    Vocab* rvoc;
    /// file to write the index
    FILE* fout_idx;             
    /// file to write the name list
    FILE* fout_nl;              
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
    static void indexFiles(Vocab* voc, Vocab* rvoc, string feat_dir, string file_extn, string idx_dir, int nt)
    {
        IO::mkdir(idx_dir);
        string idx_file = idx_dir + "idx";
        string nl_file  = idx_dir + "nl";
        string idx_sz   = idx_dir + "voc_sz";


        printf("Indexing files: \"%s\"\n", feat_dir.c_str());
        FILE* fout_idx = fopen(idx_file.c_str(), "ab");
        FILE* fout_nl  = fopen(nl_file.c_str(), "a");
        assert(fout_idx && fout_nl);


        vector<string> filelist = IO::getFileList(feat_dir, file_extn, 1, 1);
        int tot_ims = filelist.size();
        fwrite(&tot_ims, sizeof(int), 1, fout_idx);
        fprintf(fout_nl, "%d\n", tot_ims);


        index_args args = {filelist, voc, rvoc, fout_idx, fout_nl};
        MultiThd::compute_tasks(tot_ims, nt, &index_task, &args);
        printf("\n");

        fclose(fout_idx);
        fclose(fout_nl);
        filelist.clear();

        gen_idx_sz_file(idx_file, idx_sz, voc->num_leaf);
    }

private:

	/**
	@brief helper function for indexFiles. Index one file.
	*/
    static void index_task(void* args, int tid, int i, pthread_mutex_t& mutex)
    {
        index_args* arguments = (index_args*) args;
        string filename = arguments->namelist[i];
        string shortname = Util::parseFileName(filename);

        int n, m, d;
        float* feature = IO::readFeatFile(filename, n, m, d);

        int* quanti_result = new int[n];
        int* residual_result = new int[n];
        arguments->voc->quantize2leaf(feature, quanti_result, n, m);
        
        float* residual_vec = new float[n*d];
        for(int j=0; j < n; j++)
        {    
            for(int x=0;x<d;x++)
            {
                residual_vec[j*d+x] =*(feature+j*d+x) - *(arguments->voc->vec+quanti_result[j]*d+x); 
            }
        }

        arguments->rvoc->quantize2leaf(residual_vec, residual_result, n, m);
        
        Entry* entrylist = new Entry[n];
        for(int j = 0; j < n; j++)
        {
            entrylist[j].set(quanti_result[j], residual_result[j]);
        }

        /// write sync
        pthread_mutex_lock (&mutex);
        fwrite(&n, sizeof(int), 1, arguments->fout_idx);
        for(int j= 0; j < n; j++)
            entrylist[j].write(arguments->fout_idx);


        fprintf(arguments->fout_nl, "%s\n", shortname.c_str());
        pthread_mutex_unlock(&mutex);
        /// end of write sync

        delete[] feature;
        delete[] entrylist;
        delete[] quanti_result;

        if ( (i+1) % 10 == 0)
        {
            pthread_mutex_lock (&mutex);
            printf("\r%d", i+1); fflush(stdout);
            pthread_mutex_unlock(&mutex);
        }
    }

	/**
	@brief generate aux files for the index
	*/
    static void gen_idx_sz_file(string idx_file, string idx_sz, int voc_size)
    {
        // reconstruct the index using stl and output num of entries in each word
        FILE* fin_idx = fopen(idx_file.c_str(), "rb");
        IO::chkFileErr(fin_idx, idx_file);

        vector< vector<Entry> > index(voc_size);


        int num_entries, tot_ims;
        assert( 1 == fread(&tot_ims, sizeof(int), 1, fin_idx) );

        Entry* entry = new Entry();
        for(int i = 0; i < tot_ims; i++) // i-th image
        {
            assert( 1 == fread(&num_entries, sizeof(int), 1, fin_idx) );
            for(int j = 0; j < num_entries; j++) // j-th entry
            {
                entry->read(fin_idx);
                int word_id = entry->id;

                entry->id = i;

                index[word_id].push_back(*entry); // push_back will make a copy of entry
            }
        }
        fclose(fin_idx);


        int* sz = new int[voc_size];
        for(int i = 0; i < voc_size; i++)
            sz[i] = index[i].size();

        IO::writeMat(sz, voc_size, 1, idx_sz);

        for(unsigned int i = 0; i < index.size(); i++)
            index[i].clear();
        index.clear();

        delete[] sz;
    }

};
#endif // INDEX_H_INCLUDED
