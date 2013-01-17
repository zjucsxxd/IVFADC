/**
@file util.h
@brief This file provides several project-independent utilities which are commonly used functions in C++,
including string manipulation, math operations as well as other common functions.
@author wzhang
@date May 7th, 2012
*/

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED



#include <vector>
#include <cmath>
#include <sched.h>
#include <cstdlib>
//#include "windows.h"
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <map>
#include <cmath>
#include <dirent.h>
#include <algorithm>
#include <cstring>
#include <cassert>

using std::string;

/// common utilities for string, math and other misc operations
class Util
{
public:

    //static int numCPU;
//------------------------------------string---------------------------------------


    /**
    @brief convert a number to string
    @param number the data to convert
    @return same number with type of "string"
    */
    template <class T>
    static std::string num2str(T number)
    {
        std::stringstream ss;
        ss.clear();
        ss << number;
        return ss.str();
    }

	/**
	@brief finds the last occurance of a character inside a string
	*/
    static int lastIndexOf(std::string str, char ch)
    {
        for(int i = str.length()-1; i>0; i--)
        {
            if(str[i] == ch)
                return i;
        }
        return 1;
    }

	/**
	@brief check if a string ends with another string
	*/
    static bool endWith(std::string str, std::string postfix)
    {
        assert(postfix.length() <= str.length() );
        return postfix == str.substr(str.length() - postfix.length(), postfix.length());
    }

	/**
	@brief check whether a string starts with another string
	*/
    static bool startWith(std::string str, std::string prefix)
    {
        if (prefix.length() > str.length())
        {
            return false;
        }
        return str.substr(0, prefix.length()) == prefix;
    }

	/**
	@brief get the file name out of the full path name.
	@remark file extension is dropped also.
	*/
    static std::string parseFileName(std::string fullpath)
    {
        int pos1 = Util::lastIndexOf(fullpath, '/');
        int pos2 = Util::lastIndexOf(fullpath, '.');
        return fullpath.substr(pos1+1, pos2-pos1-1);
    }

	/**
	Typical strtok function.
	@brief tokenize a string with a delimiter
	*/
    static std::string strtok(std::string& str, std::string deli)
    {
        int i = str.find_first_of(deli, 0);

        std::string ret = str.substr(0, i);
        str = str.substr(i+1, str.size()-i-1);
        return ret;
    }

	/**
	@brief drop starting and ending white spaces for a string
	*/
    static std::string trim(std::string str)
    {
        std::string trimStr = " ";
        std::string::size_type pos = str.find_first_not_of( trimStr );
        str.erase( 0, pos );

        pos = str.find_last_not_of( trimStr );
        str.erase( pos + 1 );

        return str;
    }

//------------------------------------math---------------------------------------

	/**
	@brief calc the l1 norm for a given vector
	@param vec pointer to the vector
	@param size size of the vector
	*/
    template <class T>
    static float l1_norm(T* vec, int size)
    {
        float n = 0.0;
        for(int i = 0; i<size; i++)
            n += fabs(vec[i]);
        return n;
    }

	/**
	@brief calc the l2 norm for a given vector
	@param vec pointer to the vector
	@param size size of the vector
	*/
    template <class T>
    static float l2_norm(T* vec, int size)
    {
        float n = 0.0;
        for(int i =0; i<size; i++)
            n += (vec[i]*vec[i]);

        return sqrt(n);
    }

	/**
	@brief normalize a vector with its l2 norm
	*/
    template <class T>
    static void normalize(T* vec, int size)
    {
        float norm = Util::l2_norm(vec, size);
        for(int i = 0; i<size; i++)
            vec[i] /= norm;
    }


//------------------------------------Misc----------------------------------------

	/**
	@brief projects a vector with another matrix. In short out = Px.
	@param P the projection matrix, size of row x col
	@param row row of P
	@param col column of P
	@param x vector to project, size of col x 1
	@param out vector projected, size of row x 1
	@return void
	*/
    static void project(const float* P, int row, int col, const float* x, float* out)
    {
        //
        memset(out, 0, sizeof(float)*row);
        for(int i = 0; i < row; i++)
            for(int j = 0; j < col; j++)
                out[i] += P[i*col+j]*x[j];
    }

	/**
	@brief execute external commands
	@param cmd the command to eval
	*/
    static void exec(std::string cmd)
    {
        int ret = system((cmd + " 2> log").c_str());
        if (ret != 0)
        {
            printf("Error when excuting the following command: \n%s\n", cmd.c_str());
        }
    }

	/**
	@brief calculate random permutation
	@param n a number indicating the permutation range
	@param seed a random seed for random function. Not used under Windows.
	@return a random perm of [0 1 2... n-1]
	*/
    static int* rand_perm(int n, unsigned int seed)
    {
        int *perm = new int[n];
        for (int i = 0; i < n; i++)
            perm[i] = i;

        int tmp;
        srand(time(NULL));
        for (int i = 0; i < n-1 ; i++)
        {
            int j = i +  rand() % (n - i);
            //swap
            tmp = perm[i];
            perm[i] = perm[j];
            perm[j] = tmp;
        }

        return perm;
    }

    /**
    @brief calc squared l2 distance
    */
    static float dist_l2_sq(const float* a, const float* b, int d)
    {
        float dist = 0.0f;
        for(int i = 0; i<d; i++)
        {
            dist += (a[i] - b[i])*(a[i] - b[i]);
            //std::cout << dist << std::endl;
        }

        return dist;
    }


	/**
	@brief count number of physical cpu cores
	*/
    static int count_cpu ()
    {
        /*
        SYSTEM_INFO sysinfo;
        GetSystemInfo( &sysinfo );

        int numCPU = sysinfo.dwNumberOfProcessors;
        return numCPU;
        */
        return 2;
    }


	/*
	@brief get total memory in megabytes.
	*/
    /*static int getTotalSystemMemory()
    {
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return status.ullTotalPhys/1024/1024;
    }*/

    /*static int getTotalSystemMemory()
    {
        struct sysinfo myinfo;
        sysinfo(&myinfo);

        return (int)(myinfo.mem_unit * myinfo.totalram / 1024 / 1024);
    }*/

    /**
    @brief calc combination of C(n, r).
	@remark calc C(n, r) in double, in case return value is too large to fit into a long
    */
    static double combination (int n, int r)
    {
        double ret = 1;
        for(int i = r+1; i<n+1; i++)
            ret *= (double)i;

        for(int i = 1; i<n-r+1; i++)
            ret /= (double)i;

        return ret;
    }

	/**
	@brief sleep for some miliseconds
	*/
    static void uSleep(int waitTime)
    {
        /*
        __int64 time1 = 0, time2 = 0, sysFreq = 0;

        QueryPerformanceCounter((LARGE_INTEGER *) &time1);
        QueryPerformanceFrequency((LARGE_INTEGER *)&sysFreq);

        do
        {
            QueryPerformanceCounter((LARGE_INTEGER *) &time2);
        } while((time2-time1) < waitTime);
        */
        sleep(waitTime/1000.0);
    }
};

#endif // UTIL_H_INCLUDED









