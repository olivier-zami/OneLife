//
// Created by olivier on 15/11/2021.
//

#include "outsideMap.h"

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