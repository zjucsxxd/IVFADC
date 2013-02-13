/**
@file ParamReader.cpp
@brief this file implements ParamReader.h
@author wzhang
@date May 7, 2012
*/

#include <cstdio>
#include <map>
#include <cstring>

#include "util.h"
#include "ParamReader.h"

using std::string;
using std::map;




/// destructor
CParamReader::~CParamReader ()
{
    params.clear();
}

/// constructor
CParamReader::CParamReader (string paramFileName)
{
    ReadParamFile (paramFileName);
}


/**
@brief read parameter file
@param paramFileName config file to read
@return void
*/
void CParamReader::ReadParamFile (string paramFileName)
{
    string seps = "=", var;
    FILE* fin = fopen(paramFileName.c_str(), "r");
    //FILE* fin = fopen("/Users/nebula/mylab/copyDetection/xcode_search/IVFADC/sample.config", "r");
    if (!fin)
        printf("Config file not found: %s\n", paramFileName.c_str());

    const int max_len = 2000;
    char* buffer = new char[max_len];

    while ( fgets(buffer, max_len, fin) )
    {
        string line(buffer);
        if(line[0] == '\n' || line[0] == '#')
            continue;

        var = Util::trim(Util::strtok(line, seps));
        line = Util::trim(line).substr(0, line.length() -2);

        if( params.find(var) != params.end() )
        {
            printf("redefinition of parameter: %s\n", var.c_str());
            fflush(stdout);
            exit(1);
        }
        params.insert( std::pair<string, string>(var, line) );
    }

    delete[] buffer;

    fclose(fin);
}


/// get a string from config file
string CParamReader::GetStr (string param)
{
    if ( params.find(param) != params.end() )
        return params[param];
    else
    {
        printf("error: undefined %s.\n", param.c_str());
        exit(1);
    }
}

/// get an interger from config file
int CParamReader::GetInt (std::string param)
{
    if ( params.find(param) != params.end() )
        return atoi(params[param].c_str());
    else
    {
        printf("error: undefined %s.\n", param.c_str());
        exit(1);
    }

}

/// gen a float from config file
float CParamReader::GetFlt (std::string param)
{
    if ( params.find(param) != params.end() )
        return atof(params[param].c_str());
    else
    {
        printf("error: undefined %s.\n", param.c_str());
        exit(1);
    }
}

/// print all paramters loaded from the config file
void CParamReader::print()
{
    printf("Parameters loaded:\n");
    for (map<string, string>::iterator it = params.begin(); it != params.end(); it++ )
        printf("%s  =  %s\n", it->first.c_str(), it->second.c_str());

    printf("\n"); fflush(stdout);
}
