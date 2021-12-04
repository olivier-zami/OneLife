//
// Created by olivier on 03/12/2021.
//

#include "../socket.h"

#include <cstring>

bool OneLife::socket::isMessageHeaderMatchMessage(const char* messageHeader, char* message)
{
	char messageType[5];
	memset(messageType, 0, sizeof(messageType));
	for(unsigned int i=0; i<5; i++)
	{
		if(message[i]=='\n')break;
		messageType[i] = message[i];
	}
	return !strcmp( messageType, messageHeader );
}
