//
// Created by olivier on 22/08/2022.
//

#include "Socket.h"

SocketPoll sockPoll;

oneLife::server::game::listener::Socket::Socket()
{
	this->readySock = nullptr;
}

oneLife::server::game::listener::Socket::~Socket() {}

void oneLife::server::game::listener::Socket::listen()
{
	this->readySock = nullptr;
	this->readySock = sockPoll.wait((int)(this->pollTimeout * 1000));
}

bool oneLife::server::game::listener::Socket::isUnknownConnectionRequestReceived()
{
	return (bool) (this->readySock != NULL && !this->readySock->isSocket);
}
