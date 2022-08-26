//
// Created by olivier on 22/08/2022.
//

#include "Player.h"

#include "../../../third_party/minorGems/util/SimpleVector.h"

#include "../../../commonSource/dataType/socket.h"
#include "../dataType/connection.h"

SimpleVector<FreshConnection> newConnections;

oneLife::server::game::registry::Player::Player() {}

oneLife::server::game::registry::Player::~Player() {}

FreshConnection* oneLife::server::game::registry::Player::getNewConnection(char *address)
{
	FreshConnection* connection = nullptr;
	for(int i=0; i<newConnections.size(); i++)
	{
		if(!strcmp(address, newConnections.getElement(i)->sock->getRemoteHostAddress()->mAddressString))
		{
			connection = newConnections.getElement(i);
			break;
		}
	}
	return connection;
}

bool oneLife::server::game::registry::Player::isAnyTicketServerRequestsOut()
{
	bool anyTicketServerRequestsOut = false;
	for( int i=0; i<newConnections.size(); i++ )
	{
		FreshConnection *nextConnection = newConnections.getElement( i );
		if( nextConnection->ticketServerRequest != NULL )
		{
			anyTicketServerRequestsOut = true;
			break;
		}
	}
	return anyTicketServerRequestsOut;
}