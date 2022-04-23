//
// Created by olivier on 20/04/2022.
//

#ifndef ONELIFE_DATATYPE__SERVERSETTINGS_H
#define ONELIFE_DATATYPE__SERVERSETTINGS_H

#include <string>

namespace OneLife::server
{
	typedef struct
	{
		int shutdownMode;
		char someClientMessageReceived;
		int forceShutdownMode;
		int versionNumber;
		int port;
		std::string strInfertilitySuffix;
	}Settings;
}

#endif //ONELIFE_DATATYPE__SERVERSETTINGS_H
