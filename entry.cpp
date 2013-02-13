/**
@file entry.cpp
@brief this file implements entry.h
@author wzhang
@date May 7th, 2012
*/





#include <cmath>
#include <cstdio>
#include <cassert>

#include "entry.h"

/**
@brief cpy constructor
*/
Entry::Entry(const Entry& p) // copy constructor
{
    this->id        = p.id;
    this->residual_id = p.residual_id;
}

void Entry::set(unsigned int id_l, unsigned int nsq, unsigned int* residual_id_l)
{
    id = id_l;
    residual_id = residual_id_l;
}

void Entry::set(unsigned int id_l, unsigned int nsq, unsigned int* residual_id_l, float* residual_vec_l)
{
    id = id_l;
    residual_id = residual_id_l;
    residual_vec = residual_vec_l;
}

/**
@brief print the content of the entry to stdout
*/
void Entry::print()
{
    printf("coarse:%u \n", id);
    for(unsigned int i = 0; i < nsq; i++)
    {
        printf("residual:%u ",residual_id[i]);
    }
}

/**
@brief write an entry record to a file
@param fout file pointer to write
*/
void Entry::write(FILE* fout)
{
    unsigned int* items = new unsigned int[nsq+1];
    items[0] = id;
    for(unsigned int i = 0; i < nsq; i++)
    {
        items[i+1] = residual_id[i];
    }

    fwrite(items, sizeof(unsigned int), nsq+1, fout);

    delete[] items;
}

/**
@brief read a single entry from file
@param fin file pointer to read
*/
void Entry::read(FILE* fin)
{
    unsigned int *items = new unsigned int[nsq+1];
    assert(nsq+1 == fread(items, sizeof(unsigned int), nsq+1, fin));

    id     = items[0];
    residual_id = new unsigned int[nsq];
    for(unsigned int i = 0; i < nsq; i++)
    {
        residual_id[i] = items[i+1];
    }

    delete[] items;
}
