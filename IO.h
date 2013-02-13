/**
@file IO.h
@brief Handles general IO operations such as reading/writting disk.
@author wzhang
@date May 6th, 2012
*/

#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>  
#include <fcntl.h>

#include "MultiThd.h"
#include "util.h"
#include "config.h"

using std::string;
using std::vector;
using std::fstream;

extern Config con;

typedef char TCHAR;
typedef unsigned char byte;

/// holds the parameters used to divide matrix file according to centers using multi-threading
struct div_par
{
	/// file handler for input base matrix
    FILE* fin;
    /// centers defines the quantization rule. size: k x d
    const float* centers;
    /// number of centers
    int k;
    /// dimension
    int d;
	/// indicates the layer number in hierarchical clustering
    int layer;
    /// indicates the number of processing center
    int number;
    /// the matrix file to divide
    string mtrx;

    /// aux fields for hanlding the race when writing output
    int* writting;

	// out
    /// number of points quantized to each center. size: k x 1
    int* cnt;
};


/// Misc IO operations provided
class IO
{
public:

	/**
    @brief append a line at the end of a file
    @param filename file to write
    @param info content to write
    @return void
    @remark used only for debug
    */
    static void appendLine(string filename, string info)
    {
        FILE* fout = fopen(filename.c_str(), "a");
        assert(fout != NULL);

        fprintf(fout, "%s\n", info.c_str());

        fclose(fout);
    }

    /**
    @brief read content of feature file
    @param feat_file file name to read
    @param n number of feature points
    @param m number of auxiliary information per feature
    @param d dimension of each feature
    @return features of feat_file. size of n x (m+d)
    @remark Be ware of the memory it allocated (returned). User should take care of the memory it returned
    */
    static float* readFeatFile(string feat_file, int& n, int& m, int& d)
    {
        FILE* fin = fopen(feat_file.c_str(), "rb");
        chkFileErr(fin, feat_file);

        assert( 1 == fread(&n, sizeof(int), 1, fin) );
        assert( 1 == fread(&m, sizeof(int), 1, fin) );
        assert( 1 == fread(&d, sizeof(int), 1, fin) );

        byte* tmp = new byte[d];
        float* feature = new float[n*(m+d)];
        for(int i = 0; i < n; i++)
        {
            assert( m == (int)fread(feature + i*(m+d), sizeof(float), m, fin));

            assert( d == (int)fread(tmp, sizeof(byte), d, fin));

            // convert to float array
            int pos = i*(m+d) + m;
            for(int j = 0; j < d; j++)
                feature[pos + j] = (float)(tmp[j]);
        }
        //assert( n*(m+d) == (int)fread(feature, sizeof(float), n*(m+d), fin) );

        delete[] tmp;

        fclose(fin);

        return feature;
    }

    /**
    @brief load floating point based matrix from disk to memory
    @param mtrx the matrix file. size: row*col
    @param row reference to row of the matrix
    @param col reference to col of the matrix
    @param limit_points maximum rows to read.
    when limit_points >  0, only limit_points entry will be read
    when limit_points <= 0, all data in 'mtrx' will be read
    @return pointer to the matrix
    @remark take care of the memory it returned
    */
    static float* loadFMat(string mtrx, int& row, int& col, int limit_points)
    {
        FILE* fin = fopen(mtrx.c_str(), "rb");
        chkFileErr(fin, mtrx);

        assert( 1 ==  fread(&row, sizeof(int), 1, fin) );
        assert( 1 ==  fread(&col, sizeof(int), 1, fin) );


        if (limit_points > 0)
            row = std::min((int)limit_points, row);

        float* mat = new float[col*row];

        assert( row*col == (int)fread(mat, sizeof(float), row*col, fin) );
        fclose(fin);

        return mat;
    }

	/**
    @brief load integer based matrix from disk to memory
    @param mtrx the matrix file. size: row*col
    @param row reference to row of the matrix
    @param col reference to col of the matrix
    @param limit_points maximum rows to read.
    when limit_points >  0, only limit_points entry will be read
    when limit_points <= 0, all data in 'mtrx' will be read
    @return pointer to the matrix
    @remark take care of the memory it returned
    */
    static int* loadIMat(string mtrx, int& row, int& col, int limit_points)
    {
        FILE* fin = fopen(mtrx.c_str(), "rb");
        chkFileErr(fin, mtrx);

        assert( 1 == fread(&row, sizeof(int), 1, fin) );
        assert( 1 == fread(&col, sizeof(int), 1, fin) );


        if (limit_points > 0)
            row = std::min(limit_points, row);

        int* mat = new int[row*col];

        assert( row*col == (int)fread(mat, sizeof(int), row*col, fin) );
        fclose(fin);

        return mat;
    }


	/**
	@brief write floating point based matrix to disk
	@param mat pointer to the matrix to write
	@param n rows of the matrix
	@param d cols of the matrix
	@param file file name of the matrix to write
	@return void
	*/
    static void writeMat(float* mat, int n, int d, std::string file)
    {
        FILE* fout = fopen(file.c_str(), "wb");
        chkFileErr(fout, file);

        fwrite(&n, sizeof(int), 1, fout);
        fwrite(&d, sizeof(int), 1, fout);

        fwrite(mat, sizeof(float), n*d, fout);

        fclose(fout);
    }

	/**
	@brief write interger based matrix to disk
	@param mat pointer to the matrix to write
	@param n rows of the matrix
	@param d cols of the matrix
	@param file file name of the matrix to write
	@return void

	*/
    static void writeMat(int* mat, int n, int d, std::string file)
    {
        FILE* fout = fopen(file.c_str(), "wb");
        chkFileErr(fout, file);

        fwrite(&n, sizeof(int), 1, fout);
        fwrite(&d, sizeof(int), 1, fout);

        fwrite(mat, sizeof(int), n*d, fout);

        fclose(fout);
    }


	/**
	@brief get all sub-directories (except . & ..) of a directory
	@param dir directory to explore
	@return all sub directories
	*/
    //
    static vector<string> getFolders(string dir)
    {
        if (!Util::endWith(dir, "/"))
        {
            printf("Error: %s is not a directory.\n", dir.c_str());
            exit(1);
        }

        vector<string> folders;

        DIR* dp = opendir(dir.c_str());
        struct dirent* dirp;
        if(!dp)
        {
            printf("error when opening directory: %s\n", dir.c_str());
            exit(1);
        }
        std::string filename;
        while((dirp = readdir(dp)) != NULL)
        {
            filename = dirp->d_name;
            if ( filename == "." || filename == "..")
                continue;

            struct stat s;
            stat((dir + filename).c_str(), &s);
            if (S_ISDIR(s.st_mode)) //dirp is a directory: DT_DIR
                folders.push_back(dir + filename);

        }
        closedir(dp);

        std::sort(folders.begin(), folders.end());

        return folders;
    }

    /**
    @brief get all files in a directory
    @param dir directory to explore
    @param substr a sub string which returned files should begin/end with
    @param search_mode 0: top dir only, 1: recursively
    @param name_mode name_mode = 0: str = startwithstr, 1: str = endwithstr
  	@return file names which meets the search criteria
    */
    static vector<string> getFileList(string dir, string substr, int search_mode, int name_mode)
    {
        std::cout << "getFileList:" << dir << std::endl;
        std::cout << Util::endWith(dir, "/") << std::endl;
        if(!Util::endWith(dir, "/"))
        {
            printf("Error: %s is not a directory.\n", dir.c_str());
            exit(1);
        }

        vector<string> filelist;
        vector<string> subFilelist;
        filelist.clear();
        subFilelist.clear();

        DIR* dp = opendir(dir.c_str());
        struct dirent* dirp;
        if(!dp)
        {
            printf("error when opening directory: %s\n", dir.c_str());
            exit(1);
        }

        std::string filename;
        while((dirp = readdir(dp)) != NULL)
        {
            filename = dirp->d_name;
            if ( filename == "." || filename == "..")
                continue;

            struct stat s;
            stat((dir + filename).c_str(), &s);
            if (S_ISDIR(s.st_mode) && search_mode == 1)
            {//dirp is a directory: DT_DIR
                subFilelist = IO::getFileList(dir + filename + "/", substr, 1, name_mode);
            }
            else
            {// it's a regular file
                if(name_mode == 0 && Util::startWith(filename, substr)) // filename should match in startwithstr manner
                {
                    filelist.push_back(dir+filename);
                }
                else if(name_mode == 1 && Util::endWith(filename, substr))// filename end with something in common endwithstr
                {
                    filelist.push_back(dir+filename);
                }
            }


            filelist.insert(filelist.end(), subFilelist.begin(), subFilelist.end());
            subFilelist.clear();

        }
        closedir(dp);

        std::sort(filelist.begin(), filelist.end());

        return filelist;
    }


	/**
	@brief check if a file handler is OK to read/write
	*/
    static void chkFileErr(FILE* f, string fn)
    {
        if (!f)
        {
            printf("FILE IO ERROR: %s\n", fn.c_str());
            exit(1);
        }
    }

	/**
	@brief make a directory on disk
	*/
    static void mkdir(string dir)
    {
        /*
        TCHAR *param = new TCHAR[dir.size()+1];
        param[dir.size()] = 0;
        std::copy(dir.begin(),dir.end(),param);
        //if (!f_exists(dir))
        */
        if (!folderExists(dir.c_str()))
            Util::exec("mkdir \"" + dir + "\"");
    }

    /**
    @brief remove a file on disk
    */
    static void rm(string file)
    {
        replace(file.begin(), file.end(), '/', '\\');
        if(f_exists(file)) // if file exists
            Util::exec("del \"" + file + "\"");
    }


	/**
	@brief check if a folder exists
	@return Return TRUE if file 'fileName' exists
	*/
    /*
    static bool folderExists(const TCHAR *fileName)
    {
        DWORD fileAttr;
        fileAttr = GetFileAttributes(fileName);
        return (fileAttr != INVALID_FILE_ATTRIBUTES && (fileAttr & FILE_ATTRIBUTE_DIRECTORY));
    }
    */

    static bool folderExists(const TCHAR *fileName)
    {
        if((access(fileName ,F_OK))!=-1)  
        {  
            return true;
        }  
        else 
            return false;
    }

	/**
	@brief check if a file exists
	*/
    static bool f_exists(string file)
    {
        struct stat sb;
        return stat(file.c_str(), &sb) == 0;
    }


    /**
    Divide matrix to sub-matrices
    @brief divide 'mtrx' into n sub matrix, according to the quantization rule defined by 'centers'
    @param mtrx base matrix file. the complete name for spilting is 'mtrx_layer_number'
    @param layer layer id
    @param number number id of center in layer 'layer'
    @param centers defines quantization using centers. size k x d
    @param k rows of centers
    @param d dimension for centers
    @param nt number of threads to use
    @return void
    @remark note this can not be like IO::loadMatrix, since the limitation of memory could be a issue.
    So this function read & write data sequencially (slow)
    */
    static void divMatByCenters_MT(string mtrx, int layer, int number, const float* centers, int k, int d, int nt)
    {
        // write reserved bytes to each divided matrix file
        int fakeHead[2] = {0, d};
        for(int i = 0; i < k; i++)
        {
            FILE* fout = fopen((mtrx + ".l" + Util::num2str(layer+1) + ".n" + Util::num2str(number*k + i)).c_str(), "wb");
            fwrite(fakeHead, sizeof(int), 2, fout);
            fclose(fout);
        }

        // start dividing
        string baseMtrx = mtrx + ".l" + Util::num2str(layer) + ".n" + Util::num2str(number);
        FILE* fin = fopen( baseMtrx.c_str(), "rb");
        IO::chkFileErr(fin, baseMtrx);

        int row, col;
        int* count = new int[k];
        int* writting = new int[k];
        memset(count, 0, sizeof(int)*k);
        memset(writting, 0, sizeof(int)*k);

        assert( 1 == fread(&row, sizeof(int), 1, fin) );
        assert( 1 == fread(&col, sizeof(int), 1, fin) );
        assert(d == col || "dimension of feature must be consistent.");

        div_par para = {fin, centers, k, d, layer, number, mtrx, writting, count};
        MultiThd::compute_tasks(row, nt, &div_task, &para);
        fclose(fin);

        for(int i = 0; i < k; i++)
        {
            FILE* fout = fopen((mtrx + ".l" + Util::num2str(layer+1) + ".n" + Util::num2str(number*k + i)).c_str(), "r+b");
            fwrite(count + i, sizeof(int), 1, fout);
            fclose(fout);
        }

        delete[] count;
        delete[] writting;
    }

	/**
	Divide single row of features.
	@brief helper function for divMatByCenters_MT
	@param arg arguments which is div_par
	@param tid thread id
	@param i id of task
	@param mutex mutex to synchronize threads
	@return void
	*/
    static void div_task(void* arg, int tid, int i, pthread_mutex_t& mutex)
    {

        div_par* t = (div_par*) arg;


        float* vec = new float[t->d];

        // read data
        pthread_mutex_lock(&mutex);
        assert(t->d == (int)fread(vec, sizeof(float), t->d, t->fin));
        pthread_mutex_unlock(&mutex);

        float dist_best = 1e100;
        int nn_id = -1;
        for(int m = 0; m < t->k; m++) // loop for k centers, decides the nearest one
        {
            float dist = Util::dist_l2_sq(t->centers + m*t->d, vec, t->d);
            if(dist < dist_best)
            {
                dist_best = dist;
                nn_id = m;
            }
        }


        while(1)
        {
            pthread_mutex_lock(&mutex);

            // prepare to write file if no other threads writting to 'nn_id'-th file
            if(t->writting[nn_id] == 0)
            {
                t->writting[nn_id] = 1;
                t->cnt[nn_id] ++;

                pthread_mutex_unlock(&mutex);

                break; // go ahead and write file
            }
            else // wait another thread finishing writting
            {
                pthread_mutex_unlock(&mutex);
                Util::uSleep(rand() % 10);
            }
        }

        pthread_mutex_lock(&mutex);
        string filename = t->mtrx + ".l" + Util::num2str(t->layer+1) + ".n" + Util::num2str(t->number*t->k + nn_id);
        pthread_mutex_unlock(&mutex);
        //printf("%s\n", filename.c_str());
        //fflush(stdout);




        FILE* fout = fopen(filename.c_str(), "ab");
        fwrite(vec, sizeof(float), t->d, fout);
        fclose(fout);

        pthread_mutex_lock(&mutex);
        t->writting[nn_id] = 0;
        pthread_mutex_unlock(&mutex);

        delete[] vec;
    }







    /**
    Sample features to generate random matrix file for later processing, such as vocabulary training and Hamming trainning
    @brief generate matrix by sampling on the feature files in dir
    @param dir src feature directory
    @param mtrx dest mtrx file location
    @param num_point # of points to be sampled from 'dir' to 'mtrx'
    @return void
    */
    static void genMtrx(std::string dir, std::string mtrx, int num_point)
    {
        printf("Samping %d points.\n", num_point);
        vector<string> fileList = IO::getFileList(dir.c_str(), con.extn, 1, 1);
        assert( fileList.size() || !"Directory is empty for sampling.");


        FILE* fout = fopen(mtrx.c_str(), "wb"); // num_points x con.dim
        IO::chkFileErr(fout, mtrx);
        fwrite(&num_point, sizeof(int), 1, fout);
        fwrite(&con.dim, sizeof(int), 1, fout);


        int i = 0; // indicating number of points sampled.
        float tmp;
        FILE* fin = NULL;

        int* header = new int[3]; // [n m d]
        byte* vec = new byte[con.dim]; // single feature
        srand(time(NULL));
        while( i < num_point ) // sample num_points entries
        {
            string filename = fileList[rand() % fileList.size()];
            // std::cout << "total point:" << num_point << "\tpoint:" << i << "\tsrc file:" << filename << std::endl;
            fin = fopen(filename.c_str(), "rb");
            IO::chkFileErr(fin, filename);


            assert( 3 == fread(header, sizeof(int), 3, fin) ); // read header
            //std::cout << header[0] << " " << header[1] <<  " " << header[2] << std::endl;
            assert( header[2] == con.dim);

            int sample_this_file = std::ceil(header[0] * con.num_per_file);
            if (sample_this_file == 0) // if no points to sample for this file, skip.
            {
                fclose(fin);
                continue;
            }

            // gen random lines ids to sample
            vector<int> rand_line;
            for (int j = 0; j< sample_this_file; j++)
            {
                int rand_num = rand() % header[0];

                while ( find(rand_line.begin(), rand_line.end(), rand_num) != rand_line.end() )
                    rand_num = rand() % header[0];

                rand_line.push_back(rand_num);
            }
            std::sort(rand_line.begin(), rand_line.end());


            // cp the desired features
            int lastPos = 0;
            for(int j = 0; j < sample_this_file; j++)
            {
                fseek(fin, (rand_line[j] - lastPos) * (sizeof(byte) * header[2] + sizeof(float)*header[1]) + sizeof(float) * header[1], SEEK_CUR);
                lastPos = rand_line[j] + 1;

                assert( header[2] == (int)fread(vec, sizeof(byte), header[2], fin) );
                for(int k = 0; k < header[2]; k++)
                {
                    tmp = (float)(vec[k]);
                    assert(tmp < 256);
                    fwrite(&tmp, sizeof(float), 1, fout);
                }
            }


            fclose(fin);
            rand_line.clear();
            i += sample_this_file;
            printf("\r%.2f%%", ((float)i/num_point)*100); fflush(stdout);
        }
        fclose(fout);
        printf("\n");

        delete[] vec;
        delete[] header;
    }

    static void load_vlad(string train_desc, float** data, vector<string*>* img_db , int* n, int* d)
    {

        vector<string> filelist =  getFileList(train_desc, "vlad", 0, 1);
        vector<string> info_filelist =  getFileList(train_desc, "info", 0, 1);
        vector<float*> datalist;
        int total_num = 0;
        int dims = 0; 
        std::cout << "filelist size:" << filelist.size() << std::endl;
        int* info = new int[filelist.size()];
        for(unsigned int k=0;k < filelist.size(); k++)
        {
            fstream fin;
            fstream fin_info;
            fin.open(filelist[k].c_str(), std::ios::in);
            fin_info.open(info_filelist[k].c_str(), std::ios::in);

            int vec_num,dim;
            fin >> vec_num >> dim;
            std::cout << vec_num << " " << dim << std::endl;
            info[k] = vec_num;
            total_num += vec_num;
            dims = dim;
            float* tmp = new float[dim*(vec_num)];
            for(int i=0; i < vec_num; i++)
            {
                string* name_tmp = new string;
                for(int j=0; j < dim; j++)
                {
                    fin >> tmp[j+i*dim];
                }
                fin_info >> *name_tmp;
                img_db->push_back(name_tmp);
            }
            datalist.push_back(tmp);
            fin.close();
            fin_info.close();
        }

        *n = total_num;
        *d = dims;
        *data = new float[dims*total_num];
        int counter = 0;
        unsigned int dsize = datalist.size();
        for(int g=0; g < dsize;g++ )
        {
            std::cout << info[g] << " " << std::endl;
            for(int i=0; i< info[g];i++)
            {
                for(int j=0; j<dims;j++)
                {
                    //std::cout << datalist[g] << std::endl;
                    (*data)[j+dims*counter] = datalist[g][j+dims*i];
                }
                counter += 1;
            }
            delete[] datalist[g];
        }
    }
    static void write_img_db(vector<string*> img_db, string filename)
    {
        fstream out;
        out.open(filename.c_str(), std::ios::out); 
        out << img_db.size() << "\n";
        for(unsigned int i=0; i < img_db.size(); i++)
        {
            out << *(img_db[i]) << "\n";            
        }
    }
};

#endif // IO_H_INCLUDED
