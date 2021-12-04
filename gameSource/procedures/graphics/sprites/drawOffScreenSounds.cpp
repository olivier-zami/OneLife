//
// Created by olivier on 16/11/2021.
//

#include "strings.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"
#include "OneLife/gameSource/misc.h"
#include "OneLife/gameSource/dataTypes/game.h"

extern SimpleVector<OffScreenSound> offScreenSounds;
extern double viewWidth;
extern double viewHeight;
extern double frameRateFactor;
extern doublePair lastScreenViewCenter;

void OneLife::graphic::drawOffScreenSounds(SpriteHandle mChalkBlotSprite)
{
	if( offScreenSounds.size() == 0 ) return;
	double xRadius = viewWidth / 2 - 32;
	double yRadius = viewHeight / 2 - 32;
	FloatColor red = { 0.65, 0, 0, 1 };
	FloatColor white = { 1, 1, 1, 1 };
	FloatColor black = { 0, 0, 0, 1 };
	double curTime = game_getCurrentTime();
	for( int i=0; i<offScreenSounds.size(); i++ )
	{
		OffScreenSound *s = offScreenSounds.getElement( i );
		if( s->fadeETATime <= curTime )
		{
			s->fade -= 0.05 * frameRateFactor;
			if( s->fade <= 0 ) {
				offScreenSounds.deleteElement( i );
				i--;
				continue;
			}
		}

		if( fabs( s->pos.x - lastScreenViewCenter.x ) > xRadius
			|| fabs( s->pos.y - lastScreenViewCenter.y ) > yRadius )
		{
			// off screen
			doublePair v = sub( s->pos, lastScreenViewCenter );// relative vector
			doublePair normV = normalize( v );
			double xScale = fabs( xRadius / normV.x );// first extend in x dir to edge
			doublePair edgeV = mult( normV, xScale );
			if( fabs( edgeV.y ) > yRadius )
			{
				// off top/bottom
				double yScale = fabs( yRadius / normV.y );// extend in y dir instead
				edgeV = mult( normV, yScale );
			}
			if( edgeV.y < -270 )
			{
				edgeV.y = -270;
			}
			doublePair drawPos = add( edgeV, lastScreenViewCenter );
			FloatColor *textColor = &black;
			FloatColor *bgColor = &white;

			if( s->red )
			{
				textColor = &white;
				bgColor = &red;
			}
			OneLife::graphic::drawChalkBackgroundString( drawPos,
					"!",
					s->fade,
					100,
					NULL,
					-1,
					bgColor, textColor, mChalkBlotSprite );
		}
	}
}

