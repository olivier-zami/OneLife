//
// Created by olivier on 15/11/2021.
//

#include "family.h"

#include "minorGems/util/stringUtils.h"
#include "OneLife/gameSource/game.h"
#include "OneLife/gameSource/objectBank.h"

char *getRelationName( SimpleVector<int> *ourLin,
					   SimpleVector<int> *theirLin,
					   int ourID, int theirID,
					   int ourDisplayID, int theirDisplayID,
					   double ourAge, double theirAge,
					   int ourEveID, int theirEveID )
{
	ObjectRecord *theirDisplayO = getObject( theirDisplayID );

	char theyMale = false;

	if( theirDisplayO != NULL ) {
		theyMale = theirDisplayO->male;
	}


	if( ourLin->size() == 0 && theirLin->size() == 0 ) {
		// both eve, no relation
		return NULL;
	}

	const char *main = "";
	char grand = false;
	int numGreats = 0;
	int cousinNum = 0;
	int cousinRemovedNum = 0;

	char found = false;

	for( int i=0; i<theirLin->size(); i++ ) {
		if( theirLin->getElementDirect( i ) == ourID ) {
			found = true;

			if( theyMale ) {
				main = translate( "son" );
			}
			else {
				main = translate( "daughter" );
			}
			if( i > 0  ) {
				grand = true;
			}
			numGreats = i - 1;
		}
	}

	if( ! found ) {
		for( int i=0; i<ourLin->size(); i++ ) {
			if( ourLin->getElementDirect( i ) == theirID ) {
				found = true;
				main = translate( "mother" );
				if( i > 0  ) {
					grand = true;
				}
				numGreats = i - 1;
			}
		}
	}

	char big = false;
	char little = false;
	char twin = false;
	char identical = false;

	if( ! found ) {
		// not a direct descendent or ancestor

		// look for shared relation
		int ourMatchIndex = -1;
		int theirMatchIndex = -1;

		for( int i=0; i<ourLin->size(); i++ ) {
			for( int j=0; j<theirLin->size(); j++ ) {

				if( ourLin->getElementDirect( i ) ==
					theirLin->getElementDirect( j ) ) {
					ourMatchIndex = i;
					theirMatchIndex = j;
					break;
				}
			}
			if( ourMatchIndex != -1 ) {
				break;
			}
		}

		if( ourMatchIndex == -1 ) {

			if( ourEveID != -1 && theirEveID != -1 &&
				ourEveID == theirEveID ) {
				// no shared lineage, but same eve beyond lineage cuttoff
				return stringDuplicate( translate( "distantRelative" ) );
			}

			return NULL;
		}

		found = true;

		if( theirMatchIndex == 0 && ourMatchIndex == 0 ) {
			if( theyMale ) {
				main = translate( "brother" );
			}
			else {
				main = translate( "sister" );
			}

			if( ourAge < theirAge - 0.1 ) {
				big = true;
			}
			else if( ourAge > theirAge + 0.1 ) {
				little = true;
			}
			else {
				// close enough together in age
				twin = true;

				if( ourDisplayID == theirDisplayID ) {
					identical = true;
				}
			}
		}
		else if( theirMatchIndex == 0 ) {
			if( theyMale ) {
				main = translate( "uncle" );
			}
			else {
				main = translate( "aunt" );
			}
			numGreats = ourMatchIndex - 1;
		}
		else if( ourMatchIndex == 0 ) {
			if( theyMale ) {
				main = translate( "nephew" );
			}
			else {
				main = translate( "niece" );
			}
			numGreats = theirMatchIndex - 1;
		}
		else {
			// cousin of some kind

			main = translate( "cousin" );

			// shallowest determines cousin number
			// diff determines removed number
			if( ourMatchIndex <= theirMatchIndex ) {
				cousinNum = ourMatchIndex;
				cousinRemovedNum = theirMatchIndex - ourMatchIndex;
			}
			else {
				cousinNum = theirMatchIndex;
				cousinRemovedNum = ourMatchIndex - theirMatchIndex;
			}
		}
	}


	SimpleVector<char> buffer;

	buffer.appendElementString( translate( "your" ) );
	buffer.appendElementString( " " );


	if( numGreats <= 4 ) {
		for( int i=0; i<numGreats; i++ ) {
			buffer.appendElementString( translate( "great" ) );
		}
	}
	else {
		char *greatCount = autoSprintf( "%dX %s",
				numGreats, translate( "great") );
		buffer.appendElementString( greatCount );
		delete [] greatCount;
	}


	if( grand ) {
		buffer.appendElementString( translate( "grand" ) );
	}

	if( cousinNum > 0 ) {
		int remainingCousinNum = cousinNum;

		if( cousinNum >= 30 ) {
			buffer.appendElementString( translate( "distant" ) );
			remainingCousinNum = 0;
		}

		if( cousinNum > 20 && cousinNum < 30 ) {
			buffer.appendElementString( translate( "twenty" ) );
			remainingCousinNum = cousinNum - 20;
		}

		if( remainingCousinNum > 0  ) {
			char *numth = autoSprintf( "%dth", remainingCousinNum );
			buffer.appendElementString( translate( numth ) );
			delete [] numth;
		}
		buffer.appendElementString( " " );
	}

	if( little ) {
		buffer.appendElementString( translate( "little" ) );
		buffer.appendElementString( " " );
	}
	else if( big ) {
		buffer.appendElementString( translate( "big" ) );
		buffer.appendElementString( " " );
	}
	else if( twin ) {
		if( identical ) {
			buffer.appendElementString( translate( "identical" ) );
			buffer.appendElementString( " " );
		}

		buffer.appendElementString( translate( "twin" ) );
		buffer.appendElementString( " " );
	}


	buffer.appendElementString( main );

	if( cousinRemovedNum > 0 ) {
		buffer.appendElementString( " " );

		if( cousinRemovedNum > 9 ) {
			buffer.appendElementString( translate( "manyTimes" ) );
		}
		else {
			char *numTimes = autoSprintf( "%dTimes", cousinRemovedNum );
			buffer.appendElementString( translate( numTimes ) );
			delete [] numTimes;
		}
		buffer.appendElementString( " " );
		buffer.appendElementString( translate( "removed" ) );
	}

	return buffer.getElementString();
}

char *getRelationName( LiveObject *inOurObject, LiveObject *inTheirObject )
{
	SimpleVector<int> *ourLin = &( inOurObject->lineage );
	SimpleVector<int> *theirLin = &( inTheirObject->lineage );

	int ourID = inOurObject->id;
	int theirID = inTheirObject->id;


	return getRelationName( ourLin, theirLin, ourID, theirID,
			inOurObject->displayID, inTheirObject->displayID,
			inOurObject->age,
			inTheirObject->age,
			inOurObject->lineageEveID,
			inTheirObject->lineageEveID );
}

