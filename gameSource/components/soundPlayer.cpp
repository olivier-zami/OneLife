//
// Created by olivier on 19/11/2021.
//

#include "soundPlayer.h"

#include <cstddef>
#include <cstring>
#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/misc.h"
#include "OneLife/gameSource/dataTypes/game.h"

SimpleVector<OffScreenSound> offScreenSounds;

void addOffScreenSound( double inPosX, double inPosY, char *inDescription )
{
	char red = false;
	char *stringPos = strstr( inDescription, "offScreenSound" );
	if( stringPos != NULL ) {
		stringPos = &( stringPos[ strlen( "offScreenSound" ) ] );

		if( strstr( stringPos, "_red" ) == stringPos ) {
			// _red flag next
			red = true;
		}
	}
	double fadeETATime = game_getCurrentTime() + 4;
	doublePair pos = { inPosX, inPosY };
	OffScreenSound s = { pos, 1.0, fadeETATime, red };
	offScreenSounds.push_back( s );
}