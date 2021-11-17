//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_GAME_COMPONENT_OUTSIDEMAP_H
#define ONELIFE_GAME_COMPONENT_OUTSIDEMAP_H

#include "OneLife/gameSource/GridPos.h"//TODO put in dataType

namespace OneLife::game
{
	class OutsideMap{};

	int getMapIndex( int inWorldX, int inWorldY, int mMapD, int mMapOffsetX, int mMapOffsetY );
	GridPos getMapPos( int inWorldX, int inWorldY, int mMapD, int mMapOffsetX, int mMapOffsetY );
}


#endif //ONELIFE_GAME_COMPONENT_OUTSIDEMAP_H
