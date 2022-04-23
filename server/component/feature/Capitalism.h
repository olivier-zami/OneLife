//
// Created by olivier on 22/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_FEATURE_CAPITALISM_H
#define ONELIFE_SERVER_COMPONENT_FEATURE_CAPITALISM_H

#include "../../../gameSource/GridPos.h"
#include "../../dataType/LiveObject.h"

namespace OneLife::server::feature
{
	class Capitalism
	{

	};
}

char *getOwnershipString( int inX, int inY );
char *getOwnershipString( GridPos inPos );
void removeAllOwnership( LiveObject *inPlayer );
char isKnownOwned( LiveObject *inPlayer, int inX, int inY );
char isKnownOwned( LiveObject *inPlayer, GridPos inPos );
char isOwned( LiveObject *inPlayer, int inX, int inY );
char isOwned( LiveObject *inPlayer, GridPos inPos );

#endif //ONELIFE_SERVER_COMPONENT_FEATURE_CAPITALISM_H
