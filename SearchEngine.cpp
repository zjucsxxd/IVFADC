/**
@file SearchEngine.cpp
@brief this files implements SearchEngine.h
@author wzhang
@date May 7th, 2012
*/

#include <cstring>
#include <vector>
#include <set>
#include <algorithm>

#include "Vocab.h"
#include "IO.h"
#include "result.h"
#include "SearchEngine.h"


using std::string;
using std::vector;



//#define DEBUG_MODE

/// arguments used when initing the idf value for vocabulary with multi-threading
struct idf_args
{
    // in
    int tot_ims;
    int* num_entries;
    Entry** index;

    // out
    float* idf;
};

/// arguments used when initing the norms for each reference image
struct norm_args
{
    // in
    FILE* fin_idx;
    int processed;
    int size_voc;
    float* idf;
    float* word_hist;

    // out
    float* norm;
};

/// configuration parameter
extern Config con;

/// init variables
SearchEngine::SearchEngine(Vocab* vocab, Vocab* rvocab)
{
    this->voc = vocab;
    this->rvoc = rvocab;

    size_voc = voc->num_leaf;

    num_entries = new int[size_voc];
    memset(num_entries, 0, sizeof(int)*size_voc);

    index = new Entry*[size_voc];
    for(int i = 0; i < size_voc; i++)
        index[i] = NULL;

    tot_ims = 0;
    idf = NULL;
    norm = NULL;
    im_db.clear();
}

///deletes things newed
SearchEngine::~SearchEngine()
{
    for(int i = 0; i < voc->num_leaf; i++)
        delete[] index[i];
    delete[] index;
    delete[] num_entries;
    delete[] idf;
    delete[] norm;
    im_db.clear();
}

/**
@brief load all indexes under 'dir'
@param dir path for the directory containing all indexes
@return void
@remark all indexes inside dir will be loaded for reference image.
*/
void SearchEngine::loadIndexes(string dir)
{
    idxList = IO::getFolders(dir);
    for(unsigned int i = 0; i < idxList.size(); i++) // load all indexes under dir
        loadSingleIndex(idxList[i]);
    // update other fields: idf, norms
    //update();
}

/**
@brief search all feature files (images) inside a directory
@param dir directory of features to search
@param out_file file name to hold the result
@param topk top k results will be written to out_file
@return void
@remark this function will search images under dir one by one
*/
void SearchEngine::search_dir(string dir, string out_file, int topk)
{
    printf("Online search.\n");
    //vector<string> filelist = IO::getFileList(dir, con.extn, 1, 1);

    FILE* fout_result = fopen(out_file.c_str(), "w");
    IO::chkFileErr(fout_result, out_file);

    float* data;
    int n=0, d=0;
    vector<string*> img_db;
    IO::load_vlad(dir, &data, &img_db, &n, &d);
    std::cout << "normalize..." << std::endl;
    for(int i=0; i < n; i++)
    {
        Util::normalize(data+i*d, d);
    }
    std::cout << "normalize finished..." << std::endl;


    // calculate result vectors' residual.
    float* residual =  new float[con.dim];
    vector<Result*> ret;
    for(unsigned int i = 0; i < n; i++)//loop for every query im in directory
    {
        printf("\r%d", i+1); fflush(stdout);
        //string filename = Util::parseFileName(*(img_db[i]));
        string filename = *(img_db[i]);
        fprintf(fout_result, "%s", filename.c_str());

        float* feature = data+i*d;
        int* coarse_res;
        // quantize descriptors to coarse codebook
        // enrtylist is the coarse quantize result list
        int len;
        Entry* entrylist = voc->quantizeFile(feature, len, con.nt, con.ma, con.dim);
        // result entry for coarse search.
        // for each res in entry list, get res vec accroding to inverted index.
        // load query features.
        // iterator vectors in the word cell.
        // word_pos is the start word cell position for query i.
        int j = 0;
        int m = 0;
        int word_pos = j*con.ma;
        for(int g=0; g < (con.ma); g++)
        {
            int coa_word_id = entrylist[word_pos+g].id;

            std::cout << "coa_word:" << coa_word_id << std::endl;
            //Entry* j_entrylist = index[coa_word_id];            
            // coarse_vec is the visual word the query belongs to.
            float* coarse_vec = voc->vec+(d*coa_word_id); 
            for(int x=0;x<con.dim;x++)
            {
                residual[x] = *(feature+j*(m+d)+m+x) - coarse_vec[x];
                //std::cout << residual[x] << " ";
            }
            for(int f=0; f < num_entries[coa_word_id]; f++)
            {
                // read result entries number. 
                Result* tmp = new Result;
                Entry res_tmp = (index[coa_word_id][f]);
                // get result vector's residual vector.
                float* res_residual = rvoc->vec+(d*res_tmp.residual_id);


                Util::normalize(residual, d);
                Util::normalize(res_residual, d);
                tmp->score = Util::dist_l2_sq(residual, res_residual, d);
                //std::cout << "score: " << tmp->score << std::endl;
                tmp->im_id = res_tmp.id; 
                ret.push_back(tmp);

            }
        }

        std::sort(ret.begin(), ret.end(), Result::compare);
        for(vector<Result*>::iterator it = ret.begin(); it != min(ret.end(), ret.begin()+topk); it++)
        {
            fprintf(fout_result, " %s %.4f ", (img_db[(*it)->im_id])->c_str(), (*it)->score);
        }
        fprintf(fout_result, "\n");

        for(unsigned int j = 0; j < ret.size(); j++)
            delete ret[j];
        ret.clear();
        delete[] entrylist;
    } // end for i


    delete[] residual;

    printf("\n");

    fclose(fout_result);
}


/**
@brief load one index located under directory 'idx_dir'
@param dir specific directory which contains one index
@remark the detailed index is organized in a way
dir/idx, dir/aux and dir/nl.
*/
void SearchEngine::loadSingleIndex(string dir)
{
    // load name list
    printf("Loading index of '%s'\n", dir.c_str());
    FILE* fin_nl = fopen((dir + "/nl").c_str(), "r");
    IO::chkFileErr(fin_nl, dir + "/nl");

    // init im_db: name list
    char buffer[100];
    int tot_ims_old = tot_ims, tot_ims_new;
    assert( 1 == fscanf(fin_nl, "%u", &tot_ims_new));
    tot_ims += tot_ims_new;  // update tot_ims
    for(int i = 0; i < tot_ims_new; i++)
    {
        assert( 1 == fscanf(fin_nl, "%s", buffer) );
        string tmpStr(buffer);
        im_db.push_back(tmpStr);
    }
    fclose(fin_nl);



    // load number of entries of this index
    int row, col;
    int* num_entries_new = IO::loadIMat(dir+"/voc_sz", row, col, -1);
    assert(row == size_voc && col == 1);


    // "resize" memory for index
    for(int i = 0; i < size_voc; i++)
    {
        if(num_entries_new[i] == 0)
            continue;

        if(num_entries[i] == 0)
            index[i] = new Entry[num_entries_new[i]];
        else
        {
            Entry* tmp = new Entry[num_entries_new[i] + num_entries[i]];
            std::copy(index[i], index[i] + num_entries[i], tmp);
            delete[] index[i];
            index[i] = tmp;
        }
    }


    // load index
    FILE* fin_idx = fopen((dir + "/idx").c_str(), "rb");
    IO::chkFileErr(fin_idx, dir + "/idx");

    assert( 1 == fread(&tot_ims_new, sizeof(int), 1, fin_idx) );
    assert(tot_ims_new == tot_ims - tot_ims_old);

    Entry* entry = new Entry();
    for(int i = tot_ims_old; i < tot_ims_new + tot_ims_old; i++)
    {
        int n; // number of points on image-i
        assert( 1 == fread(&n, sizeof(int), 1, fin_idx) );
        for(int j = 0; j < n; j++)
        {
            entry->read(fin_idx);

            int word_id = entry->id;
            entry->id = i;

            index[word_id][num_entries[word_id]++] = *entry;

        }
        printf("\r%d", i+1);
    }
    delete entry;
    printf("\n");

    delete[] num_entries_new;

    fclose(fin_idx);
}

/**
@brief init idf and norms
*/
void SearchEngine::update()
{
    printf("Initing idfs ... \n");
    idf = new float[size_voc]; // allocate memory

    idf_args arg = {tot_ims, num_entries, index, idf};
    MultiThd::compute_tasks(size_voc, con.nt, &idf_task, &arg);
    printf("\n");


    printf("Initing norms ... \n");
    norm = new float[tot_ims];

    float* word_hist = new float[size_voc * con.nt]; // allocate memory for each thread
    int processed = 0; // counts the processed "norm of image"
    for(unsigned int i = 0; i < idxList.size(); i++)
    {
        int n; // number of images in one index
        string foldername = idxList[i];
        FILE* fin_idx = fopen((foldername+"/idx").c_str(), "rb");
        assert( 1 == fread(&n, sizeof(int), 1, fin_idx));


        norm_args arg = {fin_idx, processed, size_voc, idf, word_hist, norm};
        MultiThd::compute_tasks(n, con.nt, &norm_task, &arg);

        fclose(fin_idx);
        assert( (arg.processed - processed) == n );
        processed += n;
    }
    printf("\n");

    delete[] word_hist;
}

/// helper function of update. this function init idf[i]
void SearchEngine::idf_task(void* args, int tid, int i, pthread_mutex_t& mutex)
{
    idf_args* argument = (idf_args*) args;

    std::set<int> word_accu; // a set keeps all the quantized image id of word-i
    for(int j = 0; j < argument->num_entries[i]; j++)
        word_accu.insert(argument->index[i][j].id);

    argument->idf[i] = log(argument->tot_ims/std::max((float)(1+1e-6), (float)(word_accu.size()+1)) );
    word_accu.clear();

    if ( (i+1) % 100 == 0 )
    {
        pthread_mutex_lock (&mutex);
        printf("\r%d", i+1); fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }

}

/// helper function of update. this function init nrom[i]
void SearchEngine::norm_task(void* args, int tid, int i, pthread_mutex_t& mutex)
{
    norm_args* argument = (norm_args*) args;

    // reset word_hist for "thread_tid"
    float* hist = argument->word_hist + tid * argument->size_voc;
    memset(hist, 0, sizeof(float) * argument->size_voc);

    Entry* entry = new Entry();
    int m; // number of points on the processing image

    // read information of "image_processing"
    pthread_mutex_lock (&mutex);
    int processing = argument->processed ++;
    assert( 1 == fread(&m, sizeof(int), 1, argument->fin_idx));
    for(int j = 0; j < m; j++) // j-th point
    {
        entry->read(argument->fin_idx);
        hist[entry->id] += argument->idf[entry->id];
    }
    pthread_mutex_unlock(&mutex);

    argument->norm[processing] = Util::l2_norm(hist, argument->size_voc);

    delete entry;

    if( ( processing + 1) % 100 == 0 )
    {
        pthread_mutex_lock (&mutex);
        printf("\r%d", processing+1); fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }
}



/**
@brief calculate norm for an image
@param entrylist the quantized entrylist of the image
@param n number of entries
@return the norm of the entrylist
*/
float SearchEngine::getNorm(Entry* entrylist, int n)
{
    std::map<int, float> entrymap;
    for(int j = 0; j< n; j++)
    {
        entrymap[ entrylist[j].id ] += idf[entrylist[j].id];
    }

    float q_norm = 0.0;
    for(std::map<int, float>::iterator it = entrymap.begin(); it!= entrymap.end(); it++)
        q_norm += (it->second)*(it->second);
    entrymap.clear();

    return sqrt(q_norm);
}

/**
@brief get number of ones in the binary form of an interger
@param x the interger to evaluate
@return number of ones in binary format of x
*/
inline int SearchEngine::num_of_ones(int x)
{
    x -= ((x>>1)&013333333333)+((x>>2)&01111111111);
    x = ((x>>3)+x)&030707070707;
    x += x>>18;
    return ((x>>12)+(x>>6)+x)&63;
}

