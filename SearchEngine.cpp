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
SearchEngine::SearchEngine(Vocab* vocab, HE* he_assign)
{
    this->voc = vocab;
    this->he = he_assign;

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
    update();
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
    vector<string> filelist = IO::getFileList(dir, con.extn, 1, 1);

    FILE* fout_result = fopen(out_file.c_str(), "w");
    IO::chkFileErr(fout_result, out_file);


    for(unsigned int i = 0; i < filelist.size(); i++)//loop for every query im in directory
    {
        printf("\r%d", i+1); fflush(stdout);
        string filename = Util::parseFileName(filelist[i]);
        fprintf(fout_result, "%s", filename.c_str());

        // quantize descriptors
        int n;
        Entry* entrylist = voc->quantizeFile(filelist[i], he, n, con.nt, con.ma);


        // calc the norm for the query image
        float q_norm = getNorm(entrylist, n);


        vector<Result*> ret;
        switch(con.search_mode)
        {
            case 1:
                ret = search_he_wgc(entrylist, n, q_norm, filename);
                break;
            case 2:
                ret = search_he_ewgc(entrylist, n, q_norm, filename);
                break;
            default:
                printf("Error: search mode not defined.\n");
                exit(1);
        }


        std::sort(ret.begin(), ret.end(), Result::compare);


        for(vector<Result*>::iterator it = ret.begin(); it != min(ret.end(), ret.begin()+topk); it++)
        {
            fprintf(fout_result, " %s %.4f ", im_db[(*it)->im_id].c_str(), (*it)->score);
        }
        fprintf(fout_result, "\n");


        for(unsigned int j = 0; j < ret.size(); j++)
            delete ret[j];
        ret.clear();

        delete[] entrylist;
    }

    printf("\n");

    fclose(fout_result);
    filelist.clear();
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
@brief search with Hamming Embedding and Weak Geometric Consistency
@param entrylist pointer to array of entries
@param n number of entries
@param query_norm norm of the query image
@param query_name file name of the query. used only for debug
@return a vector of Result
*/
vector<Result*> SearchEngine::search_he_wgc(Entry* entrylist, int n, float query_norm, string query_name)
{
    printf("mkdir %s",query_name.c_str());
    #ifdef DEBUG_MODE
    Util::exec("mkdir " + query_name); //Util::exec("rm " + query_name + "/*");
    #endif

    vector<Result*> result;

    vector< vector<float> > scores_scale(tot_ims);
    vector< vector<float> > scores_ori(tot_ims);
    for(int i = 0; i < tot_ims; i++)
    {
        scores_scale[i].resize(16);
        scores_ori[i].resize(16);
    }


    float update;
    int word_id;
    int he_dist;
    float update_quantity;
    int normalize_factor = 8 - 1;

    for ( int i = 0; i < n; i++ )
    {
        word_id = entrylist[i].id;
        update = idf[word_id] * idf[word_id];

        for(int j = 0; j < num_entries[word_id]; j++)
        {
            he_dist = num_of_ones(index[word_id][j].sig ^ entrylist[i].sig);

            if( he_dist <= he->ht )
            {
                int theta = (int)floor((((int)index[word_id][j].ori - (int)entrylist[i].ori) / 10000.0)*8/PI + 16) % 16; // angle difference
                int s = (int)floor(log2(exp(((int)index[word_id][j].scale - (int)entrylist[i].scale)/10000.0)) / (7.5/8)) + normalize_factor; // scale difference
                // since there are some error when quantizing ori/scale, the theta/s could be some error too.
                theta = std::max(theta, 0);
                theta = std::min(theta, 15);
                s = std::max(s, 0);
                s = std::min(s, 15);


                update_quantity = update * he->weights[he_dist];
                scores_scale[index[word_id][j].id][ s ] += update_quantity;
                if(s != 15)
                    scores_scale[index[word_id][j].id][ s+1 ] += update_quantity/4;
                if(s != 0)
                    scores_scale[index[word_id][j].id][ s-1 ] += update_quantity/4;
                scores_ori[index[word_id][j].id][ theta ] += update_quantity;
                scores_ori[index[word_id][j].id][ (theta + 17) % 16 ] += update_quantity;
                scores_ori[index[word_id][j].id][ (theta + 15) % 16 ] += update_quantity;


                #ifdef DEBUG_MODE
                // all previous info + [vote in ori, vote in scale]
                IO::appendLine(query_name + "/" + im_db[ index[word_id][j].id ],
                               Util::num2str(word_id) + " " + Util::num2str(entrylist[i].row) + " " + Util::num2str(entrylist[i].col)
                                + " " + Util::num2str(index[word_id][j].row) + " " + Util::num2str(index[word_id][j].col) + " "
                                + Util::num2str(update_quantity) + " " + Util::num2str(theta) + " " + Util::num2str(s));
                #endif
            }
        }
    }


    float score1, score2;
    for(int i = 0; i < tot_ims; i++)
    {
        #ifdef DEBUG_MODE
        vector<float>::iterator it1 = std::max_element(scores_ori[i].begin(), scores_ori[i].end());
        int score1_index = std::distance(scores_ori[i].begin(), it1);
        score1 = *it1;

        if( score1 > 1)
        {
            float score;
            int select, cell;
            vector<float>::iterator it2 = std::max_element(scores_scale[i].begin(), scores_scale[i].end());
            int score2_index = std::distance(scores_scale[i].begin(), it2);
            score2 = *it2;
            if (score1 > score2)
            {
                score = score2;
                select = 2; //scale
                cell = score2_index;
            }
            else
            {
                score = score1;
                select = 1; // ori
                cell = score1_index;
            }

            result.push_back(new Result(i, score/(norm[i] * query_norm)));
            IO::appendLine(query_name + "/" + im_db[i] + "_aux", Util::num2str(select) + " " + Util::num2str(cell));
        }
        #else
        score1 = *(std::max_element(scores_ori[i].begin(), scores_ori[i].end()));
        if( score1 > 1 )
        {
            score2 = *(std::max_element(scores_scale[i].begin(), scores_scale[i].end()));
            result.push_back( new Result(i, std::min(score1, score2) / (norm[i] * query_norm) ) );
        }
        #endif
    }


    scores_scale.clear();
    scores_ori.clear();

    return result;
}

/**
@brief search with Hamming Embedding and Extended Weak Geometric Consistency
@param entrylist pointer to array of entries
@param n number of entries
@param query_norm norm of the query image
@param query_name file name of the query. used only for debug
@return a vector of Result
*/
vector<Result*> SearchEngine::search_he_ewgc(Entry* entrylist, int n, float query_norm, string query_name)
{
    #ifdef DEBUG_MODE
    Util::exec("mkdir " + query_name); //Util::exec("rm " + query_name + "/*");
    #endif
    /**
    // grid_size = 5
    // grid_num = 25
    // grid_width = im_sz * 2 / 5
    //    0 1 2 3 4
    // 0 | | | | | |
    // 1 | | | | | |
    // 2 | | | | | |
    // 3 | | | | | |
    // 4 | | | | | |
    */
    vector<Result*> result;
    int grid_size = 10;
    int grid_num = grid_size * grid_size;
    int grid_width = 2 * con.im_sz / grid_size; // why *2? tx (or ty) could be in [-im_sz, im_sz]
    float* scores = new float[tot_ims*grid_num]; // keeps the score for each image and offset
    memset(scores, 0, sizeof(float)*grid_num*tot_ims);


    float update, theta, s, cosine, sine;
    int word_id, tx, ty, he_dist = 0;

    //int* vote_cnt = new int[tot_ims*grid_num];
    //memset(vote_cnt, 0, sizeof(int)*tot_ims*grid_num);

    for(int i = 0; i< n; i++) // each point on query image
    {
        word_id = entrylist[i].id;
        update = idf[word_id] * idf[word_id];
        for(int j = 0; j < num_entries[word_id]; j++) // each entry of word_id
        {
            he_dist = num_of_ones(index[word_id][j].sig ^ entrylist[i].sig);
            if (he_dist > he->ht)
                continue;

            // project points on reference image to query image
            theta = ((int)entrylist[i].ori - (int)index[word_id][j].ori)/10000.0;
            s = exp(((int)entrylist[i].scale - (int)index[word_id][j].scale)/10000.0);
            cosine = cos(theta);
            sine = sin(theta);
            tx = entrylist[i].row - ROUND( s * (cosine * index[word_id][j].row - sine   * index[word_id][j].col) ) + con.im_sz; // back-project and make it non-negative
            ty = entrylist[i].col - ROUND( s * (sine   * index[word_id][j].row + cosine * index[word_id][j].col) ) + con.im_sz;


            if (tx < 0 || tx > 2*con.im_sz-1 || ty < 0 || ty > 2*con.im_sz-1 ) // skip bad record
                continue;


            // quantization
            tx /= grid_width;
            ty /= grid_width;
            scores [ index[word_id][j].id * grid_num + tx*grid_size + ty ] += update*he->weights[he_dist];
            /*
            // soft updating scores of neighbor-hoods
            if (tx > 0)
                scores [ index[word_id][j].id * grid_num + (tx-1)*grid_size + ty ] += 0.5*update*he->weights[he_dist];
            if (tx < grid_size)
                scores [ index[word_id][j].id * grid_num + (tx+1)*grid_size + ty ] += 0.5*update*he->weights[he_dist];
            if (ty > 0)
                scores [ index[word_id][j].id * grid_num + tx*grid_size + ty-1 ] += 0.5*update*he->weights[he_dist];
            if (ty < grid_size)
                scores [ index[word_id][j].id * grid_num + tx*grid_size + ty+1 ] += 0.5*update*he->weights[he_dist];
            if (tx > 0 && ty > 0)
                scores [ index[word_id][j].id * grid_num + (tx-1)*grid_size + ty-1 ] += 0.25*update*he->weights[he_dist];
            if (tx > 0 && ty < grid_size)
                scores [ index[word_id][j].id * grid_num + (tx-1)*grid_size + ty+1 ] += 0.25*update*he->weights[he_dist];
            if (tx < grid_size && ty > 0)
                scores [ index[word_id][j].id * grid_num + (tx+1)*grid_size + ty-1 ] += 0.25*update*he->weights[he_dist];
            if (tx < grid_size && ty < grid_size)
                scores [ index[word_id][j].id * grid_num + (tx+1)*grid_size + ty+1 ] += 0.25*update*he->weights[he_dist];
            */

            //vote_cnt [ index[word_id][j].id * grid_num + tx*grid_size + ty ] ++;

            #ifdef DEBUG_MODE
            // all previous info + [vote in which cell]
            IO::appendLine(query_name + "/" + im_db[ index[word_id][j].id ],
                               Util::num2str(word_id) + " " + Util::num2str(entrylist[i].row) + " " + Util::num2str(entrylist[i].col)
                                + " " + Util::num2str(index[word_id][j].row) + " " + Util::num2str(index[word_id][j].col) + " "
                                + Util::num2str(update*he->weights[he_dist]) + " " + Util::num2str(tx*grid_size + ty));
            #endif
        }
    }



    float* scores_final = new float[tot_ims];
    memset(scores_final, 0, sizeof(float)*tot_ims);

    for (int i = 0; i < tot_ims; i++)
    {
        float threshold = 0.9 * (*std::max_element(scores + i*grid_num, scores + (i+1)*grid_num));

        for (int j = 0; j < grid_num; j++)
        {
            if (scores[i * grid_num + j] > threshold)
            {
                scores_final[i] += scores[i*grid_num + j];

                #ifdef DEBUG_MODE
                IO::appendLine(query_name + "/" + im_db[i] + "_aux", Util::num2str(j));
                #endif
            }
        }
    }

    for(int i = 0; i < tot_ims; i++)
    {
        if(scores_final[i] > 1)
            result.push_back(new Result(i, scores_final[i]/ (norm[i] * query_norm)));
    }

    //delete[] vote_cnt;
    delete[] scores;
    delete[] scores_final;
    return result;
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
