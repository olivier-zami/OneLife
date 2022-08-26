//
// Created by olivier on 22/08/2022.
//

#ifndef oneLife_server_game_registry_player_H
#define oneLife_server_game_registry_player_H

#include "../../../commonSource/dataType/socket.h"

namespace oneLife::server::game::registry
{
	class Player {
		public:
			Player();
			~Player();

			void add();

			FreshConnection* getNewConnection(char* address);
			bool isAnyTicketServerRequestsOut();
	};
}

#endif //oneLife_server_game_registry_player_H
