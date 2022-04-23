//
// Created by olivier on 20/04/2022.
//

#include "geometry.h"

#include <cmath>

double intDist( int inXA, int inYA, int inXB, int inYB )
{
	double dx = (double)inXA - (double)inXB;
	double dy = (double)inYA - (double)inYB;

	return sqrt(  dx * dx + dy * dy );
}

/**
 *
 * @param inXA
 * @param inYA
 * @param inXB
 * @param inYB
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isGridAdjacentDiag( int inXA, int inYA, int inXB, int inYB )
{
	if( isGridAdjacent( inXA, inYA, inXB, inYB ) ) {
		return true;
	}

	if( abs( inXA - inXB ) == 1 && abs( inYA - inYB ) == 1 ) {
		return true;
	}

	return false;
}

/**
 *
 * @param inA
 * @param inB
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isGridAdjacentDiag( GridPos inA, GridPos inB )
{
	return isGridAdjacentDiag( inA.x, inA.y, inB.x, inB.y );
}

/**
 *
 * @param inXA
 * @param inYA
 * @param inXB
 * @param inYB
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char isGridAdjacent( int inXA, int inYA, int inXB, int inYB )
{
	if( ( abs( inXA - inXB ) == 1 && inYA == inYB )
		||
		( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {

		return true;
	}

	return false;
}

/**
 *
 * @param inA
 * @param inB
 * @return
 * @note from server/server.cpp => server/main.cpp
 */
char equal(GridPos inA, GridPos inB)
{
	if (inA.x == inB.x && inA.y == inB.y) { return true; }
	return false;
}
