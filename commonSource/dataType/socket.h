//
// Created by olivier on 24/08/2022.
//

#ifndef oneLife_common_dataType_socket_H
#define oneLife_common_dataType_socket_H

#include <cstdint>
#include "../../third_party/minorGems/network/Socket.h"
#include "../../third_party/minorGems/network/web/WebRequest.h"
#include "../../server/curses.h" //TODO: move curse.h in common/dataType/features dataType

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

#endif //oneLife_common_dataType_socket_H
