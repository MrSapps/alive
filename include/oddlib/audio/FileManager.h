#pragma once

#include <stdio.h>
#include <iostream>
#include <malloc.h>
#include <string>
#include <fstream>
#include <streambuf>

class mgFileManager
{
public:
	static char * ReadFileToString(const char * filePath);
	static std::string ReadFileToString(std::string filePath);
};

