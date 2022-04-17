//
// Created by olivier on 14/04/2022.
//

#include "LinearDB.h"

#include "../../dbCommon.h"
#include "../../lineardb3.h"

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

DBCacheRecord dbCache[DB_CACHE_SIZE];

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