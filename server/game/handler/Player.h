//
// Created by olivier on 21/08/2022.
//

#ifndef oneLife_server_game_handler_player_H
#define oneLife_server_game_handler_player_H

#include "../../../gameSource/GridPos.h"
#include "../../curses.h"
#include "minorGems/network/Socket.h"
#include "minorGems/util/SimpleVector.h"

#include <cstdint>

namespace oneLife::server::game::handler
{
	class Player {};
}

int processLoggedInPlayer( char inAllowReconnect,
						   Socket *inSock,
						   SimpleVector<char> *inSockBuffer,
						   char *inEmail,
						   uint32_t hashedSpawnSeed,
						   int inTutorialNumber,
						   CurseStatus inCurseStatus,
						   float inFitnessScore,
		// set to -2 to force Eve
						   int inForceParentID = -1,
						   int inForceDisplayID = -1,
						   GridPos *inForcePlayerPos = NULL );


#endif //oneLife_server_game_handler_player_H
