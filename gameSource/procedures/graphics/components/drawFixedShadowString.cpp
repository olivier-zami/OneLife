//
// Created by olivier on 20/11/2021.
//

#include "../components.h"

#include "minorGems/game/Font.h"

extern Font *numbersFontFixed;

void drawFixedShadowString( const char *inString, doublePair inPos )
{
	setDrawColor( 0, 0, 0, 1 );
	numbersFontFixed->drawString( inString, inPos, alignLeft );

	setDrawColor( 1, 1, 1, 1 );

	inPos.x += 2;
	inPos.y -= 2;
	numbersFontFixed->drawString( inString, inPos, alignLeft );
}
