//
// Created by olivier on 20/04/2022.
//

#include "Player.h"

#include "../../../third_party/minorGems/network/SocketPoll.h"
#include "../../../third_party/minorGems/util/log/AppLog.h"
#include "../../../gameSource/GridPos.h"

extern SocketPoll sockPoll;

/**
 *
 * @param inPlayer
 * @note from server/server.cpp => server/main.cpp
 */
void freePlayerContainedArrays( LiveObject *inPlayer )
{
	if( inPlayer->containedIDs != NULL ) {
		delete [] inPlayer->containedIDs;
	}
	if( inPlayer->containedEtaDecays != NULL ) {
		delete [] inPlayer->containedEtaDecays;
	}
	if( inPlayer->subContainedIDs != NULL ) {
		delete [] inPlayer->subContainedIDs;
	}
	if( inPlayer->subContainedEtaDecays != NULL ) {
		delete [] inPlayer->subContainedEtaDecays;
	}

	inPlayer->containedIDs = NULL;
	inPlayer->containedEtaDecays = NULL;
	inPlayer->subContainedIDs = NULL;
	inPlayer->subContainedEtaDecays = NULL;
}

/**
 *
 * @param inPlayer
 * @param inTag
 * @param inOptionalID
 * @note from server/server.cpp => server/main.cpp
 */
void setDeathReason( LiveObject *inPlayer, const char *inTag, int inOptionalID)
{

	if( inPlayer->deathReason != NULL ) {
		delete [] inPlayer->deathReason;
	}

	// leave space in front so it works at end of PU line
	if( strcmp( inTag, "killed" ) == 0 ||
		strcmp( inTag, "succumbed" ) == 0 ) {

		inPlayer->deathReason = autoSprintf( " reason_%s_%d",
											 inTag, inOptionalID );
	}
	else {
		// ignore ID
		inPlayer->deathReason = autoSprintf( " reason_%s", inTag );
	}
}

/**
 *
 * @param inRecord
 * @return
 * // returns 0 for NULL
 */
int objectRecordToID( ObjectRecord *inRecord )
{
	if( inRecord == NULL ) {
		return 0;
	}
	else {
		return inRecord->id;
	}
}