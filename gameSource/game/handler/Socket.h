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
			bool isClosed();
			bool isLastSendSucceed();
			Socket* sendMessage(char* message);

			bool lastSendSuccess;
			int* mServerSocket;//TODO: remove all LivingLifePage instance and convert to int
	};
}

char *getNextServerMessage();
char *getNextServerMessageRaw();
char readServerSocketFull( int inServerSocket );
void replaceLastMessageSent( char *inNewMessage );

#endif //oneLife_client_game_handler_socket_H
