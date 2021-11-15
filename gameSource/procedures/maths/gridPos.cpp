//
// Created by olivier on 15/11/2021.
//

#include "gridPos.h"

char equal( GridPos inA, GridPos inB )
{
	if( inA.x == inB.x && inA.y == inB.y ) {
		return true;
	}
	return false;
}

double distance2( GridPos inA, GridPos inB )
{
	int dX = inA.x - inB.x;
	int dY = inA.y - inB.y;
	return dX * dX + dY * dY;
}

doublePair gridToDouble( GridPos inGridPos )
{
	doublePair d = { (double) inGridPos.x, (double) inGridPos.y };
	return d;
}

GridPos sub( GridPos inA, GridPos inB )
{
	GridPos result = { inA.x - inB.x, inA.y - inB.y };
	return result;
}

