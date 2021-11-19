//
// Created by olivier on 19/11/2021.
//

#include "misc.h"

#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/dataTypes/ui.h"

extern SimpleVector<LocationSpeech> locationSpeech;

void clearLocationSpeech()
{
	for( int i=0; i<locationSpeech.size(); i++ ) {
		delete [] locationSpeech.getElementDirect( i ).speech;
	}
	locationSpeech.deleteAll();
}
