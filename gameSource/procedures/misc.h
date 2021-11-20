//
// Created by olivier on 19/11/2021.
//

#ifndef ONELIFE_GAME_PROCEDURE_MISC_H
#define ONELIFE_GAME_PROCEDURE_MISC_H

#include "OneLife/gameSource/dataTypes/game.h"

void clearLocationSpeech();
void stripDescriptionComment( char *inString );
void dummyFunctionA();
double getLongestLine( char *inMessage );
char shouldCreationSoundPlay( int inOldID, int inNewID );
char isFood( int inID );//TODO: put this in inquire
char isCategory( int inID );//TODO: put this in inquire
void addToGraph( SimpleVector<double> *inHistory, double inValue );

#endif //ONELIFE_GAME_PROCEDURE_MISC_H
