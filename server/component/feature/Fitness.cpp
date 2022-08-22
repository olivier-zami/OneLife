//
// Created by olivier on 22/04/2022.
//

#include "Fitness.h"

#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../fitnessScore.h"
#include "../../game/Server.h"

extern SimpleVector<LiveObject> players;
extern double forceDeathAge;

/**
 *
 * @param nextPlayer
 * @note from server/server.cpp => server/main.cpp
 */
void logFitnessDeath( LiveObject *nextPlayer )
{
	double curTime = Time::getCurrentTime();
	for( int i=0; i<players.size(); i++ ) {

		LiveObject *o = players.getElement( i );

		if( o->error ||
			o->isTutorial ||
			o->id == nextPlayer->id ) {
			continue;
		}

		SimpleVector<double> *newAncestorLifeEndTimeSeconds = new SimpleVector<double>();

		for( int e=0; e< o->ancestorIDs->size(); e++ ) {

			if( o->ancestorIDs->getElementDirect( e ) == nextPlayer->id ) {
				newAncestorLifeEndTimeSeconds->push_back( curTime );
			} else {
				newAncestorLifeEndTimeSeconds->push_back( o->ancestorLifeEndTimeSeconds->getElementDirect( e ) );
			}
		}

		for( int e=0; e< o->ancestorIDs->size(); e++ ) {
			o->ancestorLifeEndTimeSeconds->deleteElement( e );
		}
		delete o->ancestorLifeEndTimeSeconds;
		o->ancestorLifeEndTimeSeconds = newAncestorLifeEndTimeSeconds;

	}

	// log this death for fitness purposes,
	// for both tutorial and non


	// if this person themselves died before their mom, gma, etc.
	// remove them from the "ancestor" list of everyone who is older than they
	// are and still alive

	// You only get genetic points for ma, gma, and other older ancestors
	// if you are alive when they die.

	// This ends an exploit where people suicide as a baby (or young person)
	// yet reap genetic benefit from their mother living a long life
	// (your mother, gma, etc count for your genetic score if you yourself
	//  live beyond 3, so it is in your interest to protect them)
	double deadPersonAge = computeAge( nextPlayer );
	if( deadPersonAge < forceDeathAge ) {
		for( int i=0; i<players.size(); i++ ) {

			LiveObject *o = players.getElement( i );

			if( o->error ||
				o->isTutorial ||
				o->id == nextPlayer->id ) {
				continue;
			}

			if( computeAge( o ) < deadPersonAge ) {
				// this person was born after the dead person
				// thus, there's no way they are their ma, gma, etc.
				continue;
			}

			for( int e=0; e< o->ancestorIDs->size(); e++ ) {
				if( o->ancestorIDs->getElementDirect( e ) == nextPlayer->id ) {
					o->ancestorIDs->deleteElement( e );

					delete [] o->ancestorEmails->getElementDirect( e );
					o->ancestorEmails->deleteElement( e );

					delete [] o->ancestorRelNames->getElementDirect( e );
					o->ancestorRelNames->deleteElement( e );

					o->ancestorLifeStartTimeSeconds->deleteElement( e );
					o->ancestorLifeEndTimeSeconds->deleteElement( e );

					break;
				}
			}
		}
	}


	SimpleVector<int> emptyAncestorIDs;
	SimpleVector<char*> emptyAncestorEmails;
	SimpleVector<char*> emptyAncestorRelNames;
	SimpleVector<double> emptyAncestorLifeStartTimeSeconds;
	SimpleVector<double> emptyAncestorLifeEndTimeSeconds;


	//SimpleVector<int> *ancestorIDs = nextPlayer->ancestorIDs;
	SimpleVector<char*> *ancestorEmails = nextPlayer->ancestorEmails;
	SimpleVector<char*> *ancestorRelNames = nextPlayer->ancestorRelNames;
	//SimpleVector<double> *ancestorLifeStartTimeSeconds =
	//    nextPlayer->ancestorLifeStartTimeSeconds;
	SimpleVector<double> *ancestorLifeEndTimeSeconds =
			nextPlayer->ancestorLifeEndTimeSeconds;

	SimpleVector<char*> ancestorData;
	double deadPersonLifeStartTime = nextPlayer->trueStartTimeSeconds;
	double ageRate = getAgeRate();

	for( int i=0; i<ancestorEmails->size(); i++ ) {

		double endTime = ancestorLifeEndTimeSeconds->getElementDirect( i );
		double parentingTime = 0.0;

		if( endTime > 0 ) {
			parentingTime = ageRate * (endTime - deadPersonLifeStartTime);
		} else {
			parentingTime = ageRate * (curTime - deadPersonLifeStartTime);
		}

		char buffer[16];
		snprintf(buffer, sizeof buffer, "%.6f", parentingTime);

		ancestorData.push_back( buffer );

	}

	logFitnessDeath( players.size(),
					 nextPlayer->email,
					 nextPlayer->name, nextPlayer->displayID,
					 computeAge( nextPlayer ),
					 ancestorEmails,
					 ancestorRelNames,
					 &ancestorData
	);
}
