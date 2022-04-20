//
// Created by olivier on 20/04/2022.
//

#include "LiveObject.h"

/**
 *
 * @param inPlayer
 * @return
 * @note from server/main.cpp => server/server.cpp
 * // compute closest starting position part way along
// path
// (-1 if closest spot is starting spot not included in path steps)
 */
int computePartialMovePathStep( LiveObject *inPlayer )
{
	double fractionDone =
			( Time::getCurrentTime() -
			  inPlayer->moveStartTime )
			/ inPlayer->moveTotalSeconds;

	if( fractionDone > 1 ) {
		fractionDone = 1;
	}

	int c =
			lrint( ( inPlayer->pathLength ) *
				   fractionDone );
	return c - 1;
}

/**
 *
 * @param inPlayer
 * @return
 * @note from server/main.cpp => server/server.cpp
 */
GridPos computePartialMoveSpot( LiveObject *inPlayer ) {

	int c = computePartialMovePathStep( inPlayer );

	if( c >= 0 ) {

		GridPos cPos = inPlayer->pathToDest[c];

		return cPos;
	}
	else {
		GridPos cPos = { inPlayer->xs, inPlayer->ys };

		return cPos;
	}
}