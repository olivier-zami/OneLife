//
// Created by olivier on 16/11/2021.
//

#ifndef ONELIFE_MATH_MISC_H
#define ONELIFE_MATH_MISC_H

#include "minorGems/game/doublePair.h"

double recalcOffsetX( double x, bool force = false );
double recalcOffsetY( double y );
doublePair recalcOffset( doublePair ofs, bool force = false );
double getBoundedRandom( int inX, int inY, double inUpper, double inLower );

#endif //ONELIFE_MATH_MISC_H
