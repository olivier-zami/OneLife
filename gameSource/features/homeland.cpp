//
// Created by olivier on 15/11/2021.
//

#include "homeland.h"

#include "minorGems/util/SimpleVector.h"

SimpleVector<HomePos> homePosStack;
SimpleVector<HomePos> oldHomePosStack;

/**
 *
 * @return
 * @note: returns pointer to record, NOT destroyed by caller, or NULL if home unknown
 */
GridPos *getHomeLocation()
{
	int num = homePosStack.size();
	if( num > 0 ) {
		return &( homePosStack.getElement( num - 1 )->pos );
	}
	else {
		return NULL;
	}
}

void removeHomeLocation( int inX, int inY )
{
	for( int i=0; i<homePosStack.size(); i++ ) {
		GridPos p = homePosStack.getElementDirect( i ).pos;

		if( p.x == inX && p.y == inY ) {
			homePosStack.deleteElement( i );
			break;
		}
	}
}

void addHomeLocation( int inX, int inY )
{
	removeHomeLocation( inX, inY );
	GridPos newPos = { inX, inY };
	HomePos p;
	p.pos = newPos;
	p.ancient = false;

	homePosStack.push_back( p );
}

void addAncientHomeLocation( int inX, int inY )
{
	removeHomeLocation( inX, inY );

	// remove all ancient pos
	// there can be only one ancient
	for( int i=0; i<homePosStack.size(); i++ ) {
		if( homePosStack.getElementDirect( i ).ancient ) {
			homePosStack.deleteElement( i );
			i--;
		}
	}

	GridPos newPos = { inX, inY };
	HomePos p;
	p.pos = newPos;
	p.ancient = true;

	homePosStack.push_front( p );
}

/**
 *
 * @param inCurrentPlayerPos
 * @param outTileDistance
 * @param outTooClose
 * @return
 * @note: returns if -1 no home needs to be shown (home unknown) otherwise, returns 0..7 index of arrow
 */
int getHomeDir( doublePair inCurrentPlayerPos,
					   double *outTileDistance,
					   char *outTooClose)
{
	GridPos *p = getHomeLocation();
	if( p == NULL ) {
		return -1;
	}
	if( outTooClose != NULL ) {
		*outTooClose = false;
	}
	doublePair homePos = { (double)p->x, (double)p->y };
	doublePair vector = sub( homePos, inCurrentPlayerPos );
	double dist = length( vector );
	if( outTileDistance != NULL ) {
		*outTileDistance = dist;
	}
	if( dist < 5 ) {
		// too close
		if( outTooClose != NULL ) {
			*outTooClose = true;
		}
		if( dist == 0 ) {
			// can't compute angle
			return -1;
		}
	}
	double a = angle( vector );
	// north is 0
	a -= M_PI / 2;
	if( a <  - M_PI / 8 ) {
		a += 2 * M_PI;
	}
	int index = lrint( 8 * a / ( 2 * M_PI ) );
	return index;
}