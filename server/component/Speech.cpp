//
// Created by olivier on 17/04/2022.
//

#include "Speech.h"

#include "../../commonSource/math/GridPos.h"
#include "../../gameSource/objectBank.h"
#include "Map.h"

int numSpeechPipes = 0;
int maxSpeechPipeIndex = 0;
SimpleVector<GridPos> *speechPipesIn = NULL;
SimpleVector<GridPos> *speechPipesOut = NULL;

/**
 *
 * @param inX
 * @param inY
 * @param outIndicies
 * @note from server/map.cpp
 * // gets speech pipe indices for IN pipes at or adjacent to inX,inY
// vector passed in through outIndicies will be filled with indices
 */
void getSpeechPipesIn(int inX, int inY, SimpleVector<int> *outIndicies)
{
	for (int i = 0; i < numSpeechPipes; i++)
	{
		for (int p = 0; p < speechPipesIn[i].size(); p++)
		{
			GridPos inPos = speechPipesIn[i].getElementDirect(p);
			if (isAdjacent(inPos, inX, inY))
			{

				// make sure pipe-in is still here
				int id = getMapObjectRaw(inPos.x, inPos.y);

				char stillHere = false;

				if (id > 0)
				{
					ObjectRecord *oIn = getObject(id);

					if (oIn->speechPipeIn && oIn->speechPipeIndex == i) { stillHere = true; }
				}
				if (!stillHere)
				{
					speechPipesIn[i].deleteElement(p);
					p--;
				}
				else
				{
					outIndicies->push_back(i);
					break;
				}
			}
		}
	}
}

/**
 *
 * @return
 * @note from gameSource/objectBank.cpp
 */
int getMaxSpeechPipeIndex()
{
	return maxSpeechPipeIndex;
}