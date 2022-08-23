//
// Created by olivier on 23/08/2022.
//

#include "Message.h"

oneLife::handler::Message::Message() {}

oneLife::handler::Message::~Message() {}

char* oneLife::handler::Message::getInputMessage()
{
	return this->inputMessage;
}

char* oneLife::handler::Message::getOutputMessage()
{
	return this->outputMessage;
}

oneLife::handler::Message* oneLife::handler::Message::sendMessage(char *message)
{
	this->outputMessage = message;
}