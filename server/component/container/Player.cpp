//
// Created by olivier on 20/04/2022.
//

#include "Player.h"

#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../dataType/info.h"

extern SimpleVector<DeadObject> pastPlayers;
extern SimpleVector<LiveObject> players;

/**
 *
 * @param inPlayer
 * @note from server/server.cpp => server/main.cpp
 */
void addPastPlayer( LiveObject *inPlayer )
{
	DeadObject o;

	o.id = inPlayer->id;
	o.displayID = inPlayer->displayID;
	o.name = NULL;
	if( inPlayer->name != NULL ) {
		o.name = stringDuplicate( inPlayer->name );
	}
	o.lineageEveID = inPlayer->lineageEveID;
	o.lifeStartTimeSeconds = inPlayer->lifeStartTimeSeconds;
	o.deathTimeSeconds = inPlayer->deathTimeSeconds;

	o.lineage = new SimpleVector<int>();
	for( int i=0; i< inPlayer->lineage->size(); i++ ) {
		o.lineage->push_back( inPlayer->lineage->getElementDirect( i ) );
	}

	pastPlayers.push_back( o );
}

/**
 *
 * @param inID
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
LiveObject *getLiveObject( int inID )
{
	for( int i=0; i<players.size(); i++ ) {
		LiveObject *o = players.getElement( i );

		if( o->id == inID ) {
			return o;
		}
	}

	return NULL;
}