//
// Created by olivier on 20/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_HANDLER_PLAYER_H
#define ONELIFE_SERVER_COMPONENT_HANDLER_PLAYER_H

#include "../../../gameSource/objectBank.h"
#include "../../dataType/LiveObject.h"

namespace OneLife::server::handler
{
	class Player
	{
	};
}

void freePlayerContainedArrays( LiveObject *inPlayer );
void setDeathReason( LiveObject *inPlayer, const char *inTag, int inOptionalID = 0 );
int objectRecordToID( ObjectRecord *inRecord );

#endif //ONELIFE_SERVER_COMPONENT_HANDLER_PLAYER_H
