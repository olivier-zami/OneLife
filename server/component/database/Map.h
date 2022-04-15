//
// Created by olivier on 14/04/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
#define ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H

#include "minorGems/util/SimpleVector.h"
#include "../../../gameSource/GridPos.h"
#include "../../../third_party/minorGems/system/Time.h"
#include "../../lineardb3.h"

#define NUM_RECENT_PLACEMENTS 100
#define BASE_MAP_CACHE_SIZE 256 // should be a power of 2 cache will contain squared number of records
#define BIOME_CACHE_SIZE 131072 // optimization:// cache biomeIndex results in RAM // 3.1 MB of RAM for this.
#define DB_CACHE_SIZE 131072// optimization:// cache dbGet results in RAM// 2.6 MB of RAM for this.
#define NUM_CONT_SLOT 2
#define FIRST_CONT_SLOT 3

#define KISSDB_OPEN_MODE_RWCREAT 3

#define DB LINEARDB3
#define DB_open LINEARDB3_open
#define DB_close LINEARDB3_close
#define DB_get LINEARDB3_get
#define DB_put LINEARDB3_put
#define DB_put_new LINEARDB3_put// no distinction between put and put_new in lineardb3
#define DB_Iterator LINEARDB3_Iterator
#define DB_Iterator_init LINEARDB3_Iterator_init
#define DB_Iterator_next LINEARDB3_Iterator_next
#define DB_maxStack db.maxOverflowDepth
#define DB_getShrinkSize LINEARDB3_getShrinkSize
#define DB_getCurrentSize LINEARDB3_getCurrentSize
#define DB_getNumRecords LINEARDB3_getNumRecords

typedef struct MapGridPlacement
{
	int               id;
	int               spacing;
	int               phase;
	int               wiggleScale;
	SimpleVector<int> permittedBiomes;
} MapGridPlacement;

typedef struct DBTimeCacheRecord
{
	int       x, y, slot, subCont;
	timeSec_t timeVal;
} DBTimeCacheRecord;

typedef struct BlockingCacheRecord
{
	int x, y;
	// -1 if not present
	char blocking;
} BlockingCacheRecord;

typedef struct BiomeCacheRecord
{
	int    x, y;
	int    biome, secondPlace;
	double secondPlaceGap;
} BiomeCacheRecord;

typedef struct DBCacheRecord
{
	int x, y, slot, subCont;
	int value;
} DBCacheRecord;

typedef struct RecentPlacement
{
	GridPos pos;
	int depth;// depth of object in tech tree
} RecentPlacement;

typedef struct BaseMapCacheRecord
{
	int  x, y;
	int  id;
	char gridPlacement;
} BaseMapCacheRecord;

namespace OneLife::server::database
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


#endif //ONELIFE_SERVER_COMPONENT_DATABASE_MAP_H
