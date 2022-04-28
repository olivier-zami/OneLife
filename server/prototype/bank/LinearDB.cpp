//
// Created by olivier on 14/04/2022.
//

#include "LinearDB.h"

#include "../../../third_party/minorGems/system/Time.h"
#include "../../../third_party/minorGems/util/log/AppLog.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../../commonSource/Debug.h"
#include "../../dbCommon.h"
#include "../../monument.h"
#include "linearDB/lineardb3.h"

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

OneLife::server::bank::LinearDB::LinearDB(
		const char* path,
		int mode,
		unsigned long hash_table_size,
		unsigned long key_size,
		unsigned long value_size)
{
	this->db = nullptr;
	this->dbi = nullptr;
	this->dbEmpty = nullptr;

	//!settings
	int strSize = strlen(path)+1;
	this->settings.path = (char*)malloc(strSize*sizeof(char));
	memset(this->settings.path, 0, strSize);
	strcpy(this->settings.path, path);
	this->settings.mode = mode;
	this->settings.hash_table_size = hash_table_size;
	this->settings.key_size = key_size;
	this->settings.value_size = value_size;

	//record
	this->rawRecord.key = nullptr;
	this->rawRecord.value = nullptr;

	this->dbFile = nullptr;
	if(!this->dbFile) this->dbFile = new File(NULL, this->settings.path);
	
	this->recordNumber = 0;
}

OneLife::server::bank::LinearDB::~LinearDB()
{
	if(this->settings.path)
	{
		free(this->settings.path);
		this->settings.path = nullptr;
	}
}

/**********************************************************************************************************************/

void OneLife::server::bank::LinearDB::open(DB* db, char* ptrEmptyStatus)
{
	if(!this->db)
	{
		if(!db) this->db = new DB;
		else this->db =db;
	}

	if(!this->dbEmpty)
	{
		if(!ptrEmptyStatus) this->dbEmpty = new char;
		else this->dbEmpty = ptrEmptyStatus;
	}

	int error = DB_open(
			db ? db : this->db,
			this->settings.path,
			this->settings.mode,
			this->settings.hash_table_size,
			this->settings.key_size,
			this->settings.value_size
	);

	if(error)
	{
		AppLog::errorF("Error %d opening look time %s", error, "LINEARDB3");
		//return false;
	}

	if(this->dbEmpty && !this->dbFile->exists()) *(this->dbEmpty) = true;
	this->recordNumber = 1;//TODO: iterate & count
}

void OneLife::server::bank::LinearDB::close()
{
	DB_close(this->db);
}

/**********************************************************************************************************************/

OneLife::server::bank::linearDB::Settings OneLife::server::bank::LinearDB::getSettings()
{
	return this->settings;
}

/**
 *
 * @return
 */
uint32_t OneLife::server::bank::LinearDB::getSize()
{
	return DB_getCurrentSize(this->db);
}

/**
 *
 * @return
 * @note replace lookTimeDBEmpty
 */
bool OneLife::server::bank::LinearDB::isEmpty()
{
	return !this->dbFile->exists() || !this->recordNumber;
}

/**
 *
 * @return
 */
bool OneLife::server::bank::LinearDB::isEnable()
{
	return this->dbFile->exists();
}

/**
 *
 * @param processRawRecord
 */
void OneLife::server::bank::LinearDB::iterate(std::function<void(RawRecord)> processRawRecord)
{
	if(!this->db) this->db = new DB();
	DB_Iterator dbi;
	DB_Iterator_init(this->db, &dbi);
	if(!this->rawRecord.key) this->rawRecord.key = new unsigned char[this->settings.key_size];
	if(!this->rawRecord.value) this->rawRecord.value = new unsigned char[this->settings.value_size];
	while(DB_Iterator_next(&dbi, this->rawRecord.key, this->rawRecord.value) > 0)
	{
		processRawRecord(this->rawRecord);
	}
}

/**********************************************************************************************************************/


void OneLife::server::bank::LinearDB::copy(OneLife::server::bank::LinearDB* srcDB)
{
	File srcFile(NULL, srcDB->getSettings().path);
	srcFile.copy(this->dbFile);
}

void OneLife::server::bank::LinearDB::insert(RawRecord rawRecord)
{
	DB_put_new(this->db, rawRecord.key, rawRecord.value);
}

/**********************************************************************************************************************/

/**
 *
 */
void OneLife::server::bank::LinearDB::clean() {}

/**
 *
 */
void OneLife::server::bank::LinearDB::removeDBFile()
{
	if(this->dbFile->exists())
	{
		AppLog::info("flushLookTimes.ini set, deleting lookTime.db");
		this->dbFile->remove();//TODO check if this->dbFile->exists() after remove because isEmpty()
	}
}

/**********************************************************************************************************************/


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
