//
// Created by olivier on 15/11/2021.
//

#ifndef ONELIFE_GAME_AGENT_H
#define ONELIFE_GAME_AGENT_H

#include "../../GridPos.h"
#include "../../dataTypes/game.h"
#include "../../animationBank.h"

namespace OneLife::game
{
	class Agent {};

	void computePathToDest(
			LiveObject *inObject,
			int mMapD,
			int mMapOffsetX,
			int mMapOffsetY,
			int *mMap);
}

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
char *getDisplayObjectDescription( int inID );//TODO: put this in inquirer, AgentBank or something
char checkIfHeldContChanged( LiveObject *inOld, LiveObject *inNew );


#endif //ONELIFE_GAME_AGENT_H
