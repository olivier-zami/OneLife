//
// Created by olivier on 21/08/2022.
//

#include "Player.h"

#include "../../../gameSource/objectBank.h" //TODO: getRandomPersonObject(...) getRandomFemalePersonObject(...); ClothingSet getEmptyClothingSet(...)
//TODO: getRaces(...) getRaceSize(...) getRandomFamilyMember(...) getRandomPersonObjectOfRace(...)
#include "../../component/connection/Tcp.h"//TODO: setPlayerDisconnected
#include "../../component/container/Player.h" //TODO: getLiveObject( inForceParentID ) pickBirthCooldownSeconds()
#include "../../component/feature/apocalypse.h"
#include "../../component/Map.h"//TODO remove getPlayerPos() getEvePosition()
#include "../../cravings.h" //TODO: getCravedFood()
#include "../../curseDB.h"//TODO: (put in feature?) initPersonalCurseTest() addPersonToPersonalCurseTest() isBirthLocationCurseBlocked(...)
#include "../../dataType/LiveObject.h" //TODO: computeAge( LiveObject *inPlayer ) computeFoodCapacity( LiveObject *inPlayer )
#include "../../familySkipList.h" //TODO: isSkipped(...) clearSkipList(...)
#include "../../language.h" //TODO: addEveLanguage() incrementLanguageCount()
#include "../../lifeLog.h" // TODO: logBirth()
#include "../../map.h" //TODO: loadTutorialStart()
#include "../../triggers.h" //TODO: areTriggersEnabled() getTriggerPlayerDisplayID() getTriggerPlayerAge() getTriggerPlayerPos() getTriggerPlayerHolding()
#include "../Server.h" //TODO: remove countFertileMothers(...) countHelplessBabies(...) countFamilies(...) isFertileAge(...)
//TODO: getAgeRate(...); computeFoodDecrementTimeSeconds(...) countYoungFemalesInLineage(...) computePartialMoveSpot(...) getFemale(...)
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/random/CustomRandomSource.h"
#include "minorGems/util/SettingsManager.h"//TODO remove
#include "minorGems/util/stringUtils.h"//TODO: stringDuplicate(...) autoSprintf(...)
//#include "minorGems/network/Socket.h"
#include "minorGems/system/Time.h"

#include <cmath>
//#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

extern char apocalypseTriggered;
extern int babyBirthFoodDecrement;
extern int canYumChainBreak;
extern double childSameRaceLikelihood;
extern double minAgeForCravings;
extern int eatBonus;
extern FILE *familyDataLogFile;
extern int familySpan;
extern double maxFoodDecrementSeconds;
extern int maxLineageTracked;
extern int maxPlacementX;
extern int minActivePlayersForLanguages;
extern double minFoodDecrementSeconds;
extern int nextID;
extern Craving noCraving;
extern int numConnections;
extern SimpleVector<LiveObject> players;
extern CustomRandomSource randSource;
extern float targetHeat;
extern SimpleVector<char*> tempTwinEmails;
extern int tutorialCount;
extern SimpleVector<LiveObject> tutorialLoadingPlayers;
extern int tutorialOffsetX;
extern int useCurseWords;
extern int usePersonalCurses;//TODO : make this a server settings

/**
 *
 * @param inAllowReconnect
 * @param inSock
 * @param inSockBuffer
 * @param inEmail
 * @param hashedSpawnSeed
 * @param inTutorialNumber
 * @param inCurseStatus
 * @param inFitnessScore
 * @param inForceParentID
 * @param inForceDisplayID
 * @param inForcePlayerPos
 * @return from server/server.cpp => server/main.cpp
 * @note from server/server.cpp => server/main.cpp
 * // returns ID of new player,
// or -1 if this player reconnected to an existing ID
 // set to -2 to force Eve int inForceParentID
 */
int processLoggedInPlayer( char inAllowReconnect,
						   Socket *inSock,
						   SimpleVector<char> *inSockBuffer,
						   char *inEmail,
						   uint32_t hashedSpawnSeed,
						   int inTutorialNumber,
						   CurseStatus inCurseStatus,
						   float inFitnessScore,
						   int inForceParentID,
						   int inForceDisplayID,
						   GridPos *inForcePlayerPos)
{
	usePersonalCurses = SettingsManager::getIntSetting( "usePersonalCurses",0 );
	if( usePersonalCurses )
	{
		// ignore what old curse system said
		inCurseStatus.curseLevel = 0;
		inCurseStatus.excessPoints = 0;
		initPersonalCurseTest( inEmail );

		for( int p=0; p<players.size(); p++ )
		{
			LiveObject *o = players.getElement( p );
			if( ! o->error &&
				! o->isTutorial &&
				o->curseStatus.curseLevel == 0 &&
				strcmp( o->email, inEmail ) != 0 )
			{

				// non-tutorial, non-cursed, non-us player
				addPersonToPersonalCurseTest( o->email, inEmail,
											  getPlayerPos( o ) );
			}
		}
	}

	// new behavior:
	// allow this new connection from same
	// email (most likely a re-connect
	// by same person, when the old connection
	// hasn't broken on our end yet)

	// to make it work, force-mark
	// the old connection as broken
	for( int p=0; p<players.size(); p++ )
	{
		LiveObject *o = players.getElement( p );

		if( ! o->error &&
			o->connected &&
			strcmp( o->email, inEmail ) == 0 ) {

			setPlayerDisconnected( o, "Authentic reconnect received" );

			break;
		}
	}



	// see if player was previously disconnected
	for( int i=0; i<players.size(); i++ )
	{
		LiveObject *o = players.getElement( i );

		if( ! o->error && ! o->connected &&
			strcmp( o->email, inEmail ) == 0 ) {

			if( ! inAllowReconnect ) {
				// trigger an error for them, so they die and are removed
				o->error = true;
				o->errorCauseString = "Reconnected as twin";
				break;
			}

			// else allow them to reconnect to existing life

			// give them this new socket and buffer
			if( o->sock != NULL ) {
				delete o->sock;
				o->sock = NULL;
			}
			if( o->sockBuffer != NULL ) {
				delete o->sockBuffer;
				o->sockBuffer = NULL;
			}

			o->sock = inSock;
			o->sockBuffer = inSockBuffer;

			// they are connecting again, need to send them everything again
			o->firstMapSent = false;
			o->firstMessageSent = false;
			o->inFlight = false;

			o->connected = true;
			o->cravingKnown = false;

			if( o->heldByOther ) {
				// they're held, so they may have moved far away from their
				// original location

				// their first PU on reconnect should give an estimate of this
				// new location

				LiveObject *holdingPlayer =
						getLiveObject( o->heldByOtherID );

				if( holdingPlayer != NULL ) {
					o->xd = holdingPlayer->xd;
					o->yd = holdingPlayer->yd;

					o->xs = holdingPlayer->xs;
					o->ys = holdingPlayer->ys;
				}
			}

			AppLog::infoF( "Player %d (%s) has reconnected.",
						   o->id, o->email );

			delete [] inEmail;

			return -1;
		}
	}


	// a baby needs to be born
	char eveWindow = isEveWindow();
	char forceGirl = false;
	int familyLimitAfterEveWindow = SettingsManager::getIntSetting("familyLimitAfterEveWindow", 15 );
	int cM = countFertileMothers();
	int cB = countHelplessBabies();
	int cFam = countFamilies();

	if( ! eveWindow )
	{

		float ratio = SettingsManager::getFloatSetting("babyMotherApocalypseRatio", 6.0 );
		if( cM == 0 || (float)cB / (float)cM >= ratio )
		{
			// too many babies per mother inside barrier
			triggerApocalypseNow();
		}
		else
		{
			int minFertile = players.size() / 15;
			if( minFertile < 2 )
			{
				minFertile = 2;
			}
			if( cM < minFertile )
			{
				// less than 1/15 of the players are fertile mothers
				forceGirl = true;
			}
		}

		if( !apocalypseTriggered && familyLimitAfterEveWindow > 0 ) {

			// there's a family limit
			// see if we passed it

			if( cFam > familyLimitAfterEveWindow ) {
				// too many families

				// that means we've reach a state where no one is surviving
				// and there are lots of eves scrounging around
				triggerApocalypseNow();
			}
		}

	}

	int barrierRadius = SettingsManager::getIntSetting( "barrierRadius", 250 );
	int barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );

	// reload these settings every time someone new connects
	// thus, they can be changed without restarting the server
	minFoodDecrementSeconds = SettingsManager::getFloatSetting( "minFoodDecrementSeconds", 5.0f );
	maxFoodDecrementSeconds = SettingsManager::getFloatSetting( "maxFoodDecrementSeconds", 20 );
	babyBirthFoodDecrement = SettingsManager::getIntSetting( "babyBirthFoodDecrement", 10 );
	eatBonus = SettingsManager::getIntSetting( "eatBonus", 0 );
	useCurseWords = SettingsManager::getIntSetting( "useCurseWords", 1 );
	minActivePlayersForLanguages = SettingsManager::getIntSetting( "minActivePlayersForLanguages", 15 );
	canYumChainBreak = SettingsManager::getIntSetting( "canYumChainBreak", 0 );
	minAgeForCravings = SettingsManager::getDoubleSetting( "minAgeForCravings", 10 );

	numConnections ++;

	LiveObject newObject;

	newObject.email = inEmail;
	newObject.origEmail = NULL;

	newObject.lastSidsBabyEmail = NULL;

	newObject.lastBabyEmail = NULL;

	newObject.cravingFood = noCraving;
	newObject.cravingFoodYumIncrement = 0;
	newObject.cravingKnown = false;

	newObject.id = nextID;
	nextID++;

	if( familyDataLogFile != NULL )
	{
		int eveCount = 0;
		int inCount = 0;

		double ageSum = 0;
		int ageSumCount = 0;

		for( int i=0; i<players.size(); i++ ) {
			LiveObject *o = players.getElement( i );

			if( ! o->error && o->connected ) {
				if( o->parentID == -1 ) {
					eveCount++;
				}
				if( barrierOn ) {
					// only those inside the barrier
					GridPos pos = getPlayerPos( o );

					if( abs( pos.x ) < barrierRadius &&
						abs( pos.y ) < barrierRadius ) {
						inCount++;

						ageSum += computeAge( o );
						ageSumCount++;
					}
				}
				else {
					ageSum += computeAge( o );
					ageSumCount++;
				}
			}
		}

		double averageAge = 0;
		if( ageSumCount > 0 ) {
			averageAge = ageSum / ageSumCount;
		}

		fprintf( familyDataLogFile,
				 "%.2f nid:%d fam:%d mom:%d bb:%d plr:%d eve:%d rft:%d "
				 "avAge:%.2f\n",
				 Time::getCurrentTime(), newObject.id,
				 cFam, cM, cB,
				 players.size(),
				 eveCount,
				 inCount,
				 averageAge );
	}

	newObject.fitnessScore = inFitnessScore;
	SettingsManager::setSetting( "nextPlayerID", (int)nextID );

	newObject.responsiblePlayerID = -1;
	newObject.displayID = getRandomPersonObject();
	newObject.isEve = false;
	newObject.isTutorial = false;

	if( inTutorialNumber > 0 )
	{
		newObject.isTutorial = true;
	}

	newObject.trueStartTimeSeconds = Time::getCurrentTime();
	newObject.lifeStartTimeSeconds = newObject.trueStartTimeSeconds;
	newObject.lastSayTimeSeconds = Time::getCurrentTime();
	newObject.heldByOther = false;
	newObject.everHeldByParent = false;

	int numPlayers = players.size();
	SimpleVector<LiveObject*> parentChoices;
	int numBirthLocationsCurseBlocked = 0;
	int numOfAge = 0;


	// first, find all mothers that could possibly have us
	// three passes, once with birth cooldown limit and lineage limits on,
	// then more passes with them off (if needed)
	char checkCooldown = true;
	for( int p=0; p<2; p++ )
	{

		for( int i=0; i<numPlayers; i++ ) {
			LiveObject *player = players.getElement( i );

			if( player->error ) {
				continue;
			}

			if( player->isTutorial ) {
				continue;
			}

			if( player->vogMode ) {
				continue;
			}

			//skips over solo players who declare themselves infertile
			if( player->declaredInfertile ) {
				continue;
			}

			//GridPos motherPos = getPlayerPos( player );


			if( player->lastSidsBabyEmail != NULL &&
				strcmp( player->lastSidsBabyEmail,
						newObject.email ) == 0 ) {
				// this baby JUST committed SIDS for this mother
				// skip her
				// (don't ever send SIDS baby to same mother twice in a row)
				continue;
			}

			if( isFertileAge( player ) ) {
				numOfAge ++;

				if( checkCooldown &&
					Time::timeSec() < player->birthCoolDown ) {
					continue;
				}

				GridPos motherPos = getPlayerPos( player );

				if( usePersonalCurses &&
					isBirthLocationCurseBlocked( newObject.email,
												 motherPos ) ) {
					// this spot forbidden
					// because someone nearby cursed new player
					numBirthLocationsCurseBlocked++;
					continue;
				}

				// test any twins also
				char twinBanned = false;
				for( int s=0; s<tempTwinEmails.size(); s++ ) {
					if( usePersonalCurses &&
						// non-cached version for twin emails
						// (otherwise, we interfere with caching done
						//  for our email)
						isBirthLocationCurseBlockedNoCache(
								tempTwinEmails.getElementDirect( s ),
								motherPos ) ) {
						twinBanned = true;

						numBirthLocationsCurseBlocked++;

						break;
					}
				}

				if( twinBanned ) {
					continue;
				}


				if( ( inCurseStatus.curseLevel <= 0 &&
					  player->curseStatus.curseLevel <= 0 )
					||
					( inCurseStatus.curseLevel > 0 &&
					  player->curseStatus.curseLevel > 0 ) ) {
					// cursed babies only born to cursed mothers
					// non-cursed babies never born to cursed mothers
					parentChoices.push_back( player );
				}
			}
		}



		if( p == 0 ) {
			if( parentChoices.size() > 0 || numOfAge == 0 ) {
				// found some mothers off-cool-down,
				// or there are none at all
				// skip second pass
				break;
			}

			// else found no mothers (but some on cool-down?)
			// start over with cooldowns off

			AppLog::infoF(
					"Trying to place new baby %s, out of %d fertile moms, "
					"all are on cooldown, lineage banned, or curse blocked.  "
					"Trying again ignoring cooldowns.", newObject.email, numOfAge );

			checkCooldown = false;
			numBirthLocationsCurseBlocked = 0;
			numOfAge = 0;
		}

	}

	if( parentChoices.size() == 0 && numBirthLocationsCurseBlocked > 0 )
	{
		// they are blocked from being born EVERYWHERE by curses

		AppLog::infoF( "No available mothers, and %d are curse blocked, "
					   "sending a new Eve to donkeytown",
					   numBirthLocationsCurseBlocked );

		// d-town
		inCurseStatus.curseLevel = 1;
		inCurseStatus.excessPoints = 1;
	}

	if( inTutorialNumber > 0 )
	{
		// Tutorial always played full-grown
		parentChoices.deleteAll();
	}

	if( inForceParentID == -2 )
	{
		// force eve
		parentChoices.deleteAll();
	}
	else if( inForceParentID > -1 )
	{
		// force parent choice
		parentChoices.deleteAll();

		LiveObject *forcedParent = getLiveObject( inForceParentID );

		if( forcedParent != NULL ) {
			parentChoices.push_back( forcedParent );
		}
	}

	if( SettingsManager::getIntSetting( "forceAllPlayersEve", 0 ) )
	{
		parentChoices.deleteAll();
	}
	if( hashedSpawnSeed != 0 && SettingsManager::getIntSetting( "forceEveOnSeededSpawn", 0 ) )
	{
		parentChoices.deleteAll();
	}

	newObject.parentChainLength = 1;

	if( parentChoices.size() == 0 )
	{
		// new Eve
		// she starts almost full grown

		newObject.isEve = true;
		newObject.lineageEveID = newObject.id;
		newObject.lifeStartTimeSeconds -= 14 * ( 1.0 / getAgeRate() );

		// she starts off craving a food right away
		newObject.cravingFood = getCravedFood( newObject.lineageEveID, newObject.parentChainLength );
		// initilize increment
		newObject.cravingFoodYumIncrement = 1;

		int femaleID = getRandomFemalePersonObject();

		if( femaleID != -1 )
		{
			newObject.displayID = femaleID;
		}
	}

	// else player starts as newborn
	newObject.foodCapModifier = 1.0;
	newObject.fever = 0;

	// start full up to capacity with food
	newObject.foodStore = computeFoodCapacity( &newObject );

	newObject.drunkenness = 0;
	newObject.drunkennessEffectETA = 0;
	newObject.drunkennessEffect = false;

	newObject.tripping = false;
	newObject.gonnaBeTripping = false;
	newObject.trippingEffectStartTime = 0;
	newObject.trippingEffectETA = 0;


	if( ! newObject.isEve ) {
		// babies start out almost starving
		newObject.foodStore = 2;
	}

	if( newObject.isTutorial && newObject.foodStore > 10 ) {
		// so they can practice eating at the beginning of the tutorial
		newObject.foodStore -= 6;
	}

	double currentTime = Time::getCurrentTime();


	newObject.envHeat = targetHeat;
	newObject.bodyHeat = targetHeat;
	newObject.biomeHeat = targetHeat;
	newObject.lastBiomeHeat = targetHeat;
	newObject.heat = 0.5;
	newObject.heatUpdate = false;
	newObject.lastHeatUpdate = currentTime;
	newObject.isIndoors = false;
	newObject.foodDecrementETASeconds = currentTime + computeFoodDecrementTimeSeconds( &newObject );
	newObject.foodUpdate = true;
	newObject.lastAteID = 0;
	newObject.lastAteFillMax = 0;
	newObject.justAte = false;
	newObject.justAteID = 0;
	newObject.yummyBonusStore = 0;
	newObject.clothing = getEmptyClothingSet();

	for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
		newObject.clothingEtaDecay[c] = 0;
	}

	newObject.xs = 0;
	newObject.ys = 0;
	newObject.xd = 0;
	newObject.yd = 0;

	newObject.facingLeft = 0;
	newObject.lastFlipTime = currentTime;

	newObject.lastRegionLookTime = 0;
	newObject.playerCrossingCheckTime = 0;


	LiveObject *parent = NULL;

	char placed = false;

	if( parentChoices.size() > 0 ) {
		placed = true;

		if( newObject.isEve ) {
			// spawned next to random existing player
			int parentIndex =
					randSource.getRandomBoundedInt( 0,
													parentChoices.size() - 1 );

			parent = parentChoices.getElementDirect( parentIndex );
		}
		else {
			// baby



			// filter parent choices by this baby's skip list
			SimpleVector<LiveObject *>
			filteredParentChoices( parentChoices.size() );

			for( int i=0; i<parentChoices.size(); i++ ) {
				LiveObject *p = parentChoices.getElementDirect( i );

				if( ! isSkipped( inEmail, p->lineageEveID ) ) {
					filteredParentChoices.push_back( p );
				}
			}

			if( filteredParentChoices.size() == 0 ) {
				// baby has skipped everyone

				// clear their list and let them start over again
				clearSkipList( inEmail );

				filteredParentChoices.push_back_other( &parentChoices );
			}



			// pick random mother from a weighted distribution based on
			// each mother's temperature

			// AND each mother's current YUM multiplier

			int maxYumMult = 1;

			for( int i=0; i<filteredParentChoices.size(); i++ ) {
				LiveObject *p = filteredParentChoices.getElementDirect( i );

				int yumMult = p->yummyFoodChain.size() - 1;

				if( yumMult < 0 ) {
					yumMult = 0;
				}

				if( yumMult > maxYumMult ) {
					maxYumMult = yumMult;
				}
			}

			// 0.5 temp is worth .5 weight
			// 1.0 temp and 0 are worth 0 weight

			// max YumMult worth same that perfect temp is worth (0.5 weight)

			double totalWeight = 0;

			for( int i=0; i<filteredParentChoices.size(); i++ ) {
				LiveObject *p = filteredParentChoices.getElementDirect( i );

				// temp part of weight
				totalWeight += 0.5 - fabs( p->heat - 0.5 );


				int yumMult = p->yummyFoodChain.size() - 1;

				if( yumMult < 0 ) {
					yumMult = 0;
				}

				// yum mult part of weight
				totalWeight += 0.5 * yumMult / (double) maxYumMult;
			}

			double choice =
					randSource.getRandomBoundedDouble( 0, totalWeight );


			totalWeight = 0;

			for( int i=0; i<filteredParentChoices.size(); i++ ) {
				LiveObject *p = filteredParentChoices.getElementDirect( i );

				totalWeight += 0.5 - fabs( p->heat - 0.5 );


				int yumMult = p->yummyFoodChain.size() - 1;

				if( yumMult < 0 ) {
					yumMult = 0;
				}

				// yum mult part of weight
				totalWeight += 0.5 * yumMult / (double) maxYumMult;

				if( totalWeight >= choice ) {
					parent = p;
					break;
				}
			}
		}



		if( ! newObject.isEve )
		{
			// mother giving birth to baby
			// take a ton out of her food store

			int min = 4;
			if( parent->foodStore < min ) {
				min = parent->foodStore;
			}
			parent->foodStore -= babyBirthFoodDecrement;
			if( parent->foodStore < min ) {
				parent->foodStore = min;
			}

			parent->foodDecrementETASeconds +=
					computeFoodDecrementTimeSeconds( parent );

			parent->foodUpdate = true;


			// only set race if the spawn-near player is our mother
			// otherwise, we are a new Eve spawning next to a baby

			timeSec_t curTime = Time::timeSec();

			parent->babyBirthTimes->push_back( curTime );
			parent->babyIDs->push_back( newObject.id );

			if( parent->lastBabyEmail != NULL ) {
				delete [] parent->lastBabyEmail;
			}
			parent->lastBabyEmail = stringDuplicate( newObject.email );


			// set cool-down time before this worman can have another baby
			parent->birthCoolDown = pickBirthCooldownSeconds() + curTime;

			ObjectRecord *parentObject = getObject( parent->displayID );

			// pick race of child
			int numRaces;
			int *races = getRaces( &numRaces );

			int parentRaceIndex = -1;

			for( int i=0; i<numRaces; i++ ) {
				if( parentObject->race == races[i] ) {
					parentRaceIndex = i;
					break;
				}
			}


			if( parentRaceIndex != -1 ) {

				int childRace = parentObject->race;

				char forceDifferentRace = false;

				if( getRaceSize( parentObject->race ) < 3 ) {
					// no room in race for diverse family members

					// pick a different race for child to ensure village
					// diversity
					// (otherwise, almost everyone is going to look the same)
					forceDifferentRace = true;
				}

				// everyone has a small chance of having a neighboring-race
				// baby, even if not forced by parent's small race size
				if( forceDifferentRace ||
					randSource.getRandomDouble() >
					childSameRaceLikelihood ) {

					// different race than parent

					int offset = 1;

					if( randSource.getRandomBoolean() ) {
						offset = -1;
					}
					int childRaceIndex = parentRaceIndex + offset;

					// don't wrap around
					// but push in other direction instead
					if( childRaceIndex >= numRaces ) {
						childRaceIndex = numRaces - 2;
					}
					if( childRaceIndex < 0 ) {
						childRaceIndex = 1;
					}

					// stay in bounds
					if( childRaceIndex >= numRaces ) {
						childRaceIndex = numRaces - 1;
					}


					childRace = races[ childRaceIndex ];
				}

				if( childRace == parentObject->race ) {

					if( countYoungFemalesInLineage( parent->lineageEveID ) <
						SettingsManager::getIntSetting( "minYoungFemalesToForceGirl", 2 ) ) {
						forceGirl = true;
					}

					newObject.displayID = getRandomFamilyMember(
							parentObject->race, parent->displayID, familySpan,
							forceGirl );
				}
				else {
					newObject.displayID =
							getRandomPersonObjectOfRace( childRace );
				}

			}

			delete [] races;
		}

		if( parent->xs == parent->xd &&
			parent->ys == parent->yd )
		{

			// stationary parent
			newObject.xs = parent->xs;
			newObject.ys = parent->ys;

			newObject.xd = parent->xs;
			newObject.yd = parent->ys;
		}
		else {
			// find where parent is along path
			GridPos cPos = computePartialMoveSpot( parent );

			newObject.xs = cPos.x;
			newObject.ys = cPos.y;

			newObject.xd = cPos.x;
			newObject.yd = cPos.y;
		}

		if( newObject.xs > maxPlacementX ) {
			maxPlacementX = newObject.xs;
		}
	}
	else if( inTutorialNumber > 0 ) {

		int startX = maxPlacementX + tutorialOffsetX;
		int startY = tutorialCount * 25;

		newObject.xs = startX;
		newObject.ys = startY;

		newObject.xd = startX;
		newObject.yd = startY;

		char *mapFileName = autoSprintf( "tutorial%d.txt", inTutorialNumber );

		placed = loadTutorialStart( &( newObject.tutorialLoad ),
									mapFileName, startX, startY );

		delete [] mapFileName;

		tutorialCount ++;

		int maxPlayers =
				SettingsManager::getIntSetting( "maxPlayers", 200 );

		if( tutorialCount > maxPlayers ) {
			// wrap back to 0 so we don't keep getting farther
			// and farther away on map if server runs for a long time.

			// The earlier-placed tutorials are over by now, because
			// we can't have more than maxPlayers tutorials running at once

			tutorialCount = 0;
		}
	}


	if( !placed ) {
		// tutorial didn't happen if not placed
		newObject.isTutorial = false;

		char allowEveRespawn = true;

		if( numOfAge >= 4 ) {
			// there are at least 4 fertile females on the server
			// why is this player spawning as Eve?
			// they must be on lineage ban everywhere
			// (and they are NOT a solo player on an empty server)
			// don't allow them to spawn back at their last old-age Eve death
			// location.
			allowEveRespawn = false;
		}

		// else starts at civ outskirts (lone Eve)

		SimpleVector<GridPos> otherPeoplePos( numPlayers );


		// consider players to be near Eve location that match
		// Eve's curse status
		char seekingCursed = false;

		if( inCurseStatus.curseLevel > 0 ) {
			seekingCursed = true;
		}


		for( int i=0; i<numPlayers; i++ ) {
			LiveObject *player = players.getElement( i );

			if( player->error ||
				! player->connected ||
				player->isTutorial ||
				player->vogMode ) {
				continue;
			}

			if( seekingCursed && player->curseStatus.curseLevel <= 0 ) {
				continue;
			}
			else if( ! seekingCursed &&
					 player->curseStatus.curseLevel > 0 ) {
				continue;
			}

			GridPos p = { player->xs, player->ys };
			otherPeoplePos.push_back( p );
		}


		int startX, startY;
		getEvePosition( newObject.email,
						newObject.id, &startX, &startY,
						&otherPeoplePos, allowEveRespawn );

		if( inCurseStatus.curseLevel > 0 )
		{
			// keep cursed players away

			// 20K away in X and 20K away in Y, pushing out away from 0
			// in both directions

			if( startX > 0 )
				startX += 20000;
			else
				startX -= 20000;

			if( startY > 0 )
				startY += 20000;
			else
				startY -= 20000;
		}

		if( SettingsManager::getIntSetting( "forceEveLocation", 0 ) && inCurseStatus.curseLevel == 0 )
		{
			startX = SettingsManager::getIntSetting( "forceEveLocationX", 0 );
			startY = SettingsManager::getIntSetting( "forceEveLocationY", 0 );
		}

		uint32_t tempHashedSpawnSeed;
		int useSeedList = SettingsManager::getIntSetting( "useSeedList", 0 );
		//pick a random seed from a list to be the default spawn
		if ( useSeedList && hashedSpawnSeed == 0 )
		{

			//parse the seeds
			SimpleVector<char *> *list =
					SettingsManager::getSetting(
							"defaultSeedList" );

			//chose a random seed from the list
			int seedIndex =
					randSource.getRandomBoundedInt( 0, list->size() - 1 );

			char *choseSeed;
			for( int i=0; i<list->size(); i++ ) {
				if( seedIndex == i ) {
					choseSeed = list->getElementDirect( i );
					break;
				}
			}

			std::string seed( choseSeed );

			//convert and apply seed hash (copy pasted code)
			//make this a separate method in the future to prevent redundancy

			// FNV-1a Hashing algorithm
			auto hashStr = [](std::string &s, const uint32_t FNV_init = 2166136261u){
				const size_t FNV_prime = 111337;

				// Hash seed to 4 byte int
				uint32_t hash = FNV_init;
				for( auto c : s ) {
					hash ^= c;
					hash *= FNV_prime;
				}

				return hash;
			};

			// Get the substr from one after the seed delim
			std::string seedSalt { SettingsManager::getStringSetting("seedSalt", "default salt") };
			tempHashedSpawnSeed = hashStr(seed, hashStr(seedSalt));
		}
		else
		{
			//use defalt seed configuration
			tempHashedSpawnSeed = hashedSpawnSeed;
		}

		if( tempHashedSpawnSeed != 0 )
		{
			// Get bounding box from setting, default to 10k
			int seedSpawnBoundingBox = SettingsManager::getIntSetting( "seedSpawnBoundingBox", 10000 );
			std::seed_seq ssq { tempHashedSpawnSeed };
			std::mt19937_64 mt { ssq };
			std::uniform_int_distribution<int> dist( -seedSpawnBoundingBox/2, seedSpawnBoundingBox/2 );
			startX = dist(mt);
			startY = dist(mt);
			startX = 0; startY = 0;
			printf("\n\t===============================>we force start point to (%i, %i)", startX, startY);
			AppLog::infoF( "Player %s seed evaluated to (%d,%d)", newObject.email, startX, startY );
		}


		newObject.xs = startX;
		newObject.ys = startY;

		newObject.xd = startX;
		newObject.yd = startY;

		if( newObject.xs > maxPlacementX )
		{
			maxPlacementX = newObject.xs;
		}
	}


	if( inForceDisplayID != -1 ) {
		newObject.displayID = inForceDisplayID;
	}

	if( inForcePlayerPos != NULL ) {
		int startX = inForcePlayerPos->x;
		int startY = inForcePlayerPos->y;

		newObject.xs = startX;
		newObject.ys = startY;

		newObject.xd = startX;
		newObject.yd = startY;

		if( newObject.xs > maxPlacementX ) {
			maxPlacementX = newObject.xs;
		}
	}

	if( parent == NULL )
	{
		// Eve
		int forceID = SettingsManager::getIntSetting( "forceEveObject", 0 );

		if( forceID > 0 ) {
			newObject.displayID = forceID;
		}


		float forceAge = SettingsManager::getFloatSetting( "forceEveAge", 0.0 );

		if( forceAge > 0 ) {
			newObject.lifeStartTimeSeconds =
					Time::getCurrentTime() - forceAge * ( 1.0 / getAgeRate() );
		}
	}


	newObject.holdingID = 0;


	if( areTriggersEnabled() ) {
		int id = getTriggerPlayerDisplayID( inEmail );

		if( id != -1 ) {
			newObject.displayID = id;

			newObject.lifeStartTimeSeconds =
					Time::getCurrentTime() -
					getTriggerPlayerAge( inEmail ) * ( 1.0 / getAgeRate() );

			GridPos pos = getTriggerPlayerPos( inEmail );

			newObject.xd = pos.x;
			newObject.yd = pos.y;
			newObject.xs = pos.x;
			newObject.ys = pos.y;
			newObject.xd = pos.x;

			newObject.holdingID = getTriggerPlayerHolding( inEmail );
			newObject.clothing = getTriggerPlayerClothing( inEmail );
		}
	}


	newObject.lineage = new SimpleVector<int>();

	newObject.name = NULL;
	newObject.displayedName = NULL;
	newObject.familyName = NULL;

	newObject.nameHasSuffix = false;
	newObject.lastSay = NULL;
	newObject.curseStatus = inCurseStatus;

	//2HOL additions for: password-protected-objects, additional fields initialization
	newObject.saidPassword = NULL;
	newObject.assignedPassword = NULL;


	if( newObject.curseStatus.curseLevel == 0 &&
		hasCurseToken( inEmail ) ) {
		newObject.curseTokenCount = 1;
	}
	else {
		newObject.curseTokenCount = 0;
	}

	newObject.curseTokenUpdate = true;


	newObject.pathLength = 0;
	newObject.pathToDest = NULL;
	newObject.pathTruncated = 0;
	newObject.firstMapSent = false;
	newObject.lastSentMapX = 0;
	newObject.lastSentMapY = 0;
	newObject.moveStartTime = Time::getCurrentTime();
	newObject.moveTotalSeconds = 0;
	newObject.facingOverride = 0;
	newObject.actionAttempt = 0;
	newObject.actionTarget.x = 0;
	newObject.actionTarget.y = 0;
	newObject.holdingEtaDecay = 0;
	newObject.heldOriginValid = 0;
	newObject.heldOriginX = 0;
	newObject.heldOriginY = 0;

	newObject.heldGraveOriginX = 0;
	newObject.heldGraveOriginY = 0;
	newObject.heldGravePlayerID = 0;

	newObject.heldTransitionSourceID = -1;
	newObject.numContained = 0;
	newObject.containedIDs = NULL;
	newObject.containedEtaDecays = NULL;
	newObject.subContainedIDs = NULL;
	newObject.subContainedEtaDecays = NULL;
	newObject.embeddedWeaponID = 0;
	newObject.embeddedWeaponEtaDecay = 0;
	newObject.murderSourceID = 0;
	newObject.holdingWound = false;

	newObject.murderPerpID = 0;
	newObject.murderPerpEmail = NULL;

	newObject.deathSourceID = 0;

	newObject.everKilledAnyone = false;
	newObject.suicide = false;


	newObject.sock = inSock;
	newObject.sockBuffer = inSockBuffer;

	newObject.gotPartOfThisFrame = false;

	newObject.isNew = true;
	newObject.isNewCursed = false;
	newObject.firstMessageSent = false;
	newObject.inFlight = false;

	newObject.dying = false;
	newObject.dyingETA = 0;

	newObject.emotFrozen = false;
	newObject.emotUnfreezeETA = 0;
	newObject.emotFrozenIndex = 0;

	newObject.starving = false;

	newObject.connected = true;
	newObject.error = false;
	newObject.errorCauseString = "";

	newObject.lastActionTime = Time::getCurrentTime();
	newObject.isAFK = false;

	newObject.customGraveID = -1;
	newObject.deathReason = NULL;

	newObject.deleteSent = false;
	newObject.deathLogged = false;
	newObject.newMove = false;

	newObject.posForced = false;
	newObject.waitingForForceResponse = false;

	// first move that player sends will be 2
	newObject.lastMoveSequenceNumber = 1;

	newObject.needsUpdate = false;
	newObject.updateSent = false;
	newObject.updateGlobal = false;

	newObject.babyBirthTimes = new SimpleVector<timeSec_t>();
	newObject.babyIDs = new SimpleVector<int>();

	newObject.birthCoolDown = 0;
	newObject.declaredInfertile = false;

	newObject.monumentPosSet = false;
	newObject.monumentPosSent = true;

	newObject.holdingFlightObject = false;

	newObject.vogMode = false;
	newObject.postVogMode = false;
	newObject.vogJumpIndex = 0;


	for( int i=0; i<HEAT_MAP_D * HEAT_MAP_D; i++ ) {
		newObject.heatMap[i] = 0;
	}


	newObject.parentID = -1;
	char *parentEmail = NULL;

	if( parent != NULL && isFertileAge( parent ) ) {
		// do not log babies that new Eve spawns next to as parents
		newObject.parentID = parent->id;
		parentEmail = parent->email;

		if( parent->familyName != NULL ) {
			newObject.familyName = stringDuplicate( parent->familyName );
		}

		newObject.lineageEveID = parent->lineageEveID;

		newObject.parentChainLength = parent->parentChainLength + 1;

		// mother
		newObject.lineage->push_back( newObject.parentID );


		// inherit mother's craving at time of birth
		newObject.cravingFood = parent->cravingFood;

		// increment for next generation
		newObject.cravingFoodYumIncrement = parent->cravingFoodYumIncrement + 1;


		// inherit last heard monument, if any, from parent
		newObject.monumentPosSet = parent->monumentPosSet;
		newObject.lastMonumentPos = parent->lastMonumentPos;
		newObject.lastMonumentID = parent->lastMonumentID;
		if( newObject.monumentPosSet ) {
			newObject.monumentPosSent = false;
		}


		for( int i=0;
			 i < parent->lineage->size() &&
			 i < maxLineageTracked - 1;
			 i++ ) {

			newObject.lineage->push_back(
					parent->lineage->getElementDirect( i ) );
		}
	}

	newObject.birthPos.x = newObject.xd;
	newObject.birthPos.y = newObject.yd;

	newObject.originalBirthPos = newObject.birthPos;


	newObject.heldOriginX = newObject.xd;
	newObject.heldOriginY = newObject.yd;

	newObject.actionTarget = newObject.birthPos;



	newObject.ancestorIDs = new SimpleVector<int>();
	newObject.ancestorEmails = new SimpleVector<char*>();
	newObject.ancestorRelNames = new SimpleVector<char*>();
	newObject.ancestorLifeStartTimeSeconds = new SimpleVector<double>();
	newObject.ancestorLifeEndTimeSeconds = new SimpleVector<double>();

	for( int j=0; j<players.size(); j++ ) {
		LiveObject *otherPlayer = players.getElement( j );

		if( otherPlayer->error ) {
			continue;
		}

		// a living other player

		// consider all men here
		// and any childless women (they are counted as aunts
		// for any children born before they themselves have children
		// or after all their own children die)
		if( newObject.parentID != otherPlayer->id
			&&
			( ! getFemale( otherPlayer ) ||
			  countLivingChildren( otherPlayer->id ) == 0 ) ) {

			//Only direct mother-son/daughter parenting is counted

		}
		else {
			// females, look for direct ancestry

			for( int i=0; i<newObject.lineage->size(); i++ ) {

				if( newObject.lineage->getElementDirect( i ) ==
					otherPlayer->id ) {

					//Only direct mother-son/daughter parenting is counted
					if( i != 0 ) continue;

					newObject.ancestorIDs->push_back( otherPlayer->id );

					newObject.ancestorEmails->push_back(
							stringDuplicate( otherPlayer->email ) );

					// i tells us how many greats and grands
					SimpleVector<char> workingName;

					for( int g=1; g<=i; g++ ) {
						if( g == i ) {
							workingName.appendElementString( "Grand" );
						}
						else {
							workingName.appendElementString( "Great_" );
						}
					}


					if( i != 0 ) {
						if( ! getFemale( &newObject ) ) {
							workingName.appendElementString( "son" );
						}
						else {
							workingName.appendElementString( "daughter" );
						}
					}
					else {
						// no "Grand"
						if( ! getFemale( &newObject ) ) {
							workingName.appendElementString( "Son" );
						}
						else {
							workingName.appendElementString( "Daughter" );
						}
					}


					newObject.ancestorRelNames->push_back(
							workingName.getElementString() );

					newObject.ancestorLifeStartTimeSeconds->push_back(
							otherPlayer->lifeStartTimeSeconds );
					newObject.ancestorLifeEndTimeSeconds->push_back(
							-1.0 );

					break;
				}
			}
		}


	}





	// parent pointer possibly no longer valid after push_back, which
	// can resize the vector
	parent = NULL;


	if( newObject.isTutorial ) {
		AppLog::infoF( "New player %s pending tutorial load (tutorial=%d)",
					   newObject.email,
					   inTutorialNumber );

		// holding bay for loading tutorial maps incrementally
		tutorialLoadingPlayers.push_back( newObject );
	}
	else {
		players.push_back( newObject );
	}

	if( newObject.isEve ) {
		addEveLanguage( newObject.id );
	}
	else {
		incrementLanguageCount( newObject.lineageEveID );
	}


	// addRecentScore( newObject.email, inFitnessScore );


	if( ! newObject.isTutorial )
		logBirth( newObject.id,
				  newObject.email,
				  newObject.parentID,
				  parentEmail,
				  ! getFemale( &newObject ),
				  newObject.xd,
				  newObject.yd,
				  players.size(),
				  newObject.parentChainLength );

	AppLog::infoF( "New player %s connected as player %d (tutorial=%d) (%d,%d)"
				   " (maxPlacementX=%d)",
				   newObject.email, newObject.id,
				   inTutorialNumber, newObject.xs, newObject.ys,
				   maxPlacementX );

	return newObject.id;
}