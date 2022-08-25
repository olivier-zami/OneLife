//
// Created by olivier on 22/08/2022.
//

#ifndef oneLife_server_game_listener_socket_H
#define oneLife_server_game_listener_socket_H

#include "../../../third_party/minorGems/network/Socket.h"
#include "../../../third_party/minorGems/network/SocketPoll.h"
#include "../../../third_party/minorGems/network/SocketServer.h"
#include "../../../commonSource/dataType/message.h"
#include "../../../commonSource/handler/message/SequenceNumber.h"
#include "../dataType/connection.h"

namespace oneLife::server::game::handler
{
	class Socket:
		public oneLife::handler::message::SequenceNumber
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
			//using oneLife::handler::message::SequenceNumber::sendMessage;
			void setMaximumConnectionListened(int maxConnection);
			void setPort(int port);
			void to(FreshConnection newConnection);

			void initSocketServer();

		//protected:
			int maxConnection;
			bool isClientKnown;
			bool isLastConnectionAccepted;
			bool isConnectionRequestReceived;
			struct {char* address; int port;} lastClientListened;
			::Socket* lastClientSocket;
			char *outputMessage;
			double pollTimeout;
			int port;
			SocketOrServer *readySock;
			SocketServer *socketServer;
	};
}

#endif //oneLife_server_game_listener_socket_H
