//
// Created by olivier on 24/10/2021.
//

#include "drawString.h"

//#include <SDL/SDL.h>
//#include <GL/gl.h>
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/stringUtils.h"

extern doublePair lastScreenViewCenter;//game procedure/game
extern char initDone;//game procedure/game editor
extern double viewWidth;
extern double viewHeight;
extern double viewHeightFraction;
extern Font *mainFont;

void drawString( const char *inString, char inForceCenter ) {

	setDrawColor( 1, 1, 1, 0.75 );

	doublePair messagePos = lastScreenViewCenter;

	TextAlignment align = alignCenter;

	if( initDone && !inForceCenter ) {
		// transparent message
		setDrawColor( 1, 1, 1, 0.75 );

		// stick messages in corner
		messagePos.x -= viewWidth / 2;

		messagePos.x +=  20;



		messagePos.y += (viewWidth * viewHeightFraction) /  2;

		messagePos.y -= 32;

		align = alignLeft;
	}
	else {
		// fully opaque message
		setDrawColor( 1, 1, 1, 1 );

		// leave centered
	}


	int numLines;

	char **lines = split( inString, "\n", &numLines );

	for( int i=0; i<numLines; i++ ) {


		mainFont->drawString( lines[i], messagePos, align );
		messagePos.y -= 32;

		delete [] lines[i];
	}
	delete [] lines;
}