//
// Created by olivier on 27/11/2021.
//

#include "channel.h"

#include <cstring>
#include "OneLife/gameSource/dataTypes/controllers.h"
#include "OneLife/gameSource/dataTypes/signals.h"

using CONTROLLER = OneLife::dataValue::Controller::Type;
using SIGNAL = OneLife::dataValue::Signal;

const unsigned int OneLife::game::Channel::SIZE_MESSAGE = 255;

OneLife::game::Channel::Channel()
{
	this->lastSignal.type = SIGNAL::NONE;
	this->lastSignal.target = CONTROLLER::NONE;
	memset(this->lastSignal.message, 0, sizeof(this->lastSignal.message));
	this->lastSignalValue = SIGNAL::NONE;
	memset(this->waitingMessage, 0, sizeof(this->waitingMessage));
}

OneLife::game::Channel::~Channel(){}

void OneLife::game::Channel::setLastSignal(unsigned int lastSignal, unsigned int target, const char* message)
{
	this->lastSignal.type = lastSignal;
	this->lastSignal.target = target;
	if(message)
	{
		memset(this->lastSignal.message, 0, sizeof(this->lastSignal.message));
		strcpy((char*)this->lastSignal.message, message);
	}
}

OneLife::dataType::Signal OneLife::game::Channel::getLastSignal()
{
	return this->lastSignal;
}

void OneLife::game::Channel::stackMessage(OneLife::dataType::Message message)
{
	this->message.push(message);
}

OneLife::dataType::Message OneLife::game::Channel::unstackMessage()
{
	OneLife::dataType::Message message = {0, 0, nullptr};
	if(!this->message.empty())
	{
		message = this->message.front();
		this->message.pop();
	}
	return message;
}

void OneLife::game::Channel::setInitMapMessage(const char* message)
{
	//TODO: test strlen(message)<32
	strcpy(this->waitingMessage, message);
}

void OneLife::game::Channel::setExitMapMessage(const char* message)
{
	//TODO: test strlen(message)<32
	strcpy(this->waitingMessage, message);
}

void OneLife::game::Channel::insert(OneLife::dataType::Message* message){}
OneLife::dataType::Message* OneLife::game::Channel::output(){}