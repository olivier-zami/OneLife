//
// Created by olivier on 07/12/2021.
//

#include "string.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/game/Font.h"

extern float gui_fov_scale_hud;
extern Font *handwritingFont;

double getLongestLine( char *inMessage )
{
	double longestLine = 0;

	int numLines;
	char **lines = split( inMessage, "#", &numLines );

	for( int l=0; l<numLines; l++ )
	{
		double len = handwritingFont->measureString( lines[l] ) / gui_fov_scale_hud;
		if( len > longestLine ) {longestLine = len;}
		delete [] lines[l];
	}
	delete [] lines;

	return longestLine;
}