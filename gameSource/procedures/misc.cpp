//
// Created by olivier on 19/11/2021.
//

#include "misc.h"

#include "minorGems/util/SimpleVector.h"
//#include "minorGems/util/stringUtils.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/dataTypes/ui.h"
#include "OneLife/gameSource/categoryBank.h"

extern SimpleVector<LocationSpeech> locationSpeech;
extern float gui_fov_scale_hud;
extern int historyGraphLength;

void clearLocationSpeech()
{
	for( int i=0; i<locationSpeech.size(); i++ ) {
		delete [] locationSpeech.getElementDirect( i ).speech;
	}
	locationSpeech.deleteAll();
}

void stripDescriptionComment( char *inString )
{
	// pound sign is used for trailing developer comments
	// that aren't show to end user, cut them off if they exist
	char *firstPound = strstr( inString, "#" );

	if( firstPound != NULL ) {
		firstPound[0] = '\0';
	}
}
/**
 *
 * @note:previous function (step) is so long that it's slowin down Emacs on the following function put a dummy function here to help emacs.
 */
void dummyFunctionA() {
	// call self to avoid compiler warning for unused function
	if( false ) {
		dummyFunctionA();
	}
}

// inNewID > 0
char shouldCreationSoundPlay( int inOldID, int inNewID ) {
	if( inOldID == inNewID ) {
		// no change
		return false;
	}
	// make sure this is really a fresh creation
	// of newID, and not a cycling back around
	// for a reusable object

	// also not useDummies that have the same
	// parent
	char sameParent = false;

	ObjectRecord *obj = getObject( inNewID );

	if( obj->creationSound.numSubSounds == 0 ) {
		return false;
	}

	if( inOldID > 0 && inNewID > 0 ) {
		ObjectRecord *oldObj = getObject( inOldID );

		if( obj->isUseDummy &&
			oldObj->isUseDummy &&
			obj->useDummyParent ==
			oldObj->useDummyParent ) {
			sameParent = true;
		}
		else if( obj->numUses > 1
				 &&
				 oldObj->isUseDummy
				 &&
				 oldObj->useDummyParent
				 == inNewID ) {
			sameParent = true;
		}
		else if( oldObj->numUses > 1
				 &&
				 obj->isUseDummy
				 &&
				 obj->useDummyParent
				 == inOldID ) {
			sameParent = true;
		}
	}

	if( ! sameParent
		&&
		( ! obj->creationSoundInitialOnly
		  ||
		  inOldID <= 0
		  ||
		  ( ! isSpriteSubset( inOldID, inNewID ) ) ) ) {
		return true;
	}
	return false;
}

char isFood( int inID )
{
	if( inID <= 0 ) {
		return false;
	}

	ObjectRecord *o = getObject( inID );

	if( o->foodValue > 0 ) {
		return true;
	}
	else {
		return false;
	}
}

char isCategory( int inID )
{
	if( inID <= 0 ) {
		return false;
	}

	CategoryRecord *c = getCategory( inID );

	if( c == NULL ) {
		return false;
	}
	if( ! c->isPattern && c->objectIDSet.size() > 0 ) {
		return true;
	}
	return false;
}

void addToGraph( SimpleVector<double> *inHistory, double inValue )
{
	inHistory->push_back( inValue );

	while( inHistory->size() > historyGraphLength ) {
		inHistory->deleteElement( 0 );
	}
}


