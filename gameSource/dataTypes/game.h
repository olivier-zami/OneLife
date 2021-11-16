//
// Created by olivier on 04/11/2021.
//

#ifndef ONELIFE_DATATYPE_GAME_H
#define ONELIFE_DATATYPE_GAME_H

#include "OneLife/gameSource/GridPos.h"
#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/emotion.h"

typedef struct LiveObject {
	int id;

	int displayID;

	char allSpritesLoaded;

	char onScreen;

	double age;
	double ageRate;

	char finalAgeSet;

	double lastAgeSetTime;

	SimpleVector<int> lineage;

	int lineageEveID;


	char outOfRange;
	char dying;
	char sick;

	char *name;

	char *relationName;

	int curseLevel;

	int excessCursePoints;

	int curseTokenCount;


	// roll back age temporarily to make baby revert to crying
	// when baby speaks
	char tempAgeOverrideSet;
	double tempAgeOverride;
	double tempAgeOverrideSetTime;


	int foodStore;
	int foodCapacity;

	int maxFoodStore;
	int maxFoodCapacity;

	// -1 unless we're currently being held
	// by an adult
	int heldByAdultID;

	// -1 if not set
	// otherwise, ID of adult that is holding us according to pending
	// messages, but not according to already-played messages
	int heldByAdultPendingID;


	// usually 0, unless we're being held by an adult
	// and just got put down
	// then we slide back into position
	doublePair heldByDropOffset;

	// the actual world pos we were last held at
	char lastHeldByRawPosSet;
	doublePair lastHeldByRawPos;

	// track this so that we only send one jump message even if
	// the player clicks more than once before the server registers the
	// jump
	double jumpOutOfArmsSentTime;

	// true if locally-controlled baby is attempting to jump out of arms
	char babyWiggle;
	double babyWiggleProgress;


	// usually 0, but used to slide into and out of riding position
	doublePair ridingOffset;


	// 0 or positive holdingID means holding nothing or an object
	// a negative number here means we're holding another player (baby)
	// and the number, made positive, is the ID of the other player
	int holdingID;
	int lastHoldingID;

	char holdingFlip;

	double lastFlipSendTime;
	char lastFlipSent;

	char heldPosOverride;
	char heldPosOverrideAlmostOver;
	doublePair heldObjectPos;
	double heldObjectRot;
	int heldPosSlideStepCount;

	AnimType curAnim;
	AnimType lastAnim;
	double lastAnimFade;

	// anim tracking for held object
	AnimType curHeldAnim;
	AnimType lastHeldAnim;
	double lastHeldAnimFade;



	// furture states that curAnim should fade to, one at a time
	SimpleVector<AnimType> *futureAnimStack;
	SimpleVector<AnimType> *futureHeldAnimStack;

	// store frame counts in fractional form
	// this allows animations that can vary in speed without
	// ever experiencing discontinuities
	// (an integer frame count, with a speed modifier applied later
	// could jump backwards in time when the modifier changes)
	double animationFrameCount;
	double heldAnimationFrameCount;

	double lastAnimationFrameCount;
	double lastHeldAnimationFrameCount;


	double frozenRotFrameCount;
	double heldFrozenRotFrameCount;

	char frozenRotFrameCountUsed;
	char heldFrozenRotFrameCountUsed;


	float heat;


	int numContained;
	int *containedIDs;
	SimpleVector<int> *subContainedIDs;

	ClothingSet clothing;
	// stacks of items contained in each piece of clothing
	SimpleVector<int> clothingContained[ NUM_CLOTHING_PIECES ];

	float clothingHighlightFades[ NUM_CLOTHING_PIECES ];

	int currentMouseOverClothingIndex;


	// current fractional grid position and speed
	doublePair currentPos;
	// current speed is move delta per frame
	double currentSpeed;

	// current move speed in grid cells per sec
	double currentGridSpeed;


	// for instant reaction to move command when server hasn't
	// responded yet
	// in grid spaces per sec
	double lastSpeed;

	// recompute speed periodically during move so that we don't
	// fall behind when frame rate fluctuates
	double timeOfLastSpeedUpdate;

	// destination grid position
	int xd;
	int yd;

	// true if xd,yd set based on a truncated PM from the server
	char destTruncated;


	// use a waypoint along the way during pathfinding.
	// path must pass through this point on its way to xd,yd
	char useWaypoint;
	int waypointX, waypointY;
	// max path length to find that uses waypoint
	// if waypoint-including path is longer than this
	// a path stopping at the waypoint will be used instead
	// and xd, yd will be repaced with waypoint
	int maxWaypointPathLength;


	// last confirmed stationary position of this
	// object on the server (from the last player_update)
	int xServer;
	int yServer;


	int pathLength;
	GridPos *pathToDest;


	int closestDestIfPathFailedX;
	int closestDestIfPathFailedY;


	int currentPathStep;
	doublePair currentMoveDirection;

	int numFramesOnCurrentStep;

	char onFinalPathStep;


	// how long whole move should take
	double moveTotalTime;

	// wall clock time in seconds object should arrive
	double moveEtaTime;

	// skip drawing this object
	char hide;

	char inMotion;

	int lastMoveSequenceNumber;

	char displayChar;

	int actionTargetX;
	int actionTargetY;

	// tweak for when we are performing an action on a moving object
	// that hasn't reach its destination yet.  actionTargetX,Y is the
	// destination, but this is the closest cell where it was at
	// when we clicked on it.
	int actionTargetTweakX;
	int actionTargetTweakY;

	char pendingAction;
	float pendingActionAnimationProgress;
	double pendingActionAnimationStartTime;

	double lastActionSendStartTime;
	// how long it took server to get back to us with a PU last
	// time we sent an action.  Most recent round-trip time
	double lastResponseTimeDelta;


	// NULL if no active speech
	char *currentSpeech;
	double speechFade;
	// wall clock time when speech should start fading
	double speechFadeETATime;

	char speechIsSuccessfulCurse;


	char shouldDrawPathMarks;
	double pathMarkFade;


	// messages that arrive while we're still showing our current
	// movement animation
	SimpleVector<char*> pendingReceivedMessages;
	char somePendingMessageIsMoreMovement;


	// NULL if none
	Emotion *currentEmot;
	// wall clock time when emot clears
	double emotClearETATime;

	SimpleVector<Emotion*> permanentEmots;


	char killMode;
	int killWithID;

} LiveObject;

typedef struct GraveInfo {
	GridPos worldPos;
	char *relationName;
	// wall clock time when grave was created
	// (for old graves, estimated based on grave age
	// and current age rate)
	double creationTime;

	// if server sends -1 for grave age, we don't display age
	char creationTimeUnknown;

	// to prevent YEARS display from ticking up while we
	// are still mousing over (violates the erased pencil consistency)
	// -1 if not set
	int lastMouseOverYears;
	// last time we displayed a mouse-over label for this
	// used to detect when we've moused away, even if not mousing
	// over another grave
	double lastMouseOverTime;
} GraveInfo;

typedef struct OwnerInfo {
	GridPos worldPos;

	SimpleVector<int> *ownerList;
} OwnerInfo;

typedef struct PointerHitRecord {
	int closestCellX;
	int closestCellY;
	int hitSlotIndex;

	char hit;
	char hitSelf;

	char hitOtherPerson;
	int hitOtherPersonID;

	int hitClothingIndex;

	// when we click in a square, only count as hitting something
	// if we actually clicked the object there.  Else, we can walk
	// there if unblocked.
	char hitAnObject;

	// for case where we hit an object that we remembered placing
	// which may be behind
	// should NEVER click through a person
	char hitOurPlacement;

	// true if hitOurPlacement happened THROUGH another non-person object
	char hitOurPlacementBehind;


} PointerHitRecord;

typedef struct OldArrow {
	int i;
	float heat;
	float fade;
} OldArrow;

typedef struct HomeArrow {
	// true for at most one arrow, the current one
	char solid;

	// fade for erased arrows
	float fade;
} HomeArrow;

// for objects moving in-transit in special cases where we can't store
// them in the map (if they're moving onto an occupied space that should
// only change when they're done moving)
// We track them separately from the map until they are done moving
typedef struct ExtraMapObject {
	int objectID;

	double moveSpeed;
	doublePair moveOffset;

	double animationFrameCount;
	double animationLastFrameCount;

	double animationFrozenRotFrameCount;

	AnimType curAnimType;
	AnimType lastAnimType;
	double lastAnimFade;
	char flip;

	SimpleVector<int> containedStack;
	SimpleVector< SimpleVector<int> > subContainedStack;
} ExtraMapObject;

typedef struct OffScreenSound {
	doublePair pos;

	double fade;
	// wall clock time when should start fading
	double fadeETATime;

	char red;
} OffScreenSound;

#endif //ONELIFE_DATATYPE_GAME_H
