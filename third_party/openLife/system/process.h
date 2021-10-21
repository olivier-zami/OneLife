//
// Created by olivier on 20/10/2021.
//

#ifndef ONELIFE_PROCESS_H
#define ONELIFE_PROCESS_H

#ifndef OPENLIFE_SYSTEM_PROCESS_H
#define OPENLIFE_SYSTEM_PROCESS_H

namespace openLife::system
{
	void setString(char** string, unsigned int systemValue);
	char* getApplicationFilename();
	char* getApplicationDirectory();
}

#endif //OPENLIFE_SYSTEM_PROCESS_H

#endif //ONELIFE_PROCESS_H
