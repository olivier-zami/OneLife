//
// Created by olivier on 21/04/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_CONNECTION_H
#define ONELIFE_SERVER_DATATYPE_CONNECTION_H

#include <cstdint>
#include "../../../third_party/minorGems/network/Socket.h"
#include "../../../third_party/minorGems/network/web/WebRequest.h"
#include "../../../commonSource/dataType/messageType.h"
#include "../../curses.h"

// for incoming socket connections that are still in the login process
typedef struct FreshConnection {
	Socket *sock;
	SimpleVector<char> *sockBuffer;

	unsigned int sequenceNumber;
	char *sequenceNumberString;

	WebRequest *ticketServerRequest;
	char ticketServerAccepted;
	char lifeTokenSpent;

	float fitnessScore;

	double ticketServerRequestStartTime;

	char error;
	const char *errorCauseString;

	double rejectedSendTime;

	char shutdownMode;

	// for tracking connections that have failed to LOGIN
	// in a timely manner
	double connectionStartTimeSeconds;

	char *email;
	uint32_t hashedSpawnSeed;

	int tutorialNumber;
	CurseStatus curseStatus;

	char *twinCode;
	int twinCount;

} FreshConnection;

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
