//
// Created by olivier on 17/11/2021.
//

#include "camera.h"

#include "OneLife/gameSource/groundSprites.h"

extern doublePair lastScreenViewCenter;

doublePair getVectorFromCamera( int inMapX, int inMapY )
{
	doublePair vector =
			{ inMapX - lastScreenViewCenter.x / CELL_D,
			  inMapY - lastScreenViewCenter.y / CELL_D };

	return vector;
}