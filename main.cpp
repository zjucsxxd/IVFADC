#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>
#include <stdio.h>

#include "ParamReader.h"
#include "ivfpq_new.h"
#include "config.h"
#include "Index.h"
#include "SearchEngine.h"
#include "PQCluster.h"


using std::string;
using namespace std;


Config con; // global configuration: keeps settings of the program


/// entry point of whole project
int main(int argc, char* argv[])
{

	// check input arguments and print help message
    if (argc < 3)
    {
        printf("Usage: \n%s <config_file> <mode>\n", argv[0]);
        exit(1);
    }

    char* conf              = argv[1];
    con.mode                = atoi(argv[2]);

    CParamReader *params    = new CParamReader (conf);

    con.dataId              = params->GetStr ("dataId");
    string id               = con.dataId;
    con.nt                  = params->GetInt ("nt");
    con.num_per_file        = 0.02;
    con.T                   = 10000;

	// check running mode
    switch(con.mode)
    {
        case 1: // gen vocab
        {
            // number of centroids for the coarse quantizer.
            con.coarsek             = params->GetInt("coarsek");
            // number of subquantizers to be used, m in the paper
            con.nsq = params->GetInt("nsq");
            // the number of bits per subquantizer
            con.nsqbits = params->GetInt("nsqbits");

            con.train_desc          = params->GetStr ("train_desc");
            con.dim                 = params->GetInt ("dim");
            con.iter                = params->GetInt ("iter");
            con.attempts            = params->GetInt ("attempts");

            //con.coarsek             = params->GetInt("coarsek");
            //Vocab* voc = new Vocab(con.coarsek, 1, con.dim);
            //voc->loadFromDisk(id + ".out/vk_words/");
            
            ivfpq_new* ivfpq = new ivfpq_new(con);
            ivfpq->train_coarse_codebook();
            ivfpq->train_residual_codebook();
            break;
        }
        case 2: // quantization 
        {
            con.dim                 = params->GetInt ("dim");
            con.coarsek             = params->GetInt("coarsek");
            // index feature dir
            con.index_desc          = params->GetStr("index_desc");

            con.nsq                 = params->GetInt("nsq");
            con.nsqbits             = params->GetInt("nsqbits");
            
            Vocab* voc = new Vocab(con.coarsek, 1, con.dim);
            voc->loadFromDisk(id + "/vk_words/");

            PQCluster* pqvoc = new PQCluster(con.nsqbits, con.nsq, con.dim);
            pqvoc->loadFromDisk(id + "/vk_words_residual/");
            //pqvoc->print_clusters();

            IO::mkdir(id + "/index/");
            Index::indexFiles(voc, pqvoc, con.index_desc, ".vlad", id + "/index/", con.nt,con.coarsek);

            delete voc;
            break;
        }
        case 3: // online search
        {
            con.coarsek             = params->GetInt("coarsek");
            con.nsq                 = params->GetInt("nsq");
            con.nsqbits             = params->GetInt("nsqbits");
            con.query_desc          = params->GetStr ("query_desc");
            con.dim                 = params->GetInt ("dim");

            // number of elements to be returned.
            con.num_ret             = params->GetInt ("num_ret");
            // number of cell visited per query.
            con.ma                  = params->GetInt ("ma");

            Vocab* voc = new Vocab(con.coarsek, 1, con.dim);
            voc->loadFromDisk(id + "/vk_words/");
            
            PQCluster* pqvoc = new PQCluster(con.nsqbits, con.nsq, con.dim);
            pqvoc->loadFromDisk(id + "/vk_words_residual/");
            //pqvoc->print_clusters()

            SearchEngine* engine = new SearchEngine(voc, pqvoc);
            engine->loadIndexes(id + "index/");
            engine->search_dir(con.query_desc, id + "result", id + "coarse_result", con.num_ret);

            delete engine;
            delete voc;

            break;
        }
        default:
        {
            printf("Un-defined operation! Exitting ...\n");
            return 1;
        }
    }

    return 0;
}
