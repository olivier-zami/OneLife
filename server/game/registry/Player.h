//
// Created by olivier on 22/08/2022.
//

#ifndef oneLife_server_game_registry_player_H
#define oneLife_server_game_registry_player_H

namespace oneLife::server::game::registry
{
	class Player {
		public:
			Player();
			~Player();

			bool isAnyTicketServerRequestsOut();

	};
}

#endif //oneLife_server_game_registry_player_H
