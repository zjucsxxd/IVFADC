#include "Index.h"
#include <fstream>
using std::fstream;

void Index::indexFiles(Vocab* voc, Vocab* rvoc, string feat_dir, string file_extn, string idx_dir, int nt)
{
    IO::mkdir(idx_dir);
    string idx_file = idx_dir + "idx";
    string nl_file  = idx_dir + "nl";
    string idx_sz   = idx_dir + "voc_sz";


    printf("Indexing files: \"%s\"\n", feat_dir.c_str());
    FILE* fout_idx = fopen(idx_file.c_str(), "wb");
    FILE* fout_nl  = fopen(nl_file.c_str(), "w");
    assert(fout_idx && fout_nl);


    int tot_ims = 0;
    int dim = 0;
    float* feature;
    vector<string*> namelist;
    IO::load_vlad(feat_dir, &feature, &namelist, &tot_ims, &dim);
    std::cout << "normalize..." << std::endl;
    for(int i=0; i < tot_ims; i++)
    {
        Util::normalize(feature+i*dim, dim);
    }
    std::cout << "normalize finished..." << std::endl;



    fwrite(&tot_ims, sizeof(int), 1, fout_idx);
    fprintf(fout_nl, "%d\n", tot_ims);


    index_args args = {dim, feature, namelist, voc, rvoc, fout_idx, fout_nl};
    MultiThd::compute_tasks(tot_ims, nt, &index_task, &args);
    printf("\n");

    fclose(fout_idx);
    fclose(fout_nl);
    for(unsigned int i=0; i < namelist.size(); i++)
    {
        delete namelist[i];
    }

    gen_idx_sz_file(idx_file, idx_sz, voc->num_leaf);
}


void Index::index_task(void* args, int tid, int i, pthread_mutex_t& mutex)
{

    //std::cout << "enter index task fun..." << std::endl;
    index_args* arguments = (index_args*) args;
    string filename = *(arguments->namelist[i]);
    float* feature = i*arguments->dim+arguments->feature;
    string shortname = Util::parseFileName(filename);
    std::cout << "shortname:" << shortname << std::endl;

    int m = 0, n = 1, d=0;
    d = arguments->dim;

    //std::cout << "m:" << m << "n:" << n << "d:" << d << std::endl;

    int* quanti_result = new int[n];
    int* residual_result = new int[n];

    //std::cout << "quantize coarse vector..." << std::endl;
    arguments->voc->quantize2leaf(feature, quanti_result, n, m);

    //std::cout << "calculate residual vectors..." << std::endl;
    float* residual_vec = new float[n*(d+m)];
    for(int j=0; j < n; j++)
    {    
        for(int x=0;x<d;x++)
        {
            residual_vec[j*(d+m)+m+x] =*(feature+j*(d+m)+m+x) - *(arguments->voc->vec+quanti_result[j]*d+x); 
        }
    }

    //std::cout << "quantize residual vector..." << std::endl;
    arguments->rvoc->quantize2leaf(residual_vec, residual_result, n, m);

    //std::cout << "construct entry list..." << std::endl;
    Entry* entrylist = new Entry[n];
    for(int j = 0; j < n; j++)
    {
        entrylist[j].set(quanti_result[j], residual_result[j]);
    }

    //std::cout << "write sync to idx file..." << std::endl;
    /// write sync
    pthread_mutex_lock (&mutex);
    fwrite(&n, sizeof(int), 1, arguments->fout_idx);
    for(int j= 0; j < n; j++)
        entrylist[j].write(arguments->fout_idx);


    //std::cout << "write to nl..." << std::endl;
    fprintf(arguments->fout_nl, "%s\n", shortname.c_str());
    pthread_mutex_unlock(&mutex);
    /// end of write sync

    //std::cout << "delete new space..." << std::endl;
    delete[] entrylist;
    delete[] quanti_result;
    delete[] residual_vec;

    //std::cout << "prepare existing..." << std::endl;
    if ( (i+1) % 10 == 0)
    {
        pthread_mutex_lock (&mutex);
        printf("\r%d", i+1); fflush(stdout);
        pthread_mutex_unlock(&mutex);
    }
    //std::cout << "exist function Index.." << std::endl;
}

void Index::gen_idx_sz_file(string idx_file, string idx_sz, int voc_size)
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


