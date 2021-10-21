//
// Created by olivier on 20/10/2021.
//

#include "system.h"

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "third_party/openLife/system/_base/entity/exception.h"

void* openLife::System::memory = nullptr;
void* openLife::System::ptr = nullptr;
char* openLife::System::string[OPENLIFE_SYSTEM_VAR_STRING_NUMBER];

openLife::System* openLife::System::instance = new openLife::System();

const unsigned int openLife::System::CURRENT_APPLICATION_FILENAME = 1;

openLife::System::System()
{
	openLife::System::memory = malloc(1024);
	memset(openLife::System::memory, 0, 1024);
	openLife::System::ptr = openLife::System::memory;
	for(unsigned int i=0; i<OPENLIFE_SYSTEM_VAR_STRING_NUMBER; i++)
	{
		openLife::System::string[i] = nullptr;
	}
}

openLife::System::~System()
{
	free(openLife::System::memory);
}

void openLife::System::setString(unsigned int id, char *string)
{
	size_t size;
	size = strlen(string);
	if(id>=OPENLIFE_SYSTEM_VAR_STRING_NUMBER)
	{
		openLife::System::instance->~System();
		throw new openLife::system::Exception();
	}

	if(openLife::System::string[id])
	{
		openLife::System::instance->~System();
		throw new openLife::system::Exception();
	}

	strncpy((char*)openLife::System::ptr, string, size);
	openLife::System::string[id] = (char*)openLife::System::ptr;
	openLife::System::ptr = (void*)((intptr_t)openLife::System::ptr + (intptr_t)(size+1));
}

char *openLife::System::getString(unsigned int id)
{
	return openLife::System::string[id];
}