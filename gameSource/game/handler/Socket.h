//
// Created by olivier on 23/08/2022.
//

#ifndef oneLife_client_game_handler_socket_H
#define oneLife_client_game_handler_socket_H

//#include "../../../commonSource/handler/Message.h"TODO merge message_client.h with message in common/dataType

namespace oneLife::client::game::handler
{
	class Socket
	{
		public:
			Socket();
			~Socket();

			void close();
			void disconnect();
			void connect(char *ip, int port);
			double getClosePeriod();
			bool isClosed();
			bool isConnected();
			bool isLastSendSucceed();
			void read();
			Socket* sendMessage(char* message);

			struct{
				char* ip;
				int port;
			}address;
			double closeTime;
			bool lastSendSuccess;
			int socketId;
	};
}

char *getNextServerMessage();
char *getNextServerMessageRaw();
char readServerSocketFull( int inServerSocket );
void replaceLastMessageSent( char *inNewMessage );
void startConnecting();

#endif //oneLife_client_game_handler_socket_H
