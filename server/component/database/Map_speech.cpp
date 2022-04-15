//
// Created by olivier on 14/04/2022.
//

#include "../../../commonSource/math/GridPos.h"

SimpleVector<GridPos> *speechPipesIn = NULL;
SimpleVector<GridPos> *speechPipesOut = NULL;

static int numSpeechPipes = 0;

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
