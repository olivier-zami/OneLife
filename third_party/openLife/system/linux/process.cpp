//
// Created by olivier on 20/10/2021.
//

#include "../_init.h"
#include "../process.h"

#if defined(SYSTEM)&&(SYSTEM==linux)

#define osLinux system



#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <cstring>
#include <cstdio>
#include "third_party/openLife/system.h"
#include "third_party/openLife/system/_base/entity/exception.h"

void openLife::system::setString(char** string, unsigned int systemValue)
{
	char buffer[PATH_MAX];
	memset(buffer, 0, sizeof(buffer)); // readlink does not null terminate!
	if (readlink("/proc/self/exe", buffer, PATH_MAX) == -1) throw new openLife::system::Exception("readlink : unable to read application name");
	if(!openLife::System::getString(openLife::System::CURRENT_APPLICATION_FILENAME))
	{
		openLife::System::setString(openLife::System::CURRENT_APPLICATION_FILENAME, buffer);
	}
	*string = openLife::System::getString(openLife::System::CURRENT_APPLICATION_FILENAME);
}

char* openLife::system::getApplicationFilename()
{
	return openLife::System::getString(openLife::System::CURRENT_APPLICATION_FILENAME);
}

char *openLife::system::getApplicationDirectory()
{
	return dirname(openLife::system::getApplicationFilename());
}

#undef osLinux

#endif
