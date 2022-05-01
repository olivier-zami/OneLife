//
// Created by olivier on 20/04/2022.
//

#ifndef ONELIFE_DATATYPE__SERVERSETTINGS_H
#define ONELIFE_DATATYPE__SERVERSETTINGS_H

#include <string>

namespace OneLife::server::settings
{
	typedef struct WorldMap
	{
		int flushLookTimes;
		int skipLookTimeCleanup;
		double maxLoadForOpenCalls;
		int staleSec;
		struct {
			struct{
				char* url;
			}lookTime;
			struct{
				char* url;
			}biome;
		}database;
	}WorldMap;
}

namespace OneLife::server
{
	typedef struct Settings
	{
		int shutdownMode;
		char someClientMessageReceived;
		int forceShutdownMode;
		int versionNumber;
		int port;
		std::string strInfertilitySuffix;

		OneLife::server::settings::WorldMap worldMap;
	}Settings;
}

#endif //ONELIFE_DATATYPE__SERVERSETTINGS_H
