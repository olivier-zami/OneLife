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
			void connect(char *ip, int port);
			bool isClosed();
			bool isConnected();
			bool isLastSendSucceed();
			Socket* sendMessage(char* message);

			struct{
				char* ip;
				int port;
			}address;
			bool lastSendSuccess;
			int* mServerSocket;//TODO: remove all LivingLifePage instance and convert to int
			int socketId;
	};
}

char *getNextServerMessage();
char *getNextServerMessageRaw();
char readServerSocketFull( int inServerSocket );
void replaceLastMessageSent( char *inNewMessage );
void startConnecting();

#endif //oneLife_client_game_handler_socket_H
