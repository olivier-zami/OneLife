//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_MATH_GRIDPOS_H
#define ONELIFE_MATH_GRIDPOS_H

#include "minorGems/game/doublePair.h"
#include "OneLife/gameSource/GridPos.h"

char equal( GridPos inA, GridPos inB );
double distance2( GridPos inA, GridPos inB );
doublePair gridToDouble( GridPos inGridPos );
GridPos sub( GridPos inA, GridPos inB );

#endif //ONELIFE_MATH_GRIDPOS_H
