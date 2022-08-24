//
// Created by olivier on 21/04/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_CONNECTION_H
#define ONELIFE_SERVER_DATATYPE_CONNECTION_H

#include "../../../commonSource/dataType/messageType.h"
#include "../../../gameSource/GridPos.h"

typedef struct ClientMessage {
	messageType type;
	int x, y, c, i, id;

	int trigger;
	int bug;

	// some messages have extra positions attached
	int numExtraPos;

	// NULL if there are no extra
	GridPos *extraPos;

	// null if type not SAY
	char *saidText;

	// null if type not BUG
	char *bugText;

	// for MOVE messages
	int sequenceNumber;

} ClientMessage;

#endif //ONELIFE_SERVER_DATATYPE_CONNECTION_H
