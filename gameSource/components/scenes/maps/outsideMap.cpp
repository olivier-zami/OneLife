//
// Created by olivier on 15/11/2021.
//

#include "outsideMap.h"

#include <cmath>

OneLife::game::OutsideMap::OutsideMap() {}
OneLife::game::OutsideMap::~OutsideMap() {}

/**********************************************************************************************************************/
int OneLife::game::getMapIndex( int inWorldX, int inWorldY, int mMapD, int mMapOffsetX, int mMapOffsetY)
{
	//GridPos mapTarget = getMapPos( inWorldX, inWorldY );//TODO remove livingLifePage::getMapPos() references
	GridPos mapTarget = getMapPos( inWorldX, inWorldY, mMapD, mMapOffsetX, mMapOffsetY );

	if( mapTarget.y >= 0 && mapTarget.y < mMapD &&
		mapTarget.x >= 0 && mapTarget.x < mMapD ) {

		return mapTarget.y * mMapD + mapTarget.x;
	}
	return -1;
}

GridPos OneLife::game::getMapPos( int inWorldX, int inWorldY, int mMapD, int mMapOffsetX, int mMapOffsetY) {
	GridPos p =
			{ inWorldX - mMapOffsetX + mMapD / 2,
			  inWorldY - mMapOffsetY + mMapD / 2 };

	return p;
}

/**********************************************************************************************************************/

char isGridAdjacent( int inXA, int inYA, int inXB, int inYB )
{
	if( ( abs( inXA - inXB ) == 1 && inYA == inYB )
		||
		( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {

		return true;
	}

	return false;
}

char isInBounds( int inX, int inY, int inMapD )
{
	if( inX < 0 || inY < 0 || inX > inMapD - 1 || inY > inMapD - 1 ) {
		return false;
	}
	return true;
}