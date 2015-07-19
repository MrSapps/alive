#include "oddlib/audio/FileManager.h"

char * mgFileManager::ReadFileToString(const char * filePath)
{
	FILE *f = fopen(filePath, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char *string = (char*)malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;

	return string;
}

std::string mgFileManager::ReadFileToString(std::string filePath)
{
	std::ifstream t(filePath);
	t.seekg(0, std::ios::end);
	size_t fileSize = t.tellg();
	std::string buffer(fileSize, ' ');
	t.seekg(0);
	t.read(&buffer[0], fileSize);
	return buffer;
}