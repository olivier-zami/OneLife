//
// Created by olivier on 24/10/2021.
//

#ifndef ONELIFE_DATA_WEB_H
#define ONELIFE_DATA_WEB_H

#include "minorGems/network/web/WebRequest.h"

typedef struct WebRequestRecord {
	int handle;
	WebRequest *request;
} WebRequestRecord;

typedef struct SocketConnectionRecord {
	int handle;
	Socket *sock;
} SocketConnectionRecord;

#endif //ONELIFE_DATA_WEB_H
