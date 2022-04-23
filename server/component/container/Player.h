//
// Created by olivier on 20/04/2022.
//

#ifndef ONELIFE_SERVER_CONTAINER_PLAYER_H
#define ONELIFE_SERVER_CONTAINER_PLAYER_H

#include "../../dataType/LiveObject.h"

typedef struct DeadObject
{
	int id;
	int displayID;
	char *name;
	SimpleVector<int> *lineage;
	// id of Eve that started this line
	int lineageEveID;

	// time that this life started (for computing age)
	// not actual creation time (can be adjusted to tweak starting age,
	// for example, in case of Eve who starts older).
	double lifeStartTimeSeconds;

	// time this person died
	double deathTimeSeconds;
}DeadObject;

namespace OneLife::server::container
{
	class Player
	{

	};
}

void addPastPlayer( LiveObject *inPlayer );
LiveObject *getLiveObject( int inID );

#endif //ONELIFE_SERVER_CONTAINER_PLAYER_H
