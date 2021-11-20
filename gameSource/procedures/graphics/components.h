//
// Created by olivier on 20/11/2021.
//

#ifndef ONELIFE_GRAPIC_COMPONENTS_H
#define ONELIFE_GRAPIC_COMPONENTS_H

#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"

void drawHUDBarPart( double x, double y, double width, double height );
void drawGraph( SimpleVector<double> *inHistory, doublePair inPos, FloatColor inColor );
void drawFixedShadowString( const char *inString, doublePair inPos );

#endif //ONELIFE_GRAPIC_COMPONENTS_H
