//
// Created by olivier on 27/11/2021.
//

#include "messageChannel.h"

#include <cstring>
#include "OneLife/gameSource/dataTypes/signals.h"

using signal = OneLife::dataType::Signal;

OneLife::game::component::MessageChannel::MessageChannel()
{
	this->lastSignalValue = signal::NONE;
	memset(this->waitingMessage, 0, sizeof(this->waitingMessage));
}

OneLife::game::component::MessageChannel::~MessageChannel() {}

void OneLife::game::component::MessageChannel::setLastSignal(unsigned int lastSignal)
{
	this->lastSignalValue = lastSignal;
}

unsigned int OneLife::game::component::MessageChannel::getLastSignal()
{
	return this->lastSignalValue;
}

void OneLife::game::component::MessageChannel::stackMessage(OneLife::dataType::Message message)
{
	this->message.push(message);
}

OneLife::dataType::Message OneLife::game::component::MessageChannel::unstackMessage()
{
	OneLife::dataType::Message message = {0, 0, nullptr};
	if(!this->message.empty())
	{
		message = this->message.front();
		this->message.pop();
	}
	return message;
}

void OneLife::game::component::MessageChannel::setInitMapMessage(const char* message)
{
	//TODO: test strlen(message)<32
	strcpy(this->waitingMessage, message);
}

void OneLife::game::component::MessageChannel::setExitMapMessage(const char* message)
{
	//TODO: test strlen(message)<32
	strcpy(this->waitingMessage, message);
}

void OneLife::game::component::MessageChannel::insert(OneLife::dataType::Message* message){}
OneLife::dataType::Message* OneLife::game::component::MessageChannel::output(){}