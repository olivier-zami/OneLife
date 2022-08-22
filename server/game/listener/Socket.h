//
// Created by olivier on 22/08/2022.
//

#ifndef oneLife_server_game_listener_socket_H
#define oneLife_server_game_listener_socket_H

#include "minorGems/network/SocketPoll.h"

namespace oneLife::server::game::listener
{
	class Socket
	{
		public:
			Socket();
			~Socket();

			void listen();

			bool isUnknownConnectionRequestReceived();

		//protected:
			double pollTimeout;
			SocketOrServer *readySock;

	};
}

#endif //oneLife_server_game_listener_socket_H
