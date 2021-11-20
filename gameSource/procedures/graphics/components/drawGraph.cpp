//
// Created by olivier on 20/11/2021.
//

#include "../components.h"

#include "minorGems/game/drawUtils.h"

extern int historyGraphLength;

void drawGraph( SimpleVector<double> *inHistory, doublePair inPos, FloatColor inColor )
{
	double max = 0;
	for( int i=0; i<inHistory->size(); i++ ) {
		double val = inHistory->getElementDirect( i );
		if( val > max ) {
			max = val;
		}
	}

	setDrawColor( 0, 0, 0, 0.5 );
	double graphHeight = 40;
	drawRect( inPos.x - 2,
			inPos.y - 2,
			inPos.x + historyGraphLength + 2,
			inPos.y + graphHeight + 2 );

	setDrawColor( inColor.r, inColor.g, inColor.b, 0.75 );
	for( int i=0; i<inHistory->size(); i++ )
	{
		double val = inHistory->getElementDirect( i );
		double scaledVal = val / max;
		drawRect( inPos.x + i,
				inPos.y,
				inPos.x + i + 1,
				inPos.y + scaledVal * graphHeight );
	}
}

