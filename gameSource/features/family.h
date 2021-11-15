//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_GAME_FEATURE_FAMILY_H
#define ONELIFE_GAME_FEATURE_FAMILY_H

#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/dataTypes/game.h"

char *getRelationName( SimpleVector<int> *ourLin,
					   SimpleVector<int> *theirLin,
					   int ourID, int theirID,
					   int ourDisplayID, int theirDisplayID,
					   double ourAge, double theirAge,
					   int ourEveID, int theirEveID );

char *getRelationName( LiveObject *inOurObject, LiveObject *inTheirObject );

#endif //ONELIFE_GAME_FEATURE_FAMILY_H
