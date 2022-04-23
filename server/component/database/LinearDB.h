//
// Created by olivier on 14/04/2022.
//

#ifndef INC_2HOL_LINEARDB_H
#define INC_2HOL_LINEARDB_H

#include "../../../server/lineardb3.h"
#include "../../../third_party/minorGems/system/Time.h"
#include "../../../gameSource/GridPos.h"
#include "../../component/feature/apocalypse.h"

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

#define NUM_RECENT_PLACEMENTS 100
#define BASE_MAP_CACHE_SIZE 256 // should be a power of 2 cache will contain squared number of records
#define BIOME_CACHE_SIZE 131072 // optimization:// cache biomeIndex results in RAM // 3.1 MB of RAM for this.
#define DB_CACHE_SIZE 131072// optimization:// cache dbGet results in RAM// 2.6 MB of RAM for this.
#define NUM_CONT_SLOT 2
#define FIRST_CONT_SLOT 3

#define KISSDB_OPEN_MODE_RWCREAT 3

#define CACHE_PRIME_A 776509273
#define CACHE_PRIME_B 904124281
#define CACHE_PRIME_C 528383237
#define CACHE_PRIME_D 148497157

typedef struct DBTimeCacheRecord
{
	int       x, y, slot, subCont;
	timeSec_t timeVal;
} DBTimeCacheRecord;

typedef struct BiomeCacheRecord
{
	int    x, y;
	int    biome, secondPlace;
	double secondPlaceGap;
} BiomeCacheRecord;

typedef struct BlockingCacheRecord
{
	int x, y;
	// -1 if not present
	char blocking;
} BlockingCacheRecord;

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

namespace OneLife::server::database
{
	class LinearDB
	{

	};
}



void dbLookTimePut(int inX, int inY, timeSec_t inTime);
int dbGet(int inX, int inY, int inSlot, int inSubCont = 0);
void dbPutCached(int inX, int inY, int inSlot, int inSubCont, int inValue);
int dbGetCached(int inX, int inY, int inSlot, int inSubCont);
int computeDBCacheHash(int inKeyA, int inKeyB, int inKeyC, int inKeyD);
void blockingClearCached(int inX, int inY);
int computeXYCacheHash(int inKeyA, int inKeyB);
int eveDBGet(const char *inEmail, int *outX, int *outY, int *outRadius);

#endif //INC_2HOL_LINEARDB_H
