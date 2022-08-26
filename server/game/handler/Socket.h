//
// Created by olivier on 22/08/2022.
//

#ifndef oneLife_server_game_listener_socket_H
#define oneLife_server_game_listener_socket_H

#include "../../../third_party/minorGems/network/HostAddress.h"
#include "../../../third_party/minorGems/network/Socket.h"
#include "../../../third_party/minorGems/network/SocketPoll.h"
#include "../../../third_party/minorGems/network/SocketServer.h"
#include "../../../commonSource/dataType/connection.h"
#include "../../../commonSource/dataType/message.h"
#include "../../../commonSource/handler/message/ServerInfo.h"
#include "../../../commonSource/handler/message/SequenceNumber.h"
#include "../../dataType/LiveObject.h"
#include "../dataType/connection.h"

namespace oneLife::server::game::handler
{
	class Socket:
		public oneLife::handler::message::ServerInfo,
		public oneLife::handler::message::SequenceNumber
	{
		public:
			Socket();
			~Socket();

			void listen();

			::Socket* getLastConnection();
			int getPort();
			::oneLife::dataType::Connection getRemoteHost();
			bool isConnectionAccepted();
			bool isConnectionReceived();
			bool isRemoteHostFound();
			using oneLife::handler::message::SequenceNumber::sendMessage;
			using oneLife::handler::message::ServerInfo::sendMessage;
			void setMaximumConnectionListened(int maxConnection);
			void setPort(int port);
			void to(FreshConnection newConnection);

			void initSocketServer();

		//protected:
			int maxConnection;
			::Socket* lastConnection;
			char *outputMessage;
			double pollTimeout;
			int port;
			SocketOrServer *readySock;
			HostAddress *remoteHost;
			SocketServer *socketServer;
	};
}

int sendMapChunkMessage( LiveObject *inO, char inDestOverride = false, int inDestOverrideX = 0, int inDestOverrideY = 0 );
void sendMessageToPlayer( LiveObject *inPlayer, char *inMessage, int inLength );
char *getNextClientMessage( SimpleVector<char> *inBuffer );
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer );
void sendGlobalMessage( char *inMessage );
void sendCraving( LiveObject *inPlayer );
void setPlayerDisconnected( LiveObject *inPlayer, const char *inReason );

#endif //oneLife_server_game_listener_socket_H
