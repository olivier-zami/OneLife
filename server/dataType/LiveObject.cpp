//
// Created by olivier on 20/04/2022.
//

#include "LiveObject.h"

#include "../component/handler/Player.h"
#include "../Server.h"

extern double oldAge;
extern double adultAge;
extern double fertileAge;
extern double baseWalkSpeed;
extern double forceDeathAge;

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

/**
 *
 * @param inPlayer
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
double computeMoveSpeed( LiveObject *inPlayer )
{
	double age = computeAge( inPlayer );


	double speed = baseWalkSpeed;

	// baby moves at 360 pixels per second, or 6 pixels per frame
	double babySpeedFactor = 0.75;

	double fullSpeedAge = 10.0;


	if( age < fullSpeedAge ) {

		double speedFactor = babySpeedFactor +
							 ( 1.0 - babySpeedFactor ) * age / fullSpeedAge;

		speed *= speedFactor;
	}


	// for now, try no age-based speed decrease
	/*
	if( age < 20 ) {
		speed *= age / 20;
		}
	if( age > 40 ) {
		// half speed by 60, then keep slowing down after that
		speed -= (age - 40 ) * 2.0 / 20.0;

		}
	*/
	// no longer slow down with hunger
	/*
	int foodCap = computeFoodCapacity( inPlayer );


	if( inPlayer->foodStore <= foodCap / 2 ) {
		// jumps instantly to 1/2 speed at half food, then decays after that
		speed *= inPlayer->foodStore / (double) foodCap;
		}
	*/



	// apply character's speed mult
	speed *= getObject( inPlayer->displayID )->speedMult;


	char riding = false;

	if( inPlayer->holdingID > 0 ) {
		ObjectRecord *r = getObject( inPlayer->holdingID );

		if( r->clothing == 'n' ) {
			// clothing only changes your speed when it's worn
			speed *= r->speedMult;
		}

		if( r->rideable ) {
			riding = true;
		}
	}


	if( !riding ) {
		// clothing can affect speed

		for( int i=0; i<NUM_CLOTHING_PIECES; i++ ) {
			ObjectRecord *c = clothingByIndex( inPlayer->clothing, i );

			if( c != NULL ) {

				speed *= c->speedMult;
			}
		}

		if( inPlayer->tripping ) {
			speed *= 1.2;
		}
		else if( inPlayer->drunkennessEffect ) {
			speed *= 0.9;
		}
	}

	// never move at 0 speed, divide by 0 errors for eta times
	if( speed < 0.01 ) {
		speed = 0.01;
	}


	// after all multipliers, make sure it's a whole number of pixels per frame

	double pixelsPerFrame = speed * 128.0 / 60.0;


	if( pixelsPerFrame > 0.5 ) {
		// can round to at least one pixel per frame
		pixelsPerFrame = lrint( pixelsPerFrame );
	}
	else {
		// fractional pixels per frame

		// ensure a whole number of frames per pixel
		double framesPerPixel = 1.0 / pixelsPerFrame;

		framesPerPixel = lrint( framesPerPixel );

		pixelsPerFrame = 1.0 / framesPerPixel;
	}

	speed = pixelsPerFrame * 60 / 128.0;

	return speed;
}

/**
 *
 * @param inPlayer
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
int computeFoodCapacity( LiveObject *inPlayer )
{
	int ageInYears = lrint( computeAge( inPlayer ) );

	int returnVal = 0;

	if( ageInYears < oldAge ) {

		if( ageInYears > adultAge - 4 ) {
			ageInYears = adultAge - 4;
		}

		returnVal = ageInYears + 4;
	}
	else {
		// food capacity decreases as we near death
		int cap = forceDeathAge - ageInYears + 4;

		if( cap < 4 ) {
			cap = 4;
		}

		int lostBars = 20 - cap;

		if( lostBars > 0 && inPlayer->fitnessScore > 0 ) {

			// consider effect of fitness on reducing lost bars

			// for now, let's make it quadratic
			double maxLostBars =
					16 - 16 * pow( inPlayer->fitnessScore / 60.0, 2 );

			if( lostBars > maxLostBars ) {
				lostBars = maxLostBars;
			}
		}

		returnVal = 20 - lostBars;
	}

	return ceil( returnVal * inPlayer->foodCapModifier );
}

/**
 *
 * @param inPlayer
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
double computeAge( LiveObject *inPlayer )
{
	double age = computeAge( inPlayer->lifeStartTimeSeconds );
	if( age >= forceDeathAge ) {
		setDeathReason( inPlayer, "age" );

		inPlayer->error = true;

		age = forceDeathAge;
	}
	return age;
}

/**
 *
 * @param inLifeStartTimeSeconds
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
double computeAge( double inLifeStartTimeSeconds )
{
	double deltaSeconds = Time::getCurrentTime() - inLifeStartTimeSeconds;
	double age = deltaSeconds * getAgeRate();
	return age;
}