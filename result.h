/**
@file result.h
@brief this file defines the structure for search result
@author wzhang
@date May 7th, 2012
*/
#ifndef RESULT_H_INCLUDED
#define RESULT_H_INCLUDED


/// search result structure
struct Result 
{
	/// the similarity score of this result to the quer
    float score;
    /// the image id of this result
    int im_id;

    Result(){}

	
    Result(int id, float s)
    {
        this->score = s;
        this->im_id = id;
    }
    ~Result()
    {
    }

	/// comparer function which is used for sorting results
    static bool compare(Result* r1, Result* r2)
    {
        return r1->score > r2->score;
    }
};

#endif // RESULT_H_INCLUDED
