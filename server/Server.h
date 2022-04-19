//
// Created by olivier on 18/04/2022.
//

#ifndef ONELIFE_SERVER_H
#define ONELIFE_SERVER_H

namespace OneLife
{
	class Server
	{
		public:
			Server();
			~Server();

			bool initMap();
			void routine_dbPut();
	};
}

#endif //ONELIFE_SERVER_H
