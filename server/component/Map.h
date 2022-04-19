//
// Created by olivier on 14/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
#define ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H

#include "../../third_party/minorGems/util/SimpleVector.h"
#include "../../gameSource/GridPos.h"
#include "../../third_party/minorGems/system/Time.h"
#include "database/LinearDB.h"

#define MAP_METADATA_LENGTH 128

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
		static bool init();
		static void writeRegion(
				SimpleVector<unsigned char>* chunkDataBuffer,
				int inStartX,
				int inStartY,
				int inWidth,
				int inHeight);
	};
}

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
int biomeDBGet(int inX, int inY, int *outSecondPlaceBiome = NULL, double *outSecondPlaceGap = NULL);
void setMapObjectRaw(int inX, int inY, int inID);
timeSec_t dbLookTimeGet(int inX, int inY);
int DB_open_timeShrunk(
		DB *db,
		const char *           path,
		int                    mode,
		unsigned long          hash_table_size,
		unsigned long          key_size,
		unsigned long          value_size);
void deleteFileByName(const char *inFileName);
void setContained(int inX, int inY, int inNumContained, int *inContained, int inSubCont);
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
void writeRecentPlacements();
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

#endif //ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
