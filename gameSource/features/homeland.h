//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_GAME_FEATURE_HOMELAND_H
#define ONELIFE_GAME_FEATURE_HOMELAND_H

#include <cstddef>
#include <cmath>
#include "minorGems/game/doublePair.h"
#include "OneLife/gameSource/dataTypes/const.h"
#include "OneLife/gameSource/GridPos.h"

// most recent home at end
typedef struct {
	GridPos pos;
	char ancient;
} HomePos;

GridPos *getHomeLocation();
void removeHomeLocation( int inX, int inY );
void addHomeLocation( int inX, int inY );
void addAncientHomeLocation( int inX, int inY );
int getHomeDir( doublePair inCurrentPlayerPos, double *outTileDistance = NULL, char *outTooClose = NULL );

#endif //ONELIFE_GAME_FEATURE_HOMELAND_H
