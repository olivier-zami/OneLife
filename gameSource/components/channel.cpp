//
// Created by olivier on 27/11/2021.
//

#include "channel.h"

#include <cstring>
#include "OneLife/gameSource/dataTypes/signals.h"

using signal = OneLife::dataType::Signal;

OneLife::game::Channel::Channel()
{
	this->lastSignalValue = signal::NONE;
	memset(this->waitingMessage, 0, sizeof(this->waitingMessage));
}

OneLife::game::Channel::~Channel() {}

void OneLife::game::Channel::setLastSignal(unsigned int lastSignal)
{
	this->lastSignalValue = lastSignal;
}

unsigned int OneLife::game::Channel::getLastSignal()
{
	return this->lastSignalValue;
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