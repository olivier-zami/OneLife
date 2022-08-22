//
// Created by olivier on 18/04/2022.
//

#ifndef ONELIFE_SERVER_H
#define ONELIFE_SERVER_H

#include "../../gameSource/GridPos.h"
#include "../../gameSource/transitionBank.h"
#include "../component/Map.h"
#include "../dataType/connection.h"
#include "../dataType/LiveObject.h"
#include "../dataType/Settings.h"
#include "minorGems/network/Socket.h"
#include "minorGems/network/SocketServer.h"

namespace OneLife
{
	class Server
	{
		public:
			Server(OneLife::server::Settings settings);
			~Server();

			void loadObjects();

			void start();

			bool initMap();
			void initBiomes();

			bool isLastSendingCanceled();
			bool isLastSendingFailed();

			const char* getErrorMessage();

			void sendAcceptanceMessage(FreshConnection *nextConnection);
			void sendFirstMessages(LiveObject *nextPlayer);
			void sendStartingMap(LiveObject *inO);
			void sendTravelingMap(LiveObject *inO, char inDestOverride = false, int inDestOverrideX = 0, int inDestOverrideY = 0);

			void setErrorMessage(const char* message);

			OneLife::server::Settings settings;
			OneLife::server::Map* worldMapDatabase;

			//!objects settings
			int numObjects;
			ObjectRecord **allObjects;

			//!biomes settings
			SimpleVector<int> biomeList;
			SimpleVector<int> *biomeOrderList;
			SimpleVector<float> *biomeWeightList;

		protected:
			void _procedureCreateNewConnection();//TODO: temporary function or code isolation don't keep it

		private:
			double minMoveTime;
			bool lastSendingCanceled;
			bool lastSendingFailed;
			struct{
				char* content;
				size_t size;
			}errMsg;
			int port;
			int shutdownMode;
			SocketServer *socket;
			char someClientMessageReceived;
	};
}

void writeRecentPlacements();
unsigned char *makeCompressedMessage( char *inMessage, int inLength, int *outLength );
char *getUpdateLineFromRecord(UpdateRecord *inRecord, GridPos inRelativeToPos, GridPos inObserverPos );
char *yellingSpeech( int inSpeakerID, char *inTranslatedPhrase );
char *slurSpeech( int inSpeakerID, char *inTranslatedPhrase, double inDrunkenness );
char wasRecentlyDeadly( GridPos inPos );
void addDeadlyMapSpot( GridPos inPos );
float computeClothingR( LiveObject *inPlayer );
float computeClothingHeat( LiveObject *inPlayer );
void recomputeHeatMap( LiveObject *inPlayer );
void forceObjectToRead( LiveObject *inPlayer, int inObjectID, GridPos inReadPos, bool passToRead );
void swapHeldWithGround(LiveObject *inPlayer, int inTargetID, int inMapX, int inMapY, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout);
char *getUpdateLine( LiveObject *inPlayer, GridPos inRelativeToPos, GridPos inObserverPos, char inDelete, char inPartial = false );
LiveObject *getHitPlayer( int inX, int inY, int inTargetID = -1, char inCountMidPath = false, int inMaxAge = -1, int inMinAge = -1, int *outHitIndex = NULL );
void processWaitingTwinConnection( FreshConnection inConnection );
char addHeldToContainer( LiveObject *inPlayer, int inTargetID, int inContX, int inContY, char inSwap = false );
char addHeldToClothingContainer( LiveObject *inPlayer, int inC, char inWillSwap = false, char *outCouldHaveGoneIn = NULL );
void pickupToHold( LiveObject *inPlayer, int inX, int inY, int inTargetID );
char removeFromClothingContainerToHold( LiveObject *inPlayer, int inC, int inI = -1 );
ObjectRecord **getClothingSlot( LiveObject *targetPlayer, int inIndex );
void removeClothingToHold( LiveObject *nextPlayer, LiveObject *targetPlayer, ObjectRecord **clothingSlot, int clothingSlotIndex );
TransRecord *getBareHandClothingTrans( LiveObject *nextPlayer, ObjectRecord **clothingSlot );
void checkForFoodEatingEmot( LiveObject *inPlayer, int inEatenID );
void drinkAlcohol( LiveObject *inPlayer, int inAlcoholAmount );
void doDrug( LiveObject *inPlayer );
bool clearFrozenEmote( LiveObject *inPlayer, int inEmoteIndex );
void removeAnyKillState( LiveObject *inKiller );
char isAccessBlocked( LiveObject *inPlayer, int inTargetX, int inTargetY, int inTargetID );
LiveObject *getPlayerByName( char *inName, LiveObject *inPlayerSayingName );
void removeKillState( LiveObject *inKiller, LiveObject *inTarget );
void setHeldGraveOrigin( LiveObject *inPlayer, int inX, int inY, int inNewTarget );
char containmentPermitted( int inContainerID, int inContainedID );
int getContainerSwapIndex( LiveObject *inPlayer, int idToAdd, int inStillHeld, int inSearchLimit, int inContX, int inContY );
UpdateRecord getUpdateRecord(LiveObject *inPlayer, char inDelete, char inPartial = false );
float computeHeldHeat( LiveObject *inPlayer );
char checkReadOnly();
double getAgeRate();
char *getHoldingString( LiveObject *inObject );
void makePlayerSay( LiveObject *inPlayer, char *inToSay, bool inPrivate = false );
void holdingSomethingNew( LiveObject *inPlayer, int inOldHoldingID = 0 );
char removeFromContainerToHold( LiveObject *inPlayer,int inContX, int inContY, int inSlotNumber );
void clearPlayerHeldContained( LiveObject *inPlayer );
void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID );
double pickBirthCooldownSeconds();
const char *getCurseWord( char *inSenderEmail, char *inEmail, int inWordIndex );
int countYoungFemalesInLineage( int inLineageEveID );
int getMaxChunkDimension();
LiveObject *getPlayerByEmail( char *inEmail );
void forcePlayerToRead( LiveObject *inPlayer, int inObjectID );
int countFertileMothers();
int countHelplessBabies();
int countFamilies();
char isEveWindow();
int countLivingChildren( int inMotherID );
void monumentStep();
void checkCustomGlobalMessage();
void deleteMembers( FreshConnection *inConnection );
int getLiveObjectIndex( int inID );
void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );
void setFreshEtaDecayForHeld( LiveObject *inPlayer );
void checkSickStaggerTime( LiveObject *inPlayer );
void interruptAnyKillEmots( int inPlayerID, int inInterruptingTTL );
int isGraveSwapDest( int inTargetX, int inTargetY, int inDroppingPlayerID );
void handleMapChangeToPaths(int inX, int inY, ObjectRecord *inNewObject, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );
void transferHeldContainedToMap( LiveObject *inPlayer, int inX, int inY );
double computeFoodDecrementTimeSeconds( LiveObject *inPlayer );
char isFertileAge( LiveObject *inPlayer );
double getPathSpeedModifier( GridPos *inPathPos, int inPathLength );
double measurePathLength( int inXS, int inYS, GridPos *inPathPos, int inPathLength );
ForcedEffects checkForForcedEffects( int inHeldObjectID );
ClientMessage parseMessage( LiveObject *inPlayer, char *inMessage );
void handleForcedBabyDrop(LiveObject *inBabyObject, SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout );
char *isNamedGivingSay( char *inSaidString );
char isYouGivingSay( char *inSaidString );
LiveObject *getClosestOtherPlayer( LiveObject *inThisPlayer, double inMinAge = 0, char inNameMustBeNULL = false );
char *isFamilyNamingSay( char *inSaidString );
char *getUniqueCursableName( char *inPlayerName, char *outSuffixAdded, char inIsEve );
char getFemale( LiveObject *inPlayer );
char *isInfertilityDeclaringSay( char *inSaidString );
char isYouForgivingSay( char *inSaidString );
char *isNamedForgivingSay( char *inSaidString );
char *isBabyNamingSay( char *inSaidString );
void nameBaby( LiveObject *inNamer, LiveObject *inBaby, char *inName, SimpleVector<int> *playerIndicesToSendNamesAbout );
char addKillState( LiveObject *inKiller, LiveObject *inTarget );
char directLineBlocked( GridPos inSource, GridPos inDest );
void setNoLongerDying( LiveObject *inPlayer, SimpleVector<int> *inPlayerIndicesToSendHealingAbout );
char isMapSpotEmptyOfPlayers( int inX, int inY );
FullMapContained getFullMapContained( int inX, int inY );
char isMapSpotEmpty( int inX, int inY, char inConsiderPlayers = true );
void executeKillAction( int inKillerIndex,
						int inTargetIndex,
						SimpleVector<int> *playerIndicesToSendUpdatesAbout,
						SimpleVector<int> *playerIndicesToSendDyingAbout,
						SimpleVector<int> *newEmotPlayerIDs,
						SimpleVector<int> *newEmotIndices,
						SimpleVector<int> *newEmotTTLs );
int getSecondsPlayed( LiveObject *inPlayer );
void handleShutdownDeath( LiveObject *inPlayer, int inX, int inY );
GridPos findClosestEmptyMapSpot( int inX, int inY, int inMaxPointsToCheck, char *outFound );
char findDropSpot( int inX, int inY, int inSourceX, int inSourceY, GridPos *outSpot );
LiveObject *getAdultHolding( LiveObject *inBabyObject );
void setPerpetratorHoldingAfterKill( LiveObject *nextPlayer, TransRecord *woundHit, TransRecord *rHit, TransRecord *r );
char isWildcardGivingSay( char *inSaidString, SimpleVector<char*> *inPhrases );
char *isFertilityDeclaringSay( char *inSaidString );
SimpleVector<MoveRecord> getMoveRecords(char inNewMovesOnly, SimpleVector<ChangePosition> *inChangeVector = NULL );
void getLineageLineForPlayer( LiveObject *inPlayer, SimpleVector<char> *inVector );
char *getMovesMessage( char inNewMovesOnly, GridPos inRelativeToPos, GridPos inLocalPos, SimpleVector<ChangePosition> *inChangeVector = NULL );
char *isPasswordSettingSay( char *inSaidString );//2HOL additions for: password-protected objects
char *isPasswordInvokingSay( char *inSaidString );//2HOL additions for: password-protected objects
char *isNamingSay( char *inSaidString, SimpleVector<char*> *inPhraseList );
char *isCurseNamingSay( char *inSaidString );
char *getMovesMessageFromList( SimpleVector<MoveRecord> *inMoves, GridPos inRelativeToPos );
MoveRecord getMoveRecord( LiveObject *inPlayer, char inNewMovesOnly, SimpleVector<ChangePosition> *inChangeVector = NULL );
char *isReverseNamingSay( char *inSaidString, SimpleVector<char*> *inPhraseList );
int getSayLimit( LiveObject *inPlayer );
void setContained( LiveObject *inPlayer, FullMapContained inContained );
void quitCleanup();

#endif //ONELIFE_SERVER_H
