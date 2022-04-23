//
// Created by olivier on 20/04/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_INFO_H
#define ONELIFE_SERVER_DATATYPE_INFO_H

#include "../component/feature/diplomacy/dataType.h"

#include "../../gameSource/GridPos.h"

typedef struct ChangePosition {
	int x, y;

	// true if update should be sent to everyone regardless
	// of distance (like position of a new player in the world,
	// or the removal of a player).
	char global;

	int responsiblePlayerID;

	// for movement changes
	int oldX, oldY;
	float speed;

} ChangePosition;

typedef struct GraveInfo
{
	GridPos pos;
	int playerID;
	// eve that started the line of this dead person
	// used for tracking whether grave is part of player's family or not
	int lineageEveID;
} GraveInfo;

typedef struct GraveMoveInfo {
	GridPos posStart;
	GridPos posEnd;
	int swapDest;
} GraveMoveInfo;

typedef struct MoveRecord {
	int playerID;
	char *formatString;
	int absoluteX, absoluteY;
} MoveRecord;

typedef struct UpdateRecord{
	char *formatString;
	char posUsed;
	int absolutePosX, absolutePosY;
	GridPos absoluteActionTarget;
	int absoluteHeldOriginX, absoluteHeldOriginY;
} UpdateRecord;

typedef struct KillState
{
	int killerID;
	int killerWeaponID;
	int targetID;
	double killStartTime;
	double emotStartTime;
	int emotRefreshSeconds;
} KillState;

typedef struct FullMapContained{
	int numContained;
	int *containedIDs;
	timeSec_t *containedEtaDecays;
	SimpleVector<int> *subContainedIDs;
	SimpleVector<timeSec_t> *subContainedEtaDecays;
} FullMapContained;

typedef struct {
	unsigned int uniqueLoadID;
	char *mapFileName;
	char fileOpened;
	FILE *file;
	int x, y;
	double startTime;
	int stepCount;
} TutorialLoadProgress;

typedef struct FlightDest {
	int playerID;
	GridPos destPos;
} FlightDest;

typedef struct ForcedEffects {
	// -1 if no emot specified
	int emotIndex;
	int ttlSec;

	char foodModifierSet;
	double foodCapModifier;

	char feverSet;
	float fever;
} ForcedEffects;

// tracking spots on map that inflicted a mortal wound
// put them on timeout afterward so that they don't attack
// again immediately
typedef struct DeadlyMapSpot {
	GridPos pos;
	double timeOfAttack;
} DeadlyMapSpot;

#endif //ONELIFE_SERVER_DATATYPE_INFO_H
