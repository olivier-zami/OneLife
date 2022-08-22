//
// Created by olivier on 22/08/2022.
//

#include "Socket.h"

#include "../../../third_party/minorGems/network/HostAddress.h"

SocketPoll sockPoll;

oneLife::server::game::listener::Socket::Socket()
{
	this->isConnectionRequestReceived = false;
	this->lastClientListened = {nullptr, 0};
	this->lastClientSocket = nullptr;
	this->maxConnection = 0;
	this->port = 0;
	this->readySock = nullptr;
	this->socketServer = nullptr;
}

/**
 *
 * @note from server.cpp
 * // stop listening on server socket immediately, before running
// cleanup steps.  Cleanup may take a while, and we don't want to leave
// server socket listening, because it will answer reflector and player
// connection requests but then just hang there.

// Closing the server socket makes these connection requests fail
// instantly (instead of relying on client timeouts).
 */
oneLife::server::game::listener::Socket::~Socket()
{
	if(this->lastClientListened.address)
	{
		delete[] this->lastClientListened.address;
		this->lastClientListened.address = nullptr;
	}
	if(this->socketServer) delete this->socketServer;
}

void oneLife::server::game::listener::Socket::listen()
{
	this->readySock = nullptr;
	if(!this->socketServer) this->initSocketServer();
	this->readySock = sockPoll.wait((int)(this->pollTimeout * 1000));

	this->isClientKnown = false;
	this->isLastConnectionAccepted = false;
	this->isConnectionRequestReceived = false;
	if(this->lastClientListened.address)
	{
		delete[] this->lastClientListened.address;
		this->lastClientListened.address = nullptr;
	}
	this->lastClientListened.port = 0;
	this->lastClientSocket = nullptr;

	if(this->readySock != NULL && !this->readySock->isSocket)
	{
		this->isConnectionRequestReceived = true;
		this->lastClientSocket = this->socketServer->acceptConnection(0);
		if(this->lastClientSocket)
		{
			this->isLastConnectionAccepted = true;
			HostAddress *client = this->lastClientSocket->getRemoteHostAddress();
			if(client)
			{
				this->isClientKnown = true;
				this->lastClientListened.address = new char[strlen(client->mAddressString)+1];
				strcpy(this->lastClientListened.address, client->mAddressString);
				this->lastClientListened.port = client->mPort;
			}
			else this->isClientKnown = false;
		}
		else
		{
			this->isLastConnectionAccepted = false;
		}
	}
}

const char* oneLife::server::game::listener::Socket::getLastClientListenedAddress()
{
	return this->lastClientListened.address;
}

int oneLife::server::game::listener::Socket::getLastClientListenedPort()
{
	return this->lastClientListened.port;
}

::Socket * oneLife::server::game::listener::Socket::getLastClientSocket()
{
	return this->lastClientSocket;
}

int oneLife::server::game::listener::Socket::getPort()
{
	return this->port;
}

bool oneLife::server::game::listener::Socket::isConnectionRequestAccepted()
{
	return this->isLastConnectionAccepted;
}

bool oneLife::server::game::listener::Socket::isUnknownClientConnectionRequestReceived()
{
	return (bool) (this->isLastConnectionAccepted && !this->isClientKnown);
}

void oneLife::server::game::listener::Socket::setMaximumConnectionListened(int maxConnection)
{
	this->maxConnection = maxConnection;
}

void oneLife::server::game::listener::Socket::setPort(int port)
{
	this->port = port;
}

void oneLife::server::game::listener::Socket::initSocketServer()
{
	this->socketServer = new SocketServer( this->port, this->maxConnection );
	sockPoll.addSocketServer( this->socketServer );
}
