//
// Created by olivier on 14/04/2022.
//

#include "GridPos.h"

char isAdjacent(GridPos inPos, int inX, int inY)
{
	if (inX <= inPos.x + 1 && inX >= inPos.x - 1 && inY <= inPos.y + 1 && inY >= inPos.y - 1) { return true; }
	return false;
}