//
// Created by olivier on 22/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_SOCKET_H
#define ONELIFE_SERVER_COMPONENT_SOCKET_H

#include "../dataType/LiveObject.h"

namespace OneLife::server
{
	class Socket
	{

	};
}

int sendMapChunkMessage( LiveObject *inO, char inDestOverride = false, int inDestOverrideX = 0, int inDestOverrideY = 0 );
void sendMessageToPlayer( LiveObject *inPlayer, char *inMessage, int inLength );
char *getNextClientMessage( SimpleVector<char> *inBuffer );
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer );
void sendGlobalMessage( char *inMessage );
void sendCraving( LiveObject *inPlayer );
void setPlayerDisconnected( LiveObject *inPlayer, const char *inReason );

#endif //ONELIFE_SERVER_COMPONENT_SOCKET_H
