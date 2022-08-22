//
// Created by olivier on 22/08/2022.
//

#include "Player.h"

#include "minorGems/util/SimpleVector.h"

#include "../../dataType/connection.h"

SimpleVector<FreshConnection> newConnections;

oneLife::server::game::registry::Player::Player() {}

oneLife::server::game::registry::Player::~Player() {}

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