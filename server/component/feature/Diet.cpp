//
// Created by olivier on 22/04/2022.
//

#include "Diet.h"

extern int satisfiedEmotionIndex;
extern SimpleVector<int> newEmotIndices;
extern SimpleVector<int> newEmotPlayerIDs;
extern SimpleVector<int> newEmotTTLs;

int canYumChainBreak = 0;
double minAgeForCravings = 10;

/**
 *
 * @param inPlayer
 * @param inFoodEatenID
 * @param inFedSelf
 * @note from server/server.cpp => server/main.cpp
 */
void updateYum( LiveObject *inPlayer, int inFoodEatenID, char inFedSelf)
{

	char wasYummy = true;

	if( ! isYummy( inPlayer, inFoodEatenID ) ) {
		wasYummy = false;

		// chain broken

		// only feeding self can break chain
		if( inFedSelf && canYumChainBreak ) {
			inPlayer->yummyFoodChain.deleteAll();
		}
	}


	ObjectRecord *o = getObject( inFoodEatenID );

	if( o->isUseDummy ) {
		inFoodEatenID = o->useDummyParent;
	}


	// add to chain
	// might be starting a new chain
	// (do this if fed yummy food by other player too)
	if( wasYummy ||
		inPlayer->yummyFoodChain.size() == 0 ) {

		int eatenID = inFoodEatenID;

		if( isReallyYummy( inPlayer, eatenID ) ) {
			inPlayer->yummyFoodChain.push_back( eatenID );
		}

		// now it is possible to "grief" the craving pool
		// by eating high tech food without craving them
		// but this also means that it requires more effort to
		// cheese the craving system by deliberately eating
		// easy food first in an advanced town
		logFoodDepth( inPlayer->lineageEveID, eatenID );

		if( eatenID == inPlayer->cravingFood.foodID &&
			computeAge( inPlayer ) >= minAgeForCravings ) {

			for( int i=0; i< inPlayer->cravingFood.bonus; i++ ) {
				// add extra copies to YUM chain as a bonus
				inPlayer->yummyFoodChain.push_back( eatenID );
			}

			// craving satisfied, go on to next thing in list
			inPlayer->cravingFood =
					getCravedFood( inPlayer->lineageEveID,
								   inPlayer->parentChainLength,
								   inPlayer->cravingFood );
			// reset generational bonus counter
			inPlayer->cravingFoodYumIncrement = 1;

			// flag them for getting a new craving message
			inPlayer->cravingKnown = false;

			// satisfied emot

			if( satisfiedEmotionIndex != -1 ) {
				inPlayer->emotFrozen = false;
				inPlayer->emotUnfreezeETA = 0;

				newEmotPlayerIDs.push_back( inPlayer->id );

				newEmotIndices.push_back( satisfiedEmotionIndex );
				// 3 sec
				newEmotTTLs.push_back( 1 );

				// don't leave starving status, or else non-starving
				// change might override our satisfied emote
				inPlayer->starving = false;
			}
		}
	}


	int currentBonus = inPlayer->yummyFoodChain.size() - 1;

	if( currentBonus < 0 ) {
		currentBonus = 0;
	}

	if( wasYummy ) {
		// only get bonus if actually was yummy (whether fed self or not)
		// chain not broken if fed non-yummy by other, but don't get bonus
		inPlayer->yummyBonusStore += currentBonus;
	}

}

/**
 *
 * @param inPlayer
 * @param inObjectID
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isYummy( LiveObject *inPlayer, int inObjectID )
{
	ObjectRecord *o = getObject( inObjectID );

	if( o->isUseDummy ) {
		inObjectID = o->useDummyParent;
		o = getObject( inObjectID );
	}

	if( o->foodValue == 0 ) {
		return false;
	}

	if( inObjectID == inPlayer->cravingFood.foodID &&
		computeAge( inPlayer ) >= minAgeForCravings ) {
		return true;
	}

	for( int i=0; i<inPlayer->yummyFoodChain.size(); i++ ) {
		if( inObjectID == inPlayer->yummyFoodChain.getElementDirect(i) ) {
			return false;
		}
	}
	return true;
}

/**
 *
 * @param inPlayer
 * @param inObjectID
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isReallyYummy( LiveObject *inPlayer, int inObjectID ) {

	// whether the food is actually not in the yum chain
	// return false for meh food that the player is craving
	// which is displayed "yum" client-side

	ObjectRecord *o = getObject( inObjectID );

	if( o->isUseDummy ) {
		inObjectID = o->useDummyParent;
		o = getObject( inObjectID );
	}

	if( o->foodValue == 0 ) {
		return false;
	}

	for( int i=0; i<inPlayer->yummyFoodChain.size(); i++ ) {
		if( inObjectID == inPlayer->yummyFoodChain.getElementDirect(i) ) {
			return false;
		}
	}
	return true;
}