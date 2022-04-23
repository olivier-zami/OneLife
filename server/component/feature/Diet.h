//
// Created by olivier on 22/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_FEATURE_DIET_H
#define ONELIFE_SERVER_COMPONENT_FEATURE_DIET_H

#include "../../dataType/LiveObject.h"

namespace OneLife::server::feature
{
	class Diet
	{

	};
}

void updateYum( LiveObject *inPlayer, int inFoodEatenID, char inFedSelf = true );
char isYummy( LiveObject *inPlayer, int inObjectID );
char isReallyYummy( LiveObject *inPlayer, int inObjectID );

#endif //ONELIFE_SERVER_COMPONENT_FEATURE_DIET_H
