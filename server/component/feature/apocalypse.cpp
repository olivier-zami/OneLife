//
// Created by olivier on 18/04/2022.
//

#include "apocalypse.h"

#include "../../../third_party/minorGems/crypto/hashes/sha1.h"
#include "../../../third_party/minorGems/network/web/WebRequest.h"
#include "../../../third_party/minorGems/util/log/AppLog.h"
#include "../../../third_party/minorGems/util/SettingsManager.h"
#include "../../../gameSource/GridPos.h"
#include "../../../gameSource/objectBank.h"
#include "../../arcReport.h"
#include "../../dataType/info.h"
#include "../../component/handler/Player.h"
#include "../../component/Map.h"
#include "../../Server.h"
#include "../Socket.h"


extern OneLife::Server* oneLifeServer;
extern char *reflectorURL;
extern SimpleVector<LiveObject> players;
extern SimpleVector<PeaceTreaty> peaceTreaties;
extern SimpleVector<KillState> activeKillStates;
extern SimpleVector<WarPeaceMessageRecord> warPeaceRecords;
extern FILE *familyDataLogFile;
extern double eveWindowStart;
extern double lastBabyPassedThresholdTime;

WebRequest *apocalypseRequest = NULL;
int lastApocalypseNumber = 0;
double apocalypseStartTime = 0;
double lastRemoteApocalypseCheckTime = 0;
double remoteApocalypseCheckInterval = 30;
int apocalypsePossible = 0;
char apocalypseTriggered = false;
char apocalypseRemote = false;
char apocalypseStarted = false;
char postApocalypseStarted = false;
GridPos apocalypseLocation = { 0, 0 };

/**
 *
 * @param inID
 * @return
 * @note from gameSource/objectBank.cpp
 */
char isApocalypseTrigger( int inID )
{
	ObjectRecord *r = getObject( inID );

	if( r == NULL ) {
		return false;
	}
	else {
		return r->apocalypseTrigger;
	}
}

/**
 *
 * @param inID
 * @return
 * @note from gameSource/objectBank.cpp
 * // 0 for nothing
// 1 for monumentStep
// 2 for monumentDone
// 3 for monumentCall
 */
int getMonumentStatus( int inID )
{
	ObjectRecord *r = getObject( inID );

	if( r == NULL ) {
		return 0;
	}
	else {
		if( r->monumentStep ) {
			return 1;
		}
		if( r->monumentDone ) {
			return 2;
		}
		if( r->monumentCall ) {
			return 3;
		}
		return 0;
	}
}

/**
 * @note from server/server.cpp => server/server/main.cpp
 */
void apocalypseStep() {

	double curTime = Time::getCurrentTime();

	if( !apocalypseTriggered ) {

		if( apocalypseRequest == NULL &&
			curTime - lastRemoteApocalypseCheckTime >
			remoteApocalypseCheckInterval ) {

			lastRemoteApocalypseCheckTime = curTime;

			// don't actually send request to reflector if apocalypse
			// not possible locally
			// or if broadcast mode disabled
			if( SettingsManager::getIntSetting( "remoteReport", 0 ) &&
				SettingsManager::getIntSetting( "apocalypsePossible", 0 ) &&
				SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) ) {

				printf( "Checking for remote apocalypse\n" );

				char *url = autoSprintf( "%s?action=check_apocalypse",
										 reflectorURL );

				apocalypseRequest =
						new WebRequest( "GET", url, NULL );

				delete [] url;
			}
		}
		else if( apocalypseRequest != NULL ) {
			int result = apocalypseRequest->step();

			if( result == -1 ) {
				AppLog::info(
						"Apocalypse check:  Request to reflector failed." );
			}
			else if( result == 1 ) {
				// done, have result

				char *webResult =
						apocalypseRequest->getResult();

				if( strstr( webResult, "OK" ) == NULL ) {
					AppLog::infoF(
							"Apocalypse check:  Bad response from reflector:  %s.",
							webResult );
				}
				else {
					int newApocalypseNumber = lastApocalypseNumber;

					sscanf( webResult, "%d\n", &newApocalypseNumber );

					if( newApocalypseNumber > lastApocalypseNumber ) {
						lastApocalypseNumber = newApocalypseNumber;
						apocalypseTriggered = true;
						apocalypseRemote = true;
						AppLog::infoF(
								"Apocalypse check:  New remote apocalypse:  %d.",
								lastApocalypseNumber );
						SettingsManager::setSetting( "lastApocalypseNumber",
													 lastApocalypseNumber );
					}
				}

				delete [] webResult;
			}

			if( result != 0 ) {
				delete apocalypseRequest;
				apocalypseRequest = NULL;
			}
		}
	}



	if( apocalypseTriggered ) {

		if( !apocalypseStarted ) {
			apocalypsePossible =
					SettingsManager::getIntSetting( "apocalypsePossible", 0 );

			if( !apocalypsePossible ) {
				// settings change since we last looked at it
				apocalypseTriggered = false;
				return;
			}

			AppLog::info( "Apocalypse triggerered, starting it" );


			reportArcEnd();


			// only broadcast to reflector if apocalypseBroadcast set
			if( !apocalypseRemote &&
				SettingsManager::getIntSetting( "remoteReport", 0 ) &&
				SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) &&
				apocalypseRequest == NULL && reflectorURL != NULL ) {

				AppLog::info( "Apocalypse broadcast set, telling reflector" );


				char *reflectorSharedSecret =
						SettingsManager::
						getStringSetting( "reflectorSharedSecret" );

				if( reflectorSharedSecret != NULL ) {
					lastApocalypseNumber++;

					AppLog::infoF(
							"Apocalypse trigger:  New local apocalypse:  %d.",
							lastApocalypseNumber );

					SettingsManager::setSetting( "lastApocalypseNumber",
												 lastApocalypseNumber );

					int closestPlayerIndex = -1;
					double closestDist = 999999999;

					for( int i=0; i<players.size(); i++ ) {
						LiveObject *nextPlayer = players.getElement( i );
						if( !nextPlayer->error ) {

							double dist =
									abs( nextPlayer->xd - apocalypseLocation.x ) +
									abs( nextPlayer->yd - apocalypseLocation.y );
							if( dist < closestDist ) {
								closestPlayerIndex = i;
								closestDist = dist;
							}
						}

					}
					char *name = NULL;
					if( closestPlayerIndex != -1 ) {
						name =
								players.getElement( closestPlayerIndex )->
										name;
					}

					if( name == NULL ) {
						name = stringDuplicate( "UNKNOWN" );
					}

					char *idString = autoSprintf( "%d", lastApocalypseNumber );

					char *hash = hmac_sha1( reflectorSharedSecret, idString );

					delete [] idString;

					char *url = autoSprintf(
							"%s?action=trigger_apocalypse"
							"&id=%d&id_hash=%s&name=%s",
							reflectorURL, lastApocalypseNumber, hash, name );

					delete [] hash;
					delete [] name;

					printf( "Starting new web request for %s\n", url );

					apocalypseRequest =
							new WebRequest( "GET", url, NULL );

					delete [] url;
					delete [] reflectorSharedSecret;
				}
			}


			// send all players the AP message
			const char *message = "AP\n#";
			int messageLength = strlen( message );

			for( int i=0; i<players.size(); i++ ) {
				LiveObject *nextPlayer = players.getElement( i );
				if( !nextPlayer->error && nextPlayer->connected ) {

					int numSent =
							nextPlayer->sock->send(
									(unsigned char*)message,
									messageLength,
									false, false );

					nextPlayer->gotPartOfThisFrame = true;

					if( numSent != messageLength ) {
						setPlayerDisconnected( nextPlayer,
											   "Socket write failed" );
					}
				}
			}

			apocalypseStartTime = Time::getCurrentTime();
			apocalypseStarted = true;
			postApocalypseStarted = false;
		}

		if( apocalypseRequest != NULL ) {

			int result = apocalypseRequest->step();


			if( result == -1 ) {
				AppLog::info(
						"Apocalypse trigger:  Request to reflector failed." );
			}
			else if( result == 1 ) {
				// done, have result

				char *webResult =
						apocalypseRequest->getResult();
				printf( "Apocalypse trigger:  "
						"Got web result:  '%s'\n", webResult );

				if( strstr( webResult, "OK" ) == NULL ) {
					AppLog::infoF(
							"Apocalypse trigger:  "
							"Bad response from reflector:  %s.",
							webResult );
				}
				delete [] webResult;
			}

			if( result != 0 ) {
				delete apocalypseRequest;
				apocalypseRequest = NULL;
			}
		}

		if( apocalypseRequest == NULL &&
			Time::getCurrentTime() - apocalypseStartTime >= 8 ) {

			if( ! postApocalypseStarted  ) {
				AppLog::infoF( "Enough warning time, %d players still alive",
							   players.size() );


				double startTime = Time::getCurrentTime();

				if( familyDataLogFile != NULL ) {
					fprintf( familyDataLogFile, "%.2f apocalypse triggered\n",
							 startTime );
				}


				// clear map
				freeMap( true );

				AppLog::infoF( "Apocalypse freeMap took %f sec",
							   Time::getCurrentTime() - startTime );
				wipeMapFiles();

				AppLog::infoF( "Apocalypse wipeMapFiles took %f sec",
							   Time::getCurrentTime() - startTime );

				reseedMap( true );

				//!
				oneLifeServer->initMap();

				AppLog::infoF( "Apocalypse initMap took %f sec", Time::getCurrentTime() - startTime );

				peaceTreaties.deleteAll();
				warPeaceRecords.deleteAll();
				activeKillStates.deleteAll();

				lastRemoteApocalypseCheckTime = curTime;

				for( int i=0; i<players.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement( i );
					backToBasics( nextPlayer );
				}

				// send everyone update about everyone
				for( int i=0; i<players.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement( i );
					nextPlayer->firstMessageSent = false;
					nextPlayer->firstMapSent = false;
					nextPlayer->inFlight = false;
				}

				postApocalypseStarted = true;
			}
			else {
				// make sure all players have gotten map and update
				char allMapAndUpdate = true;

				for( int i=0; i<players.size(); i++ ) {
					LiveObject *nextPlayer = players.getElement( i );
					if( ! nextPlayer->firstMapSent ) {
						allMapAndUpdate = false;
						break;
					}
				}

				if( allMapAndUpdate ) {

					// send all players the AD message
					const char *message = "AD\n#";
					int messageLength = strlen( message );

					for( int i=0; i<players.size(); i++ ) {
						LiveObject *nextPlayer = players.getElement( i );
						if( !nextPlayer->error && nextPlayer->connected ) {

							int numSent =
									nextPlayer->sock->send(
											(unsigned char*)message,
											messageLength,
											false, false );

							nextPlayer->gotPartOfThisFrame = true;

							if( numSent != messageLength ) {
								setPlayerDisconnected( nextPlayer,
													   "Socket write failed" );
							}
						}
					}

					// totally done
					apocalypseStarted = false;
					apocalypseTriggered = false;
					apocalypseRemote = false;
					postApocalypseStarted = false;
				}
			}
		}
	}
}

/**
 *
 * @param inPlayer
 * @note from server/server.cpp => server/main.cpp
 * // returns a person to their natural state
 */
void backToBasics( LiveObject *inPlayer )
{
	LiveObject *p = inPlayer;

	// do not heal dying people
	if( ! p->holdingWound && p->holdingID > 0 ) {

		p->holdingID = 0;

		p->holdingEtaDecay = 0;

		p->heldOriginValid = false;
		p->heldTransitionSourceID = -1;


		p->numContained = 0;
		if( p->containedIDs != NULL ) {
			delete [] p->containedIDs;
			delete [] p->containedEtaDecays;
			p->containedIDs = NULL;
			p->containedEtaDecays = NULL;
		}

		if( p->subContainedIDs != NULL ) {
			delete [] p->subContainedIDs;
			delete [] p->subContainedEtaDecays;
			p->subContainedIDs = NULL;
			p->subContainedEtaDecays = NULL;
		}
	}


	p->clothing = getEmptyClothingSet();

	for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
		p->clothingEtaDecay[c] = 0;
		p->clothingContained[c].deleteAll();
		p->clothingContainedEtaDecays[c].deleteAll();
	}

	p->emotFrozen = false;
	p->emotUnfreezeETA = 0;
}

/**
 * @note from server/server.cpp => server/main.cpp
 */
void triggerApocalypseNow()
{
	apocalypseTriggered = true;

	// restart Eve window, and let this player be the
	// first new Eve
	eveWindowStart = 0;

	// reset other apocalypse trigger
	lastBabyPassedThresholdTime = 0;
}