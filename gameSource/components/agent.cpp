//
// Created by olivier on 15/11/2021.
//

#include "agent.h"

#include <cstdio>
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/game.h"
#include "OneLife/gameSource/procedures/maths/gridPos.h"
#include "OneLife/gameSource/ageControl.h"

extern int baseFramesPerSecond;
extern double frameRateFactor;

void printPath( GridPos *inPath, int inLength )
{
	for( int i=0; i<inLength; i++ ) printf( "(%d,%d) ", inPath[i].x, inPath[i].y );
	printf( "\n" );
}

void removeDoubleBacksFromPath( GridPos **inPath, int *inLength )
{
	SimpleVector<GridPos> filteredPath;
	int dbA = -1;
	int dbB = -1;
	int longestDB = 0;
	GridPos *path = *inPath;
	int length = *inLength;
	for( int e=0; e<length; e++ )
	{
		for( int f=e; f<length; f++ )
		{
			if( equal( path[e], path[f] ) )
			{
				int dist = f - e;
				if( dist > longestDB )
				{
					dbA = e;
					dbB = f;
					longestDB = dist;
				}
			}
		}
	}

	if( longestDB > 0 )
	{
		printf( "Removing loop with %d elements\n", longestDB );

		for( int e=0; e<=dbA; e++ )
		{
			filteredPath.push_back(path[e] );
		}

		// skip loop between
		for( int e=dbB + 1; e<length; e++ )
		{
			filteredPath.push_back(path[e] );
		}
		*inLength = filteredPath.size();
		delete [] path;
		*inPath = filteredPath.getElementArray();
	}
}

double computeCurrentAgeNoOverride( LiveObject *inObj )
{
	if( inObj->finalAgeSet ) {
		return inObj->age;
	}
	else
	{
		return inObj->age + inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime );
	}
}

double computeCurrentAge( LiveObject *inObj )
{
	if( inObj->finalAgeSet )
	{
		return inObj->age;
	}
	else
	{
		if( inObj->tempAgeOverrideSet )
		{
			double curTime = game_getCurrentTime();
			if( curTime - inObj->tempAgeOverrideSetTime < 5 )
			{
				// baby cries for 5 seconds each time they speak
				// update age using clock
				return computeDisplayAge( inObj->tempAgeOverride + inObj->ageRate * ( curTime - inObj->tempAgeOverrideSetTime ) );
			}
			else {
				// temp override over
				inObj->tempAgeOverrideSet = false;
			}
		}
		// update age using clock
		return computeDisplayAge( inObj->age + inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime ) );
	}
}

/**
 *
 * @param inObject
 * @param inPathLength
 * @return
 * @note: measure a possibly truncated path, compensating for diagonals
 */
double measurePathLength( LiveObject *inObject, int inPathLength )
{
	// diags are square root of 2 in length
	double diagLength = 1.4142356237;
	double totalLength = 0;
	if( inPathLength < 2 )
	{
		return totalLength;
	}
	GridPos lastPos = inObject->pathToDest[0];
	for( int i=1; i<inPathLength; i++ )
	{
		GridPos thisPos = inObject->pathToDest[i];
		if( thisPos.x != lastPos.x &&
			thisPos.y != lastPos.y ) {
			totalLength += diagLength;
		}
		else
		{
			// not diag
			totalLength += 1;
		}
		lastPos = thisPos;
	}
	return totalLength;
}

void updateMoveSpeed( LiveObject *inObject )
{
	double etaSec = inObject->moveEtaTime - game_getCurrentTime();

	double moveLeft = measurePathLength( inObject, inObject->pathLength ) -
					  measurePathLength( inObject, inObject->currentPathStep + 1 );

	// count number of turns, which we execute faster than we should
	// because of path smoothing,
	// and use them to reduce overall speed to compensate
	int numTurns = 0;

	if( inObject->currentPathStep < inObject->pathLength - 1 )
	{
		GridPos lastDir = sub(
				inObject->pathToDest[inObject->currentPathStep + 1],
				inObject->pathToDest[inObject->currentPathStep] );

		for( int p=inObject->currentPathStep+1; p<inObject->pathLength -1; p++ )
		{
			GridPos dir = sub(inObject->pathToDest[p+1],inObject->pathToDest[p] );
			if( !equal( dir, lastDir ) )
			{
				numTurns++;
				lastDir = dir;
			}
		}
	}
	double turnTimeBoost = 0.08 * numTurns;
	etaSec += turnTimeBoost;
	if( etaSec < 0.1 )
	{
		// less than 1/10 of a second
		// this includes 0 values and negative values
		// we DO NOT want infinite or negative move speeds
		printf( "updateMoveSpeed sees etaSec of %f, too low, upping to 0.1 sec\n", etaSec );
		etaSec = 0.1;
	}
	double speedPerSec = moveLeft / etaSec;
	// pretend that frame rate is constant
	double fps = baseFramesPerSecond / frameRateFactor;
	inObject->currentSpeed = speedPerSec / fps;
	printf( "fixed speed = %f\n", inObject->currentSpeed );
	inObject->currentGridSpeed = speedPerSec;
	// slow move speed for testing
	//inObject->currentSpeed *= 0.25;
	inObject->timeOfLastSpeedUpdate = game_getCurrentTime();
}

void fixSingleStepPath( LiveObject *inObject )
{
	printf( "Fix for overtruncated, single-step path for player %d\n", inObject->id );

	// trimmed path too short
	// needs to have at least
	// a start and end pos

	// give it an artificial
	// start pos
	doublePair nextWorld = gridToDouble(inObject->pathToDest[0] );
	doublePair vectorAway;
	if( ! equal(inObject->currentPos,nextWorld ) )
	{
		vectorAway = normalize(
				sub(
						inObject->
								currentPos,
						nextWorld ) );
	}
	else
	{
		vectorAway.x = 1;
		vectorAway.y = 0;
	}
	GridPos oldPos = inObject->pathToDest[0];
	delete [] inObject->pathToDest;
	inObject->pathLength = 2;
	inObject->pathToDest = new GridPos[2];
	inObject->pathToDest[0].x = oldPos.x + vectorAway.x;
	inObject->pathToDest[0].y = oldPos.y + vectorAway.y;
	inObject->pathToDest[1] = oldPos;
}

void addNewAnimDirect( LiveObject *inObject, AnimType inNewAnim )
{
	inObject->lastAnim = inObject->curAnim;
	inObject->curAnim = inNewAnim;
	inObject->lastAnimFade = 1;
	inObject->lastAnimationFrameCount = inObject->animationFrameCount;
	if( inObject->lastAnim == moving )
	{
		inObject->frozenRotFrameCount = inObject->lastAnimationFrameCount;
		inObject->frozenRotFrameCountUsed = false;
	}
	else if( inObject->curAnim == moving &&
			 inObject->lastAnim != moving &&
			 inObject->frozenRotFrameCountUsed )
	{
		// switching back to moving
		// resume from where frozen
		inObject->animationFrameCount = inObject->frozenRotFrameCount;
	}
	else if( ( inObject->curAnim == ground || inObject->curAnim == ground2 )
			 &&
			 inObject->lastAnim == held )
	{
		// keep old frozen frame count as we transition away
		// from held
	}
}

void addNewHeldAnimDirect( LiveObject *inObject, AnimType inNewAnim )
{
	inObject->lastHeldAnim = inObject->curHeldAnim;
	inObject->curHeldAnim = inNewAnim;
	inObject->lastHeldAnimFade = 1;
	inObject->lastHeldAnimationFrameCount = inObject->heldAnimationFrameCount;
	if( inObject->lastHeldAnim == moving )
	{
		inObject->heldFrozenRotFrameCount =
				inObject->lastHeldAnimationFrameCount;
		inObject->heldFrozenRotFrameCountUsed = false;
	}
	else if( inObject->curHeldAnim == moving &&
			 inObject->lastHeldAnim != moving &&
			 inObject->heldFrozenRotFrameCountUsed )
	{
		// switching back to moving
		// resume from where frozen
		inObject->heldAnimationFrameCount = inObject->heldFrozenRotFrameCount;
	}
	else if( ( inObject->curHeldAnim == ground ||
			   inObject->curHeldAnim == ground2 )
			 &&
			 inObject->lastHeldAnim == held )
	{
		// keep old frozen frame count as we transition away
		// from held
	}
}

void addNewAnimPlayerOnly( LiveObject *inObject, AnimType inNewAnim )
{
	if( inObject->curAnim != inNewAnim || inObject->futureAnimStack->size() > 0 )
	{
		// if we're still in the middle of fading, finish the fade,
		// by pushing this animation on the stack...
		// ...but NOT if we're fading TO ground.
		// Cut that off, and start our next animation right away.
		if( inObject->lastAnimFade != 0 && inObject->curAnim != ground && inObject->curAnim != ground2 )
		{

			// don't double stack
			if( inObject->futureAnimStack->size() == 0 ||
				inObject->futureAnimStack->getElementDirect(
						inObject->futureAnimStack->size() - 1 )
				!= inNewAnim ) {

				// don't push another animation after ground
				// that animation will replace ground (no need to go to
				// ground between animations.... can just go straight
				// to the next animation
				char foundNonGround = false;

				while( ! foundNonGround && inObject->futureAnimStack->size() > 0 )
				{
					int prevAnim = inObject->futureAnimStack->getElementDirect( inObject->futureAnimStack->size() - 1 );
					if( prevAnim == ground || prevAnim == ground2 )
					{
						inObject->futureAnimStack->deleteElement(inObject->futureAnimStack->size() - 1 );
					}
					else
					{
						foundNonGround = true;
					}
				}
				inObject->futureAnimStack->push_back( inNewAnim );
			}
		}
		else
		{
			addNewAnimDirect( inObject, inNewAnim );
		}
	}
}

void addNewAnim( LiveObject *inObject, AnimType inNewAnim )
{
	addNewAnimPlayerOnly( inObject, inNewAnim );
	AnimType newHeldAnim = inNewAnim;
	if( inObject->holdingID < 0 )
	{
		// holding a baby
		// never show baby's moving animation
		// baby always stuck in held animation when being held
		newHeldAnim = held;
	}
	else if( inObject->holdingID > 0
		&& ( newHeldAnim == ground || newHeldAnim == ground2 || newHeldAnim == doing || newHeldAnim == eating ) )
	{
		// ground is used when person comes to a hault,
		// but for the held item, we should still show the held animation
		// same if person is starting a doing or eating animation
		newHeldAnim = held;
	}

	if( inObject->curHeldAnim != newHeldAnim )
	{
		if( inObject->lastHeldAnimFade != 0 )
		{
			// don't double stack
			if( inObject->futureHeldAnimStack->size() == 0
				|| inObject->futureHeldAnimStack->getElementDirect(inObject->futureHeldAnimStack->size()-1)!=newHeldAnim)
			{
				inObject->futureHeldAnimStack->push_back( newHeldAnim );
			}
		}
		else
		{
			addNewHeldAnimDirect( inObject, newHeldAnim );
		}
	}
}

char nearEndOfMovement( LiveObject *inPlayer )
{
	if( inPlayer->currentSpeed == 0  )
	{
		return true;
	}
	else if( inPlayer->currentPathStep >= inPlayer->pathLength - 2 )
	{
		return true;
	}
	return false;
}