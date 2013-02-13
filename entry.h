/**
@file entry.h
@brief this file defines the basic element in index.
@author wzhang
@date May 7th, 2012
*/

#ifndef ENTRY_H_INCLUDED
#define ENTRY_H_INCLUDED


#define PI 3.1415926
#define ROUND(d) ((unsigned int)(floor(d+0.5)))

#include <cmath>
#include <cstdio>
#include <cassert>


/**
One entry consists of the image id (32 bits), row (16 bits), column (16 bits),
orientation (16 bits), scale (16 bits) and hamming signature (32 bits). Entry is the basic
item in inverted file.
@brief defines the basic element inside the index.
*/
struct Entry
{
    /// id of entry is tricky here. serves as both id and word_id
    unsigned int id:32;          // id. taking 32 bits can index up to 4 trillion images
    unsigned int nsq;
    unsigned int* residual_id;
    // this is the residual vector of a vector quantize to a coarse word. 
    float*       residual_vec;

	/**
	@ default constructor which does nothing but allocates memory.
	*/
    Entry(){} // does nothing constructor

    /**
    @brief cpy constructor
    */
    Entry(const Entry& p);

    /**
	@brief destructor that free memory
	*/
    ~Entry(){}

    void set(unsigned int id_l, unsigned int nsq_l, unsigned int* residual_id_l);
    void set(unsigned int id_l, unsigned int nsq, unsigned int* residual_id_l, float* residual_vec_l);
	
    /**
    @brief print the content of the entry to stdout
    */
    void print();

	/**
    @brief write an entry record to a file
    @param fout file pointer to write
    */
    void write(FILE* fout);

    /**
    @brief read a single entry from file
    @param fin file pointer to read
    */
    void read(FILE* fin);
};

#endif // ENTRY_H_INCLUDED
