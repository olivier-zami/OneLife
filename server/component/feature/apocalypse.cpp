//
// Created by olivier on 18/04/2022.
//

#include "apocalypse.h"

#include "../../../gameSource/GridPos.h"
#include "../../../gameSource/objectBank.h"

int apocalypsePossible = 0;
char apocalypseTriggered = false;
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