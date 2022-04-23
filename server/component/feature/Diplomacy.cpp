//
// Created by olivier on 22/04/2022.
//

#include "Diplomacy.h"

#include "../../../third_party/minorGems/system/Time.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../component/Socket.h"
#include "../../dataType/LiveObject.h"

extern SimpleVector<LiveObject> players;

SimpleVector<WarPeaceMessageRecord> warPeaceRecords;
SimpleVector<PeaceTreaty> peaceTreaties;

/**
 *
 * @param inLineageAEveID
 * @param inLineageBEveID
 * @note from server/server.cpp => server/main.cpp
 */
void removePeaceTreaty( int inLineageAEveID, int inLineageBEveID )
{
	PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );

	char remove = false;

	if( p != NULL ) {
		if( p->dirAToB && p->dirBToA ) {
			// established

			// maybe it has been broken in a new direction?
			if( p->lineageAEveID == inLineageAEveID ) {
				p->dirAToBBroken = true;
			}
			if( p->lineageBEveID == inLineageAEveID ) {
				p->dirBToABroken = true;
			}

			if( p->dirAToBBroken && p->dirBToABroken ) {
				// fully broken
				// remove it
				remove = true;

				// new war!
				sendPeaceWarMessage( "WAR",
									 true,
									 p->lineageAEveID, p->lineageBEveID );
			}
		}
		else {
			// not fully established
			// remove it

			// this means if one person says PEACE and the other
			// responds with WAR, the first person's PEACE half-way treaty
			// is canceled.  Both need to say PEACE again once WAR has been
			// mentioned
			remove = true;
		}
	}

	if( remove ) {
		for( int i=0; i<peaceTreaties.size(); i++ ) {
			PeaceTreaty *otherP = peaceTreaties.getElement( i );

			if( otherP->lineageAEveID == p->lineageAEveID &&
				otherP->lineageBEveID == p->lineageBEveID ) {

				peaceTreaties.deleteElement( i );
				return;
			}
		}
	}
}

/**
 *
 * @param inLineageAEveID
 * @param inLineageBEveID
 * @note from server/server.cpp => server/main.cpp
 */
void addPeaceTreaty( int inLineageAEveID, int inLineageBEveID )
{
	PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );

	if( p != NULL ) {
		char peaceBefore = p->dirAToB && p->dirBToA;

		// maybe it has been sealed in a new direction?
		if( p->lineageAEveID == inLineageAEveID ) {
			p->dirAToB = true;
			p->dirBToABroken = false;
		}
		if( p->lineageBEveID == inLineageAEveID ) {
			p->dirBToA = true;
			p->dirBToABroken = false;
		}
		if( p->dirAToB && p->dirBToA &&
			! peaceBefore ) {
			// new peace!
			sendPeaceWarMessage( "PEACE",
								 false,
								 p->lineageAEveID, p->lineageBEveID );
		}
	}
	else {
		// else doesn't exist, create new unidirectional
		PeaceTreaty p = { inLineageAEveID, inLineageBEveID,
						  true, false,
						  false, false };

		peaceTreaties.push_back( p );
	}
}

/**
 *
 * @param inLineageAEveID
 * @param inLineageBEveID
 * @param outPartialTreaty
 * @return
 * @note from server/server.cpp => server/main.cpp
 * // parial treaty returned if it's requested
 */
char isPeaceTreaty( int inLineageAEveID, int inLineageBEveID, PeaceTreaty **outPartialTreaty)
{

	PeaceTreaty *p = getMatchingTreaty( inLineageAEveID, inLineageBEveID );

	if( p != NULL ) {

		if( !( p->dirAToB && p->dirBToA ) ) {
			// partial treaty
			if( outPartialTreaty != NULL ) {
				*outPartialTreaty = p;
			}
			return false;
		}
		return true;
	}
	return false;
}

/**
 *
 * @param inLineageAEveID
 * @param inLineageBEveID
 * @return
 * @note from server/server.cpp => server/main.cpp
 * // may be partial
 */
PeaceTreaty *getMatchingTreaty( int inLineageAEveID, int inLineageBEveID ) {

	for( int i=0; i<peaceTreaties.size(); i++ ) {
		PeaceTreaty *p = peaceTreaties.getElement( i );


		if( ( p->lineageAEveID == inLineageAEveID &&
			  p->lineageBEveID == inLineageBEveID )
			||
			( p->lineageAEveID == inLineageBEveID &&
			  p->lineageBEveID == inLineageAEveID ) ) {
			// they match a treaty.
			return p;
		}
	}
	return NULL;
}

/**
 *
 * @param inPeaceOrWar
 * @param inWar
 * @param inLineageAEveID
 * @param inLineageBEveID
 * @note from server/server.cpp => server/main.cpp
 */
void sendPeaceWarMessage( const char *inPeaceOrWar, char inWar, int inLineageAEveID, int inLineageBEveID )
{
	double curTime = Time::getCurrentTime();

	for( int i=0; i<warPeaceRecords.size(); i++ ) {
		WarPeaceMessageRecord *r = warPeaceRecords.getElement( i );

		if( inWar != r->war ) {
			continue;
		}

		if( ( r->lineageAEveID == inLineageAEveID &&
			  r->lineageBEveID == inLineageBEveID )
			||
			( r->lineageAEveID == inLineageBEveID &&
			  r->lineageBEveID == inLineageAEveID ) ) {

			if( r->t > curTime - 3 * 60 ) {
				// stil fresh, last similar message happened
				// less than three minutes ago
				return;
			}
			else {
				// stale
				// remove it
				warPeaceRecords.deleteElement( i );
				break;
			}
		}
	}
	WarPeaceMessageRecord r = { inWar, inLineageAEveID, inLineageBEveID,
								curTime };
	warPeaceRecords.push_back( r );


	const char *nameA = "NAMELESS";
	const char *nameB = "NAMELESS";

	for( int j=0; j<players.size(); j++ ) {
		LiveObject *o = players.getElement( j );

		if( ! o->error &&
			o->lineageEveID == inLineageAEveID &&
			o->familyName != NULL ) {
			nameA = o->familyName;
			break;
		}
	}
	for( int j=0; j<players.size(); j++ ) {
		LiveObject *o = players.getElement( j );

		if( ! o->error &&
			o->lineageEveID == inLineageBEveID &&
			o->familyName != NULL ) {
			nameB = o->familyName;
			break;
		}
	}

	char *message = autoSprintf( "%s BETWEEN %s**AND %s FAMILIES",
								 inPeaceOrWar,
								 nameA, nameB );

	sendGlobalMessage( message );

	delete [] message;
}