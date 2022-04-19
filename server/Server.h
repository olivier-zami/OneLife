//
// Created by olivier on 18/04/2022.
//

#ifndef ONELIFE_SERVER_H
#define ONELIFE_SERVER_H

#include "../gameSource/GridPos.h"
#include "dataType/LiveObject.h"

#define DECAY_SLOT 1
#define NO_DECAY_SLOT -1

// track currently in-process movements so that we can be queried
// about whether arrival has happened or not
typedef struct MovementRecord
{
	int    x, y;
	double etaTime;
}MovementRecord;

namespace OneLife
{
	class Server
	{
		public:
			Server();
			~Server();

			bool initMap();
			void routine_dbPut();
	};
}

int *getContainedRaw(int inX, int inY, int *outNumContained, int inSubCont = 0);
void setSlotItemsNoDecay(int inX, int inY, int inSubCont, char inNoDecay);
int checkDecayObject(int inX, int inY, int inID);
void clearAllContained(int inX, int inY, int inSubCont = 0);
void shrinkContainer(int inX, int inY, int inNumNewSlots, int inSubCont = 0);
char getSlotItemsNoDecay(int inX, int inY, int inSubCont);
GridPos getClosestPlayerPos( int inX, int inY );
GridPos computePartialMoveSpot( LiveObject *inPlayer );
int computePartialMovePathStep( LiveObject *inPlayer );

#endif //ONELIFE_SERVER_H
