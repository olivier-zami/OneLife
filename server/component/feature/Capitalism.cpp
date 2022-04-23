//
// Created by olivier on 22/04/2022.
//

#include "Capitalism.h"

#include "../../../gameSource/transitionBank.h"
#include "../../../third_party/minorGems/util/log/AppLog.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../../third_party/minorGems/util/stringUtils.h"
#include "../../component/Map.h"

extern SimpleVector<LiveObject> players;
extern SimpleVector<GridPos> recentlyRemovedOwnerPos;

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char *getOwnershipString( int inX, int inY )
{
	SimpleVector<char> messageWorking;

	for( int j=0; j<players.size(); j++ ) {
		LiveObject *otherPlayer = players.getElement( j );
		if( ! otherPlayer->error &&
			isOwned( otherPlayer, inX, inY ) ) {
			char *playerIDString =
					autoSprintf( " %d", otherPlayer->id );
			messageWorking.appendElementString(
					playerIDString );
			delete [] playerIDString;
		}
	}
	char *message = messageWorking.getElementString();
	return message;
}

/**
 *
 * @param inPos
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char *getOwnershipString( GridPos inPos )
{
	return getOwnershipString( inPos.x, inPos.y );
}

/**
 *
 * @param inPlayer
 * @note from server/server.cpp => server/main.cpp
 */
void removeAllOwnership( LiveObject *inPlayer )
{
	double startTime = Time::getCurrentTime();
	int num = inPlayer->ownedPositions.size();

	for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
		GridPos *p = inPlayer->ownedPositions.getElement( i );

		recentlyRemovedOwnerPos.push_back( *p );

		int oID = getMapObject( p->x, p->y );

		if( oID <= 0 ) {
			continue;
		}

		char noOtherOwners = true;

		for( int j=0; j<players.size(); j++ ) {
			LiveObject *otherPlayer = players.getElement( j );

			if( otherPlayer != inPlayer ) {
				if( isOwned( otherPlayer, *p ) ) {
					noOtherOwners = false;
					break;
				}
			}
		}

		if( noOtherOwners ) {
			// last owner of p just died
			// force end transition
			SimpleVector<int> *deathMarkers = getAllPossibleDeathIDs();
			for( int j=0; j<deathMarkers->size(); j++ ) {
				int deathID = deathMarkers->getElementDirect( j );
				TransRecord *t = getTrans( deathID, oID );

				if( t != NULL ) {

					setMapObject( p->x, p->y, t->newTarget );
					break;
				}
			}
		}
	}

	inPlayer->ownedPositions.deleteAll();

	AppLog::infoF( "Removing all ownership (%d owned) for "
				   "player %d (%s) took %lf sec",
				   num, inPlayer->id, inPlayer->email,
				   Time::getCurrentTime() - startTime );
}

/**
 *
 * @param inPlayer
 * @param inX
 * @param inY
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isKnownOwned( LiveObject *inPlayer, int inX, int inY )
{
	for( int i=0; i<inPlayer->knownOwnedPositions.size(); i++ ) {
		GridPos *p = inPlayer->knownOwnedPositions.getElement( i );

		if( p->x == inX && p->y == inY ) {
			return true;
		}
	}
	return false;
}

/**
 *
 * @param inPlayer
 * @param inPos
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isKnownOwned( LiveObject *inPlayer, GridPos inPos )
{
	return isKnownOwned( inPlayer, inPos.x, inPos.y );
}

/**
 *
 * @param inPlayer
 * @param inX
 * @param inY
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isOwned( LiveObject *inPlayer, int inX, int inY )
{
	for( int i=0; i<inPlayer->ownedPositions.size(); i++ ) {
		GridPos *p = inPlayer->ownedPositions.getElement( i );

		if( p->x == inX && p->y == inY ) {
			return true;
		}
	}
	return false;
}

/**
 *
 * @param inPlayer
 * @param inPos
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isOwned( LiveObject *inPlayer, GridPos inPos )
{
	return isOwned( inPlayer, inPos.x, inPos.y );
}