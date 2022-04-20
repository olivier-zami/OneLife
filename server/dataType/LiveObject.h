//
// Created by olivier on 19/04/2022.
//

#ifndef ONELIFE_DATATYPE_LIVEOBJECT_H
#define ONELIFE_DATATYPE_LIVEOBJECT_H

#include "../../third_party/minorGems/network/Socket.h"
#include "../../third_party/minorGems/system/Time.h"
#include "../../third_party/minorGems/util/SimpleVector.h"
#include "../../gameSource/objectBank.h"
#include "../cravings.h"
#include "../curses.h"
#include "../map.h"

#define HEAT_MAP_D 13

typedef struct LiveObject {
	char *email;
	// for tracking old email after player has been deleted
	// but is still on list
	char *origEmail;

	int id;

	// -1 if unknown
	float fitnessScore;


	// object ID used to visually represent this player
	int displayID;

	char *name;
	char nameHasSuffix;
	char *displayedName;

	char *familyName;


	char *lastSay;

	//2HOL additions for: password-protected objects

	//the phrase that player carries as "a secret word"
	char *saidPassword;
	//the phrase that player assigns to an object
	char *assignedPassword;

	CurseStatus curseStatus;

	int curseTokenCount;
	char curseTokenUpdate;


	char isEve;

	char isTutorial;

	// used to track incremental tutorial map loading
	TutorialLoadProgress tutorialLoad;


	GridPos birthPos;
	GridPos originalBirthPos;


	int parentID;

	// 1 for Eve
	int parentChainLength;

	SimpleVector<int> *lineage;

	SimpleVector<int> *ancestorIDs;
	SimpleVector<char*> *ancestorEmails;
	SimpleVector<char*> *ancestorRelNames;
	SimpleVector<double> *ancestorLifeStartTimeSeconds;
	SimpleVector<double> *ancestorLifeEndTimeSeconds;


	// id of Eve that started this line
	int lineageEveID;



	// time that this life started (for computing age)
	// not actual creation time (can be adjusted to tweak starting age,
	// for example, in case of Eve who starts older).
	double lifeStartTimeSeconds;

	// time when this player actually died
	double deathTimeSeconds;


	// the wall clock time when this life started
	// used for computing playtime, not age
	double trueStartTimeSeconds;


	double lastSayTimeSeconds;

	// held by other player?
	char heldByOther;
	int heldByOtherID;
	char everHeldByParent;

	// player that's responsible for updates that happen to this
	// player during current step
	int responsiblePlayerID;

	// start and dest for a move
	// same if reached destination
	int xs;
	int ys;

	int xd;
	int yd;

	// next player update should be flagged
	// as a forced position change
	char posForced;

	char waitingForForceResponse;

	int lastMoveSequenceNumber;


	int facingLeft;
	double lastFlipTime;


	int pathLength;
	GridPos *pathToDest;

	char pathTruncated;

	char firstMapSent;
	int lastSentMapX;
	int lastSentMapY;

	double moveTotalSeconds;
	double moveStartTime;

	int facingOverride;
	int actionAttempt;
	GridPos actionTarget;

	int holdingID;

	// absolute time in seconds that what we're holding should decay
	// or 0 if it never decays
	timeSec_t holdingEtaDecay;


	// where on map held object was picked up from
	char heldOriginValid;
	int heldOriginX;
	int heldOriginY;


	// track origin of held separate to use when placing a grave
	int heldGraveOriginX;
	int heldGraveOriginY;
	int heldGravePlayerID;


	// if held object was created by a transition on a target, what is the
	// object ID of the target from the transition?
	int heldTransitionSourceID;


	int numContained;
	int *containedIDs;
	timeSec_t *containedEtaDecays;

	// vector of sub-contained for each contained item
	SimpleVector<int> *subContainedIDs;
	SimpleVector<timeSec_t> *subContainedEtaDecays;


	// if they've been killed and part of a weapon (bullet?) has hit them
	// this will be included in their grave
	int embeddedWeaponID;
	timeSec_t embeddedWeaponEtaDecay;

	// and what original weapon killed them?
	int murderSourceID;
	char holdingWound;

	// who killed them?
	int murderPerpID;
	char *murderPerpEmail;

	// or if they were killed by a non-person, what was it?
	int deathSourceID;

	// true if this character landed a mortal wound on another player
	char everKilledAnyone;

	// true in case of sudden infant death
	char suicide;


	Socket *sock;
	SimpleVector<char> *sockBuffer;

	// indicates that some messages were sent to this player this
	// frame, and they need a FRAME terminator message
	char gotPartOfThisFrame;


	char isNew;
	char isNewCursed;
	char firstMessageSent;

	char inFlight;


	char dying;
	// wall clock time when they will be dead
	double dyingETA;

	// in cases where their held wound produces a forced emot
	char emotFrozen;
	double emotUnfreezeETA;
	int emotFrozenIndex;

	char starving;


	char connected;

	char error;
	const char *errorCauseString;



	int customGraveID;

	char *deathReason;

	char deleteSent;
	// wall clock time when we consider the delete good and sent
	// and can close their connection
	double deleteSentDoneETA;

	char deathLogged;

	char newMove;

	// heat map that player carries around with them
	// every time they stop moving, it is updated to compute
	// their local temp
	float heatMap[ HEAT_MAP_D * HEAT_MAP_D ];

	// net heat of environment around player
	// map is tracked in heat units (each object produces an
	// integer amount of heat)
	// this is in base heat units, range 0 to infinity
	float envHeat;

	// amount of heat currently in player's body, also in
	// base heat units
	float bodyHeat;


	// used track current biome heat for biome-change shock effects
	float biomeHeat;
	float lastBiomeHeat;


	// body heat normalized to [0,1], with targetHeat at 0.5
	float heat;

	// flags this player as needing to recieve a heat update
	char heatUpdate;

	// wall clock time of last time this player was sent
	// a heat update
	double lastHeatUpdate;

	// true if heat map features player surrounded by walls
	char isIndoors;



	int foodStore;

	double foodCapModifier;

	double drunkenness;
	bool drunkennessEffect;
	double drunkennessEffectETA;

	bool tripping;
	bool gonnaBeTripping;
	double trippingEffectStartTime;
	double trippingEffectETA;


	double fever;


	// wall clock time when we should decrement the food store
	double foodDecrementETASeconds;

	// should we send player a food status message
	char foodUpdate;

	// info about the last thing we ate, for FX food messages sent
	// just to player
	int lastAteID;
	int lastAteFillMax;

	// this is for PU messages sent to everyone
	char justAte;
	int justAteID;

	// chain of non-repeating foods eaten
	SimpleVector<int> yummyFoodChain;

	// how many bonus from yummy food is stored
	// these are used first before food is decremented
	int yummyBonusStore;


	ClothingSet clothing;

	timeSec_t clothingEtaDecay[NUM_CLOTHING_PIECES];

	SimpleVector<int> clothingContained[NUM_CLOTHING_PIECES];

	SimpleVector<timeSec_t>
			clothingContainedEtaDecays[NUM_CLOTHING_PIECES];

	char needsUpdate;
	char updateSent;
	char updateGlobal;

	// babies born to this player
	SimpleVector<timeSec_t> *babyBirthTimes;
	SimpleVector<int> *babyIDs;

	// for CURSE MY BABY after baby is dead/deleted
	char *lastBabyEmail;


	// wall clock time after which they can have another baby
	// starts at 0 (start of time epoch) for non-mothers, as
	// they can have their first baby right away.
	timeSec_t birthCoolDown;

	bool declaredInfertile;

	timeSec_t lastRegionLookTime;

	double playerCrossingCheckTime;


	char monumentPosSet;
	GridPos lastMonumentPos;
	int lastMonumentID;
	char monumentPosSent;


	char holdingFlightObject;

	char vogMode;
	GridPos preVogPos;
	GridPos preVogBirthPos;
	int vogJumpIndex;
	char postVogMode;


	// list of positions owned by this player
	SimpleVector<GridPos> ownedPositions;

	// list of owned positions that this player has heard about
	SimpleVector<GridPos> knownOwnedPositions;

	// email of last baby that we had that did /DIE
	char *lastSidsBabyEmail;

	//2HOL mechanics to read written objects
	//positions already read while in range
	SimpleVector<GridPos> readPositions;

	//time when read position is expired and can be read again
	SimpleVector<double> readPositionsETA;

	SimpleVector<int> permanentEmots;

	//2HOL: last time player does something
	double lastActionTime;

	//2HOL: player is either disconnected or inactive
	bool isAFK;

	Craving cravingFood;
	int cravingFoodYumIncrement;
	char cravingKnown;

} LiveObject;

int computePartialMovePathStep( LiveObject *inPlayer );
GridPos computePartialMoveSpot( LiveObject *inPlayer );

#endif //ONELIFE_DATATYPE_LIVEOBJECT_H
