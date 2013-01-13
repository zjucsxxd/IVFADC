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

void Entry::set(unsigned int id_l, unsigned int residual_id_l)
{
    id = id_l;
    residual_id = residual_id_l;
}


/**
@brief print the content of the entry to stdout
*/
void Entry::print()
{
    printf("%u %u\n", id, residual_id);
}

/**
@brief write an entry record to a file
@param fout file pointer to write
*/
void Entry::write(FILE* fout)
{
    unsigned int* items = new unsigned int[2];
    items[0] = id;
    items[1] = residual_id;

    fwrite(items, sizeof(unsigned int), 2, fout);

    delete[] items;
}

/**
@brief read a single entry from file
@param fin file pointer to read
*/
void Entry::read(FILE* fin)
{
    unsigned int *items = new unsigned int[2];
    assert(2 == fread(items, sizeof(unsigned int), 2, fin));

    id     = items[0];
    residual_id = items[1];

    delete[] items;
}
