//
// Created by olivier on 20/04/2022.
//

#ifndef ONELIFE_DATATYPE__SERVERSETTINGS_H
#define ONELIFE_DATATYPE__SERVERSETTINGS_H

#include <string>

#include "settings/map.h"

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
		int codeVersion;
		int dataVersion;
		int flushLookTimes;
		char* lookTimeDbName;
		SimpleVector<int> *mapBiomeOrder;
		SimpleVector<float> *mapBiomeWeight;
		SimpleVector<int> *mapBiomeSpecial;
		double maxLoadForOpenCalls;
		int maxPlayers;
		char* secretString;
		int shutdownMode;
		char someClientMessageReceived;
		int staleSec;
		int forceShutdownMode;
		int versionNumber;
		int port;
		int skipLookTimeCleanup;
		std::string strInfertilitySuffix;
	}Settings;
}

#endif //ONELIFE_DATATYPE__SERVERSETTINGS_H
