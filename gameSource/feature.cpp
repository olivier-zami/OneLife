//
// Created by olivier on 02/12/2021.
//

#include "feature.h"

#include <cstring>
#include <cstdio>

#include "OneLife/gameSource/procedures/socket.h"
#include "OneLife/gameSource/dataTypes/exception/exception.h"
#include "OneLife/gameSource/dataTypes/feature.h"
#include "OneLife/gameSource/dataTypes/message.h"

#include "features/base.h"

OneLife::game::Feature::Feature() {}
OneLife::game::Feature::~Feature() {}

void OneLife::game::Feature::readSocketMessage(char* socketMessage)
{
	memset(this->lastSocketMessage, 0, sizeof(this->lastSocketMessage));
	if(strlen(socketMessage) > 1024)//TODO: create global var for socket message max size
	{
		throw new OneLife::game::Exception("Receive server message exceed socket message size limit ...");
	}
	strcpy(this->lastSocketMessage, socketMessage);
}

OneLife::dataType::Message OneLife::game::Feature::getGameMessage()
{
	OneLife::dataType::Message message = {0};
	switch(this->type)
	{
		case OneLife::dataType::feature::Type::BASE:
			message = ((OneLife::game::feature::Base*)this)->getGameMessage();
			break;
	}
	return message;
}

unsigned int OneLife::game::Feature::getType() const
{
	return this->type;
}

/**********************************************************************************************************************/
//!private

bool OneLife::game::Feature::isMessageTypeMatch(const char* inMessageType)
{
	return OneLife::socket::isMessageHeaderMatchMessage(inMessageType, this->lastSocketMessage);
}
