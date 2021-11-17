//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_GAME_AGENT_H
#define ONELIFE_GAME_AGENT_H

#include "OneLife/gameSource/GridPos.h"
#include "OneLife/gameSource/dataTypes/game.h"
#include "OneLife/gameSource/animationBank.h"

class Agent {};

void printPath( GridPos *inPath, int inLength );
void removeDoubleBacksFromPath( GridPos **inPath, int *inLength );
double computeCurrentAgeNoOverride( LiveObject *inObj );
double computeCurrentAge( LiveObject *inObj );
double measurePathLength( LiveObject *inObject, int inPathLength );
void updateMoveSpeed( LiveObject *inObject );
void fixSingleStepPath( LiveObject *inObject );
void addNewAnimDirect( LiveObject *inObject, AnimType inNewAnim );
void addNewHeldAnimDirect( LiveObject *inObject, AnimType inNewAnim );
void addNewAnimPlayerOnly( LiveObject *inObject, AnimType inNewAnim );
void addNewAnim( LiveObject *inObject, AnimType inNewAnim );
char nearEndOfMovement( LiveObject *inPlayer );
LiveObject *getGameObject( int inID );//TODO: put this in livingLifePage->getObject();

#endif //ONELIFE_GAME_AGENT_H
