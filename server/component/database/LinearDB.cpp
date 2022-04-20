//
// Created by olivier on 14/04/2022.
//

#include "LinearDB.h"

#include "../../dbCommon.h"
#include "../../lineardb3.h"
#include "../../monument.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"

extern DB lookTimeDB;
extern char lookTimeDBOpen;
extern DB   db;
extern char dbOpen;
extern DB   timeDB;
extern char timeDBOpen;
extern DB   biomeDB;
extern char biomeDBOpen;
extern DB   floorDB;
extern char floorDBOpen;
extern DB   floorTimeDB;
extern char floorTimeDBOpen;
extern DB   graveDB;
extern char graveDBOpen;
extern DB   eveDB;// per-player memory of where they should spawn as eve
extern char eveDBOpen;
extern DB   metaDB;
extern char metaDBOpen;

extern char skipTrackingMapChanges;

BlockingCacheRecord blockingCache[DB_CACHE_SIZE];
RecentPlacement recentPlacements[NUM_RECENT_PLACEMENTS];
BiomeCacheRecord biomeCache[BIOME_CACHE_SIZE];
DBTimeCacheRecord dbTimeCache[DB_CACHE_SIZE];
DBCacheRecord dbCache[DB_CACHE_SIZE];

int currentResponsiblePlayer = -1;

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 * returns -1 if not found
 */
int dbGet(int inX, int inY, int inSlot, int inSubCont)
{

	int cachedVal = dbGetCached(inX, inY, inSlot, inSubCont);
	if (cachedVal != -2) { return cachedVal; }

	unsigned char key[16];
	unsigned char value[4];

	// look for changes to default in database
	intQuadToKey(inX, inY, inSlot, inSubCont, key);

	int result = DB_get(&db, key, value);

	int returnVal;

	if (result == 0)
	{
		// found
		returnVal = valueToInt(value);
	}
	else
	{
		returnVal = -1;
	}

	dbPutCached(inX, inY, inSlot, inSubCont, returnVal);

	return returnVal;
}

void dbPutCached(int inX, int inY, int inSlot, int inSubCont, int inValue)
{
	DBCacheRecord r = {inX, inY, inSlot, inSubCont, inValue};

	dbCache[computeDBCacheHash(inX, inY, inSlot, inSubCont)] = r;
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 *  returns -2 on miss
 */
int dbGetCached(int inX, int inY, int inSlot, int inSubCont)
{
	DBCacheRecord r = dbCache[computeDBCacheHash(inX, inY, inSlot, inSubCont)];

	if (r.x == inX && r.y == inY && r.slot == inSlot && r.subCont == inSubCont && r.value != -2) { return r.value; }
	else
	{
		return -2;
	}
}

/**
 *
 * @param inKeyA
 * @param inKeyB
 * @param inKeyC
 * @param inKeyD
 * @return
 * @note from server/map.cpp
 */
int computeDBCacheHash(int inKeyA, int inKeyB, int inKeyC, int inKeyD)
{

	int hashKey = (inKeyA * CACHE_PRIME_A + inKeyB * CACHE_PRIME_B + inKeyC * CACHE_PRIME_C + inKeyD * CACHE_PRIME_D)
				  % DB_CACHE_SIZE;
	if (hashKey < 0) { hashKey += DB_CACHE_SIZE; }
	return hashKey;
}

/**
 * @note from server/map.cpp
 */
void blockingClearCached(int inX, int inY)
{

	BlockingCacheRecord *r = &(blockingCache[computeXYCacheHash(inX, inY)]);

	if (r->x == inX && r->y == inY) { r->blocking = -1; }
}

/**
 * @note from server/map.cpp
 */
int computeXYCacheHash(int inKeyA, int inKeyB)
{

	int hashKey = (inKeyA * CACHE_PRIME_A + inKeyB * CACHE_PRIME_B) % BIOME_CACHE_SIZE;
	if (hashKey < 0) { hashKey += BIOME_CACHE_SIZE; }
	return hashKey;
}

/**
 *
 * @param inEmail
 * @param outX
 * @param outY
 * @param outRadius
 * @return
 * @note returns -1 on failure, 1 on success
 */
int eveDBGet(const char *inEmail, int *outX, int *outY, int *outRadius)
{
	unsigned char key[50];

	unsigned char value[12];

	emailToKey(inEmail, key);

	int result = DB_get(&eveDB, key, value);

	if (result == 0)
	{
		// found
		*outX      = valueToInt(&(value[0]));
		*outY      = valueToInt(&(value[4]));
		*outRadius = valueToInt(&(value[8]));

		return 1;
	}
	else
	{
		return -1;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inTime
 * @note from server/map.cpp
 */
void dbLookTimePut(int inX, int inY, timeSec_t inTime)
{
	if (!lookTimeDBOpen) return;

	unsigned char key[8];
	unsigned char value[8];

	intPairToKey(inX / 100, inY / 100, key);
	timeToValue(inTime, value);

	DB_put(&lookTimeDB, key, value);
}
