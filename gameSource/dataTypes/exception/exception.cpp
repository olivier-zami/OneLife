//
// Created by olivier on 02/11/2021.
//

#include "exception.h"

#include <cstring>

OneLife::game::Exception::Exception(const char* message)
{
	memset(this->message, 0, sizeof(this->message));
	strcpy(this->message, message);
	this->message[254] = 0;
}

OneLife::game::Exception::~Exception() {}

const char* OneLife::game::Exception::getMessage()
{
	return this->message;
}