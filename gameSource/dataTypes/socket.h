//
// Created by olivier on 24/10/2021.
//

#ifndef ONELIFE_DATA_WEB_H
#define ONELIFE_DATA_WEB_H

#include "minorGems/network/web/WebRequest.h"

namespace OneLife::game::dataType::socket
{
	typedef struct{
		char ip[16];
		int port;
	}Address;

	typedef struct{
		char* body;
	}Message;
}

namespace OneLife::game::dataType
{
	typedef struct serverSocket
	{
		char* ip;
		int port;
	}ServerSocket;
}

typedef struct WebRequestRecord {
	int handle;
	WebRequest *request;
} WebRequestRecord;

typedef struct SocketConnectionRecord {
	int handle;
	Socket *sock;
} SocketConnectionRecord;

typedef struct WebEvent {
	int handle;
	int type;
	char *bodyText;
	int bodyLength;
} WebEvent;

typedef struct SocketEvent {
	int handle;
	int type;
	int numBodyBytes;
	// can be NULL even if numBodyBytes not 0 (in case of
	// recorded send, where we don't need to record what was sent)
	char *bodyBytesHex;
} SocketEvent;

#endif //ONELIFE_DATA_WEB_H
