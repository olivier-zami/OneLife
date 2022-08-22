//
// Created by olivier on 22/08/2022.
//

#ifndef oneLife_server_game_listener_socket_H
#define oneLife_server_game_listener_socket_H

#include "../../../third_party/minorGems/network/Socket.h"
#include "../../../third_party/minorGems/network/SocketPoll.h"
#include "../../../third_party/minorGems/network/SocketServer.h"

namespace oneLife::server::game::listener
{
	class Socket
	{
		public:
			Socket();
			~Socket();

			void listen();

			const char* getLastClientListenedAddress();
			int getLastClientListenedPort();
			::Socket* getLastClientSocket();
			int getPort();
			bool isConnectionRequestAccepted();
			bool isUnknownClientConnectionRequestReceived();
			void setMaximumConnectionListened(int maxConnection);
			void setPort(int port);

			void initSocketServer();

		//protected:
			int maxConnection;
			bool isClientKnown;
			bool isLastConnectionAccepted;
			bool isConnectionRequestReceived;
			struct {char* address; int port;} lastClientListened;
			::Socket* lastClientSocket;
			double pollTimeout;
			int port;
			SocketOrServer *readySock;
			SocketServer *socketServer;
	};
}

#endif //oneLife_server_game_listener_socket_H
