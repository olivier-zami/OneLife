//
// Created by olivier on 30/08/2021.
//

#include "json.h"

#include <fstream>

using Json = nlohmann::json;

nlohmann::json oneLife::io::json::getObject(const char *filename)
{
	char *buffer;
	Json json;
	std::ifstream fileReader (filename);
	if(fileReader)
	{
		fileReader.seekg (0, fileReader.end);
		int length = fileReader.tellg();
		fileReader.seekg (0, fileReader.beg);
		buffer = new char [length];
		fileReader.read (buffer,length);
		fileReader.close();
		json = Json::parse(buffer);
		delete[] buffer;
	}
	return json;
}