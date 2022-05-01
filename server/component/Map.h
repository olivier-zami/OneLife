//
// Created by olivier on 14/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
#define ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H

#include "../../gameSource/GridPos.h"
#include "../../gameSource/objectBank.h"
#include "../../gameSource/transitionBank.h"
#include "../../third_party/minorGems/util/SimpleVector.h"
#include "../../third_party/minorGems/system/Time.h"
#include "../dataType/info.h"
#include "../dataType/map.h"
#include "../dataType/Settings.h"
#include "database/Biome.h"
#include "database/LookTime.h"

#define MAP_METADATA_LENGTH 128
#define DECAY_SLOT 1
#define NO_DECAY_SLOT -1

typedef struct MapChangeRecord {
	char *formatString;
	int absoluteX, absoluteY;

	char oldCoordsUsed;
	int absoluteOldX, absoluteOldY;
} MapChangeRecord;

// track currently in-process movements so that we can be queried
// about whether arrival has happened or not
typedef struct MovementRecord
{
	int    x, y;
	double etaTime;
}MovementRecord;

typedef struct ContRecord
{
	int maxSlots;
	int maxSubSlots;
} ContRecord;

typedef struct LiveDecayRecord
{
	int x, y;

	// 0 means main object decay
	// 1 - NUM_CONT_SLOT means contained object decay
	int slot;

	timeSec_t etaTimeSeconds;

	// 0 means main object
	// >0 indexs sub containers of object
	int subCont;

	// the transition that will apply when this decay happens
	// this allows us to avoid marking certain types of move decays
	// as stale when not looked at in a while (all other types of decays
	// go stale)
	// Can be NULL if we don't care about the transition
	// associated with this decay (for contained item decay, for example)
	TransRecord *applicableTrans;

} LiveDecayRecord;

typedef struct MapGridPlacement
{
	int               id;
	int               spacing;
	int               phase;
	int               wiggleScale;
	SimpleVector<int> permittedBiomes;
} MapGridPlacement;

typedef struct BaseMapCacheRecord
{
	int  x, y;
	int  id;
	char gridPlacement;
} BaseMapCacheRecord;

namespace OneLife::server
{
	class Map
	{
		public:
			static void writeRegion(
				SimpleVector<unsigned char>* chunkDataBuffer,
				int inStartX,
				int inStartY,
				int inWidth,
				int inHeight);

			Map();
			~Map();

			void init(OneLife::server::settings::WorldMap settings);
			void updateBiomeRegion(OneLife::server::dataType::map::BiomeRegion* biomeRegion);

		private:
			static OneLife::server::Map *worldMap;
			OneLife::server::database::Biome* ldbBiome;
			//OneLife::server::bank::LinearDB* ldbLookTime;
			OneLife::server::database::LookTime* ldbLookTime;
	};
}
int DB_open_timeShrunk(
		LINEARDB3 *db,
		const char *path,
		int mode,
		unsigned long hash_table_size,
		unsigned long key_size,
		unsigned long value_size);
int getMapBiomeIndex(int inX, int inY, int *outSecondPlaceIndex = NULL, double *outSecondPlaceGap = NULL);
char loadIntoMapFromFile(FILE *inFile, int inOffsetX = 0, int inOffsetY = 0, double inTimeLimitSec = 0);
void clearRecentPlacements();
int cleanMap();
void initDBCaches();
void initBiomeCache();
void mapCacheClear();
int countNewlines(char *inString);
BaseMapCacheRecord *mapCacheRecordLookup(int inX, int inY);
int computeMapBiomeIndex(int inX, int inY, int *outSecondPlaceIndex = NULL, double *outSecondPlaceGap = NULL);
int getBiomeIndex(int inBiome);
void setMapObjectRaw(int inX, int inY, int inID);
timeSec_t dbLookTimeGet(int inX, int inY);
void deleteFileByName(const char *inFileName);
void setContained(int inX, int inY, int inNumContained, int *inContained, int inSubCont = 0);
void dbPut(int inX, int inY, int inSlot, int inValue, int inSubCont = 0);
int getMapObjectRaw( int inX, int inY );
int getPossibleBarrier(int inX, int inY);
int getTweakedBaseMap(int inX, int inY);
int getBaseMap(int inX, int inY, char *outGridPlacement = NULL);
int mapCacheLookup(int inX, int inY, char *outGridPlacement = NULL);
void mapCacheInsert(int inX, int inY, int inID, char inGridPlacement = false);
void reseedMap(char inForceFresh);
void setupMapChangeLogFile();
void setMapObject( int inX, int inY, int inID );
void logMapChange(int inX, int inY, int inID);
int findGridPos(SimpleVector<GridPos> *inList, GridPos inP);
int applyTapoutGradientRotate(int inX, int inY, int inTargetX, int inTargetY, int inEastwardGradientID);
char runTapoutOperation(int inX,
						int                            inY,
						int                            inRadiusX,
						int                            inRadiusY,
						int                            inSpacingX,
						int                            inSpacingY,
						int                            inTriggerID,
						char                           inPlayerHasPrimaryHomeland,
						char                           inIsPost = false);
void getEvePosition( const char *inEmail, int inID, int *outX, int *outY,
					 SimpleVector<GridPos> *inOtherPeoplePos,
					 char inAllowRespawn = true );
void changeContained( int inX, int inY, int inSlotNumber, int inNewObjectID);
void changeContained(int inX, int inY, int inSlot, int inSubCont, int inID);
void dbPut(int inX, int inY, int inSlot, int inValue, int inSubCont);
int getMapFloor( int inX, int inY );
int dbFloorGet(int inX, int inY);
char getSlotItemsNoDecay(int inX, int inY, int inSubCont);
int *getContainedRaw(int inX, int inY, int *outNumContained, int inSubCont = 0);
void setSlotItemsNoDecay(int inX, int inY, int inSubCont, char inNoDecay);
void checkDecayContained(int inX, int inY, int inSubCont = 0);
float getMapContainerTimeStretch(int inX, int inY, int inSubCont = 0);
void shrinkContainer(int inX, int inY, int inNumNewSlots, int inSubCont = 0);
void trackETA(int inX, int inY, int inSlot, timeSec_t inETA, int inSubCont = 0, TransRecord *inApplicableTrans = NULL);
int biomeGetCached(int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap);
void biomePutCached(int inX, int inY, int inBiome, int inSecondPlace, double inSecondPlaceGap);
void setEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds, TransRecord *inApplicableTrans = NULL );
void dbTimePut(int inX, int inY, int inSlot, timeSec_t inTime, int inSubCont = 0);
void dbTimePutCached(int inX, int inY, int inSlot, int inSubCont, timeSec_t inValue);
int getMapObject( int inX, int inY );
int checkDecayObject(int inX, int inY, int inID);
GridPos getClosestPlayerPos( int inX, int inY );
void clearAllContained(int inX, int inY, int inSubCont = 0);
timeSec_t *getContainedEtaDecay( int inX, int inY, int *outNumContained, int inSubCont = 0 );
int *getContained( int inX, int inY, int *outNumContained, int inSubCont = 0 );
int getContained( int inX, int inY, int inSlot, int inSubCont = 0 );
void setContainedEtaDecay( int inX, int inY, int inNumContained, timeSec_t *inContainedEtaDecay, int inSubCont = 0 );
int getContainerDecaySlot(int inX, int inY, int inSlot, int inSubCont = 0, int inNumContained = -1);
timeSec_t dbTimeGet(int inX, int inY, int inSlot, int inSubCont = 0);
int getContainerDecaySlot(int inX, int inY, int inSlot, int inSubCont, int inNumContained);
int dbTimeGetCached(int inX, int inY, int inSlot, int inSubCont);
timeSec_t getFloorEtaDecay( int inX, int inY );
void setFloorEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds );
void setMapFloor( int inX, int inY, int inID );
int getNumContained( int inX, int inY, int inSubCont = 0 );
timeSec_t getSlotEtaDecay( int inX, int inY, int inSlot, int inSubCont = 0 );
void setSlotEtaDecay( int inX, int inY, int inSlot, timeSec_t inAbsoluteTimeInSeconds, int inSubCont = 0 );
timeSec_t dbFloorTimeGet(int inX, int inY);
void dbFloorTimePut(int inX, int inY, timeSec_t inTime);
void dbFloorPut(int inX, int inY, int inValue);
void restretchMapContainedDecays(int inX, int inY, int inOldContainerID, int inNewContainerID, int inSubCont = 0);
void addContained( int inX, int inY, int inContainedID, timeSec_t inEtaDecay, int inSubCont = 0 );
timeSec_t getEtaDecay( int inX, int inY );
int getMapBiome( int inX, int inY );
void restretchDecays( int inNumDecays, timeSec_t *inDecayEtas, int inOldContainerID, int inNewContainerID );
void freeMap( char inSkipCleanup = false );
void stepMap( SimpleVector<MapChangeRecord> *inMapChanges, SimpleVector<ChangePosition> *inChangePosList );
int getNextDecayDelta();
void rememberDummy(FILE *inFile, int inX, int inY, ObjectRecord *inDummyO, int inSlot = -1, int inB = 0);
void lookAtRegion( int inXStart, int inYStart, int inXEnd, int inYEnd );
char isDecayTransAlwaysLiveTracked(TransRecord *inTrans);
void cleanMaxContainedHashTable(int inX, int inY);
void doubleEveRadius();
void resetEveRadius();
char getIsCategory(int inID);
MapChangeRecord getMapChangeRecord( ChangePosition inPos );
int getMapObjectNoLook(int inX, int inY);
int *getContainedNoLook(int inX, int inY, int *outNumContained, int inSubCont = 0);
void writeRecentPlacements();
void wipeMapFiles();
unsigned char *getChunkMessage(int inStartX, int inStartY, int inWidth, int inHeight, GridPos inRelativeToPos, int *outMessageLength);
GridPos getPlayerPos( LiveObject *inPlayer );
int dbGet(int inX, int inY, int inSlot, int inSubCont = 0);
int eveDBGet(const char *inEmail, int *outX, int *outY, int *outRadius);
char isMapSpotBlocking( int inX, int inY );

#endif //ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
