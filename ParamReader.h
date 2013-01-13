/**
@file ParamReader.h
@brief this file defines a parameter loader class.
@author wzhang
@date May 7, 2012
*/

#ifndef _CPARAMREADER_H
#define _CPARAMREADER_H

/**
 Notes:
 1. This class is used to read a configuration file
 2. The format for the configuration is as follow:
    param1 = setting1
    param2 = setting2
 3. To use
    CParamReader *params	= new CParamReader ("config.txt");  // Declaration

    int maxNumOfStudents	= params->GetParamInt  ("max_students");
    float simThres		= params->GetParamFlt  ("simThres");
    string name			= params->GetParamStr  ("studentname");
    bool bDisplay			= params->GetParamBool ("bDisplay");

    params->print();   // Display to screen
*/

#include <cstdio>
#include <map>
#include <cstring>

#include "util.h"

using std::string;
using std::map;

/// this class wraps functions that read configurations/parameters from config file.
class CParamReader
{
	///parameter <key,value> table
	map<string, string> params;

public:

	/// destructor
	~CParamReader ();

	/// constructor
	CParamReader (string paramFileName);


	/**
	@brief read parameter file
	@param paramFileName config file to read
	@return void
	*/
	void ReadParamFile (string paramFileName);


	/// get a string from config file
	string GetStr (string param);

	/// get an interger from config file
	int GetInt (std::string param);

	/// gen a float from config file
	float GetFlt (std::string param);

	/// print all paramters loaded from the config file
	void print();
};

#endif
