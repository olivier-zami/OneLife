//
// Created by olivier on 18/04/2022.
//

#ifndef ONELIFE_SERVER_H
#define ONELIFE_SERVER_H

#include "../gameSource/GridPos.h"
#include "dataType/LiveObject.h"
#include "dataType/Settings.h"

namespace OneLife
{
	class Server
	{
		public:
			Server(OneLife::server::Settings settings);
			~Server();

			void start();

			bool initMap();
			void routine_dbPut();

			OneLife::server::Settings settings;
	};
}

void writeRecentPlacements();

#endif //ONELIFE_SERVER_H
