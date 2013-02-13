#include <iostream>
#include "Vocab.h"
#include "config.h"

using namespace std;

class ivfpq_new
{
private:
    // number of centroids for the coarse quantizer.
    int     coarsek;
    // vector dim
    int     d;
    // limit_point
    int limit_point;
    // index descriptors
    string train_desc;
    // data id
    string dataId;
    // coarse codebook
    float* coa_centroids;
    // matrix dir
    string train_matrix;
    // residual hiearachy codebook
    Vocab* voc;
    /////////////////////////////////////////////////
    // use for kmeans
    int iter;
    int attempts;
    int nt;
    // number of centroids per subquantizer
    int     k;
    // number of hierarchy kmeans layers.
    int     l;
    int     nsq;
    int     nsqbits;
    
private:
    void init(string work_dir);
    
public:
    ivfpq_new(Config& con_l);
    void train_coarse_codebook();
    void train_residual_codebook();
    
    void cal_word_dis(float* codebook, int n_l, int dim_l, string filename);
};
