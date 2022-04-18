//
// Created by olivier on 14/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
#define ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H

#include "../../third_party/minorGems/util/SimpleVector.h"
#include "../../gameSource/GridPos.h"
#include "../../third_party/minorGems/system/Time.h"
#include "database/LinearDB.h"

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

#endif //ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
