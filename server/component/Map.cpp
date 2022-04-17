//
// Created by olivier on 14/04/2022.
//

#include "Map.h"

#include <float.h>

#include "../../third_party/minorGems/util/stringUtils.h"
#include "../../third_party/minorGems/util/SettingsManager.h"
#include "../../third_party/minorGems/util/log/AppLog.h"
#include "../../third_party/minorGems/io/file/File.h"
#include "../../third_party/minorGems/util/random/CustomRandomSource.h"
#include "../../gameSource/objectBank.h"
#include "../../gameSource/objectMetadata.h"
#include "../../commonSource/fractalNoise.h"
#include "../../commonSource/math/misc.h"
#include "../map.h"
#include "../dbCommon.h"

#include "database/LinearDB.h"

// can replace with frozenTime to freeze time
// or slowTime to slow it down
#define MAP_TIMESEC Time::timeSec()
#define CELL_D 128// cell pixel dimension on client

static int mapCacheBitMask = BASE_MAP_CACHE_SIZE - 1;// if BASE_MAP_CACHE_SIZE is a power of 2, then this is the bit mask of solid 1's that can limit an integer to that range
BaseMapCacheRecord baseMapCache[BASE_MAP_CACHE_SIZE][BASE_MAP_CACHE_SIZE];


#include "database/Map_speech.cpp" //TODO: refacto

// object ids that occur naturally on map at random, per biome
int numBiomes;
int* biomes;
float* biomeWeights;
float* biomeCumuWeights;
float biomeTotalWeight;
int regularBiomeLimit;
int numSpecialBiomes;
int* specialBiomes;
float* specialBiomeCumuWeights;
float specialBiomeTotalWeight;

int lastCheckedBiome  = -1;
int lastCheckedBiomeX = 0;
int lastCheckedBiomeY = 0;

float minEveCampRespawnAge = 120.0;
int edgeObjectID = 0;// what object is placed on edge of map
int barrierRadius = 250;
int barrierOn = 1;
int longTermCullEnabled = 1;
int nextPlacementIndex = 0;// ring buffer
int eveRadiusStart = 2;
int eveRadius = eveRadiusStart;
GridPos eveLocation = {0, 0};
char lookTimeDBEmpty = false;
char skipLookTimeCleanup = 0;
char skipRemovedObjectCleanup = 0;
char anyBiomesInDB = false;
int  maxBiomeXLoc  = -2000000000;
int  maxBiomeYLoc  = -2000000000;
int  minBiomeXLoc  = 2000000000;
int  minBiomeYLoc  = 2000000000;
double gapIntScale = 1000000.0;
char skipTrackingMapChanges = false;// for large inserts, like tutorial map loads, we don't want to// track individual map changes.
int evePrimaryLocSpacing  = 0;
int evePrimaryLocObjectID = -1;
int eveHomeMarkerObjectID = -1;
int cellsLookedAtToInit = 0;// if lookTimeDBEmpty, then we init all map cell look times to NOW
float *totalChanceWeight;
int randSeed = 124567;
//JenkinsRandomSource randSource( randSeed );
CustomRandomSource randSource(randSeed);
int getBaseMapCallCount = 0;

SimpleVector<int> 		eveSecondaryLocObjectIDs;
SimpleVector<GridPos> 	recentlyUsedPrimaryEvePositions;
SimpleVector<double> 	recentlyUsedPrimaryEvePositionTimes;
SimpleVector<int>     	recentlyUsedPrimaryEvePositionPlayerIDs;
SimpleVector<int> 		barrierItemList;
SimpleVector<GlobalTriggerState> globalTriggerStates;
SimpleVector<MapGridPlacement> gridPlacements;
SimpleVector<int> allNaturalMapIDs;
SimpleVector<int> *  naturalMapIDs;// one vector per biome
SimpleVector<float> *naturalMapChances;

BlockingCacheRecord blockingCache[DB_CACHE_SIZE];
RecentPlacement recentPlacements[NUM_RECENT_PLACEMENTS];
BiomeCacheRecord biomeCache[BIOME_CACHE_SIZE];
DBTimeCacheRecord dbTimeCache[DB_CACHE_SIZE];

DB db;
char dbOpen = false;
DB biomeDB;
char biomeDBOpen = false;
DB lookTimeDB;// tracking when a given map cell was last seen
char lookTimeDBOpen = false;
DB timeDB;
char timeDBOpen = false;
DB floorDB;
char floorDBOpen = false;
DB floorTimeDB;
char floorTimeDBOpen = false;
DB   graveDB;
char graveDBOpen = false;
DB   eveDB;// per-player memory of where they should spawn as eve
char eveDBOpen = false;
DB   metaDB;
char metaDBOpen = false;

extern DBCacheRecord dbCache[];

// 1671 shy of int max
static int xLimit = 2147481977;
static int yLimit = 2147481977;
static char useTestMap = false;// if true, rest of natural map is blank

static inline void changeContained(int inX, int inY, int inSlot, int inSubCont, int inID)
{
	dbPut(inX, inY, FIRST_CONT_SLOT + inSlot, inID, inSubCont);
}

/**
 *
 * @return
 * @note from initMap in map.cpp
 * returns true on success
 */
bool OneLife::server::Map::init()
{

	reseedMap(false);

	numSpeechPipes = getMaxSpeechPipeIndex() + 1;

	speechPipesIn  = new SimpleVector<GridPos>[numSpeechPipes];
	speechPipesOut = new SimpleVector<GridPos>[numSpeechPipes];

	eveSecondaryLocObjectIDs.deleteAll();
	recentlyUsedPrimaryEvePositionTimes.deleteAll();
	recentlyUsedPrimaryEvePositions.deleteAll();
	recentlyUsedPrimaryEvePositionPlayerIDs.deleteAll();

	initDBCaches();
	initBiomeCache();

	mapCacheClear();

	edgeObjectID = SettingsManager::getIntSetting("edgeObject", 0);

	minEveCampRespawnAge = SettingsManager::getFloatSetting("minEveCampRespawnAge", 120.0f);

	barrierRadius = SettingsManager::getIntSetting("barrierRadius", 250);
	barrierOn     = SettingsManager::getIntSetting("barrierOn", 1);

	longTermCullEnabled = SettingsManager::getIntSetting("longTermNoLookCullEnabled", 1);

	SimpleVector<int> *list = SettingsManager::getIntSettingMulti("barrierObjects");

	barrierItemList.deleteAll();
	barrierItemList.push_back_other(list);
	delete list;

	for (int i = 0; i < NUM_RECENT_PLACEMENTS; i++)
	{
		recentPlacements[i].pos.x = 0;
		recentPlacements[i].pos.y = 0;
		recentPlacements[i].depth = 0;
	}

	nextPlacementIndex = 0;

	FILE *placeFile = fopen("recentPlacements.txt", "r");
	if (placeFile != NULL)
	{
		for (int i = 0; i < NUM_RECENT_PLACEMENTS; i++)
		{
			fscanf(placeFile,
				   "%d,%d %d",
				   &(recentPlacements[i].pos.x),
				   &(recentPlacements[i].pos.y),
				   &(recentPlacements[i].depth));
		}
		fscanf(placeFile, "\nnextPlacementIndex=%d", &nextPlacementIndex);

		fclose(placeFile);
	}

	FILE *eveRadFile = fopen("eveRadius.txt", "r");
	if (eveRadFile != NULL)
	{

		fscanf(eveRadFile, "%d", &eveRadius);

		fclose(eveRadFile);
	}

	FILE *eveLocFile = fopen("lastEveLocation.txt", "r");
	if (eveLocFile != NULL)
	{

		fscanf(eveLocFile, "%d,%d", &(eveLocation.x), &(eveLocation.y));

		fclose(eveLocFile);

		printf("Loading lastEveLocation %d,%d\n", eveLocation.x, eveLocation.y);
	}

	// override if shutdownLongLineagePos exists
	FILE *lineagePosFile = fopen("shutdownLongLineagePos.txt", "r");
	if (lineagePosFile != NULL)
	{

		fscanf(lineagePosFile, "%d,%d", &(eveLocation.x), &(eveLocation.y));

		fclose(lineagePosFile);

		printf("Overriding eveLocation with shutdownLongLineagePos %d,%d\n", eveLocation.x, eveLocation.y);
	}
	else
	{
		printf("No shutdownLongLineagePos.txt file exists\n");

		// look for longest monument log file
		// that has been touched in last 24 hours
		// (ignore spots that may have been culled)
		File f(NULL, "monumentLogs");
		if (f.exists() && f.isDirectory())
		{
			int    numChildFiles;
			File **childFiles = f.getChildFiles(&numChildFiles);

			timeSec_t longTime = 0;
			int       longLen  = 0;
			int       longX    = 0;
			int       longY    = 0;

			timeSec_t curTime = Time::timeSec();

			int secInDay = 3600 * 24;

			for (int i = 0; i < numChildFiles; i++)
			{
				timeSec_t modTime = childFiles[i]->getModificationTime();

				if (curTime - modTime < secInDay)
				{
					char *cont = childFiles[i]->readFileContents();

					int numNewlines = countNewlines(cont);

					delete[] cont;

					if (numNewlines > longLen || (numNewlines == longLen && modTime > longTime))
					{

						char *name = childFiles[i]->getFileName();

						int x, y;
						int numRead = sscanf(name, "%d_%d_", &x, &y);

						delete[] name;

						if (numRead == 2)
						{
							longTime = modTime;
							longLen  = numNewlines;
							longX    = x;
							longY    = y;
						}
					}
				}
				delete childFiles[i];
			}
			delete[] childFiles;

			if (longLen > 0)
			{
				eveLocation.x = longX;
				eveLocation.y = longY;

				printf("Overriding eveLocation with "
					   "tallest recent monument location %d,%d\n",
					   eveLocation.x,
					   eveLocation.y);
			}
		}
	}

	const char *lookTimeDBName = "lookTime.db";

	char lookTimeDBExists = false;

	File lookTimeDBFile(NULL, lookTimeDBName);

	if (lookTimeDBFile.exists() && SettingsManager::getIntSetting("flushLookTimes", 0))
	{

		AppLog::info("flushLookTimes.ini set, deleting lookTime.db");

		lookTimeDBFile.remove();
	}

	lookTimeDBExists = lookTimeDBFile.exists();

	if (!lookTimeDBExists) { lookTimeDBEmpty = true; }

	skipLookTimeCleanup = SettingsManager::getIntSetting("skipLookTimeCleanup", 0);

	if (skipLookTimeCleanup)
	{
		AppLog::info("skipLookTimeCleanup.ini flag set, "
					 "not cleaning databases based on stale look times.");
	}

	LINEARDB3_setMaxLoad(0.80);

	if (!skipLookTimeCleanup)
	{
		DB lookTimeDB_old;

		int error = DB_open(&lookTimeDB_old,
							lookTimeDBName,
							KISSDB_OPEN_MODE_RWCREAT,
							80000,
							8, // two 32-bit ints, xy
							8  // one 64-bit double, representing an ETA time
				// in whatever binary format and byte order
				// "double" on the server platform uses
		);

		if (error)
		{
			AppLog::errorF("Error %d opening look time KissDB", error);
			return false;
		}

		int staleSec = SettingsManager::getIntSetting("mapCellForgottenSeconds", 0);

		if (lookTimeDBExists && staleSec > 0)
		{
			AppLog::info("\nCleaning stale look times from map...");

			static DB lookTimeDB_temp;

			const char *lookTimeDBName_temp = "lookTime_temp.db";

			File tempDBFile(NULL, lookTimeDBName_temp);

			if (tempDBFile.exists()) { tempDBFile.remove(); }

			DB_Iterator dbi;

			DB_Iterator_init(&lookTimeDB_old, &dbi);

			timeSec_t curTime = MAP_TIMESEC;

			unsigned char key[8];
			unsigned char value[8];

			int total    = 0;
			int stale    = 0;
			int nonStale = 0;

			// first, just count them
			while (DB_Iterator_next(&dbi, key, value) > 0)
			{
				total++;

				timeSec_t t = valueToTime(value);

				if (curTime - t >= staleSec)
				{
					// stale cell
					// ignore
					stale++;
				}
				else
				{
					// non-stale
					nonStale++;
				}
			}

			// optimial size for DB of remaining elements
			unsigned int newSize = DB_getShrinkSize(&lookTimeDB_old, nonStale);

			AppLog::infoF("Shrinking hash table for lookTimes from "
						  "%d down to %d",
						  DB_getCurrentSize(&lookTimeDB_old),
						  newSize);

			error = DB_open(&lookTimeDB_temp,
							lookTimeDBName_temp,
							KISSDB_OPEN_MODE_RWCREAT,
							newSize,
							8, // two 32-bit ints, xy
							8  // one 64-bit double, representing an ETA time
					// in whatever binary format and byte order
					// "double" on the server platform uses
			);

			if (error)
			{
				AppLog::errorF("Error %d opening look time temp KissDB", error);
				return false;
			}

			// now that we have new temp db properly sized,
			// iterate again and insert
			DB_Iterator_init(&lookTimeDB_old, &dbi);

			while (DB_Iterator_next(&dbi, key, value) > 0)
			{
				timeSec_t t = valueToTime(value);

				if (curTime - t >= staleSec)
				{
					// stale cell
					// ignore
				}
				else
				{
					// non-stale
					// insert it in temp
					DB_put_new(&lookTimeDB_temp, key, value);
				}
			}

			AppLog::infoF("Cleaned %d / %d stale look times", stale, total);

			printf("\n");

			if (total == 0) { lookTimeDBEmpty = true; }

			DB_close(&lookTimeDB_temp);
			DB_close(&lookTimeDB_old);

			tempDBFile.copy(&lookTimeDBFile);
			tempDBFile.remove();
		}
		else
		{
			DB_close(&lookTimeDB_old);
		}
	}

	int error = DB_open(&lookTimeDB,
						lookTimeDBName,
						KISSDB_OPEN_MODE_RWCREAT,
						80000,
						8, // two 32-bit ints, xy
						8  // one 64-bit double, representing an ETA time
			// in whatever binary format and byte order
			// "double" on the server platform uses
	);

	if (error)
	{
		AppLog::errorF("Error %d opening look time KissDB", error);
		return false;
	}

	lookTimeDBOpen = true;

	// note that the various decay ETA slots in map.db
	// are define but unused, because we store times separately
	// in mapTime.db
	error = DB_open_timeShrunk(&db,
							   "map.db",
							   KISSDB_OPEN_MODE_RWCREAT,
							   80000,
							   16, // four 32-bit ints, xysb
			// s is the slot number
			// s=0 for base object
			// s=1 decay ETA seconds (wall clock time)
			// s=2 for count of contained objects
			// s=3 first contained object
			// s=4 second contained object
			// s=... remaining contained objects
			// Then decay ETA for each slot, in order,
			//   after that.
			// s = -1
			//  is a special flag slot set to 0 if NONE
			//  of the contained items have ETA decay
			//  or 1 if some of the contained items might
			//  have ETA decay.
			//  (this saves us from having to check each
			//   one)
			// If a contained object id is negative,
			// that indicates that it sub-contains
			// other objects in its corresponding b slot
			//
			// b is for indexing sub-container slots
			// b=0 is the main object
			// b=1 is the first sub-slot, etc.
							   4 // one int, object ID at x,y in slot (s-3)
			// OR contained count if s=2
	);

	if (error)
	{
		AppLog::errorF("Error %d opening map KissDB", error);
		return false;
	}

	dbOpen = true;

	// this DB uses the same slot numbers as the map.db
	// however, only times are stored here, because they require 8 bytes
	// so, slot 0 and 2 are never used, for example
	error = DB_open_timeShrunk(&timeDB,
							   "mapTime.db",
							   KISSDB_OPEN_MODE_RWCREAT,
							   80000,
							   16, // four 32-bit ints, xysb
			// s is the slot number
			// s=0 for base object
			// s=1 decay ETA seconds (wall clock time)
			// s=2 for count of contained objects
			// s=3 first contained object
			// s=4 second contained object
			// s=... remaining contained objects
			// Then decay ETA for each slot, in order,
			//   after that.
			// If a contained object id is negative,
			// that indicates that it sub-contains
			// other objects in its corresponding b slot
			//
			// b is for indexing sub-container slots
			// b=0 is the main object
			// b=1 is the first sub-slot, etc.
							   8 // one 64-bit double, representing an ETA time
			// in whatever binary format and byte order
			// "double" on the server platform uses
	);

	if (error)
	{
		AppLog::errorF("Error %d opening map time KissDB", error);
		return false;
	}

	timeDBOpen = true;

	error = DB_open_timeShrunk(&biomeDB,
							   "biome.db",
							   KISSDB_OPEN_MODE_RWCREAT,
							   80000,
							   8, // two 32-bit ints, xy
							   12 // three ints,
			// 1: biome number at x,y
			// 2: second place biome number at x,y
			// 3: second place biome gap as int (float gap
			//    multiplied by 1,000,000)
	);

	if (error)
	{
		AppLog::errorF("Error %d opening biome KissDB", error);
		return false;
	}

	biomeDBOpen = true;

	// see if any biomes are listed in DB
	// if not, we don't even need to check it when generating map
	DB_Iterator biomeDBi;
	DB_Iterator_init(&biomeDB, &biomeDBi);

	unsigned char biomeKey[8];
	unsigned char biomeValue[12];

	while (DB_Iterator_next(&biomeDBi, biomeKey, biomeValue) > 0)
	{
		int x = valueToInt(biomeKey);
		int y = valueToInt(&(biomeKey[4]));

		anyBiomesInDB = true;

		if (x > maxBiomeXLoc) { maxBiomeXLoc = x; }
		if (x < minBiomeXLoc) { minBiomeXLoc = x; }
		if (y > maxBiomeYLoc) { maxBiomeYLoc = y; }
		if (y < minBiomeYLoc) { minBiomeYLoc = y; }
	}

	printf("Min (x,y) of biome in db = (%d,%d), "
		   "Max (x,y) of biome in db = (%d,%d)\n",
		   minBiomeXLoc,
		   minBiomeYLoc,
		   maxBiomeXLoc,
		   maxBiomeYLoc);

	error = DB_open_timeShrunk(&floorDB,
							   "floor.db",
							   KISSDB_OPEN_MODE_RWCREAT,
							   80000,
							   8, // two 32-bit ints, xy
							   4  // one int, the floor object ID at x,y
	);

	if (error)
	{
		AppLog::errorF("Error %d opening floor KissDB", error);
		return false;
	}

	floorDBOpen = true;

	error = DB_open_timeShrunk(&floorTimeDB,
							   "floorTime.db",
							   KISSDB_OPEN_MODE_RWCREAT,
							   80000,
							   8, // two 32-bit ints, xy
							   8  // one 64-bit double, representing an ETA time
			// in whatever binary format and byte order
			// "double" on the server platform uses
	);

	if (error)
	{
		AppLog::errorF("Error %d opening floor time KissDB", error);
		return false;
	}

	floorTimeDBOpen = true;

	// ALWAYS delete old grave DB at each server startup
	// grave info is only player ID, and server only remembers players
	// live, in RAM, while it is still running
	deleteFileByName("grave.db");

	error = DB_open(&graveDB,
					"grave.db",
					KISSDB_OPEN_MODE_RWCREAT,
					80000,
					8, // two 32-bit ints, xy
					4  // one int, the grave player ID at x,y
	);

	if (error)
	{
		AppLog::errorF("Error %d opening grave KissDB", error);
		return false;
	}

	graveDBOpen = true;

	error = DB_open(&eveDB,
					"eve.db",
					KISSDB_OPEN_MODE_RWCREAT,
			// this can be a lot smaller than other DBs
			// it's not performance-critical, and the keys are
			// much longer, so stackdb will waste disk space
					5000,
					50, // first 50 characters of email address
			// append spaces to the end if needed
					12 // three ints,  x_center, y_center, radius
	);

	if (error)
	{
		AppLog::errorF("Error %d opening eve KissDB", error);
		return false;
	}

	eveDBOpen = true;

	error = DB_open(&metaDB,
					"meta.db",
					KISSDB_OPEN_MODE_RWCREAT,
			// starting size doesn't matter here
					500,
					4, // one 32-bit int as key
			// data
					MAP_METADATA_LENGTH);

	if (error)
	{
		AppLog::errorF("Error %d opening meta KissDB", error);
		return false;
	}

	metaDBOpen = true;

	DB_Iterator metaIterator;

	DB_Iterator_init(&metaDB, &metaIterator);

	unsigned char metaKey[4];

	unsigned char metaValue[MAP_METADATA_LENGTH];

	int maxMetaID      = 0;
	int numMetaRecords = 0;

	while (DB_Iterator_next(&metaIterator, metaKey, metaValue) > 0)
	{
		numMetaRecords++;

		int metaID = valueToInt(metaKey);

		if (metaID > maxMetaID) { maxMetaID = metaID; }
	}

	AppLog::infoF("MetadataDB:  Found %d records with max MetadataID of %d", numMetaRecords, maxMetaID);

	setLastMetadataID(maxMetaID);

	if (lookTimeDBEmpty && cellsLookedAtToInit > 0)
	{
		printf("Since lookTime db was empty, we initialized look times "
			   "for %d cells to now.\n\n",
			   cellsLookedAtToInit);
	}

	int            numObjects;
	ObjectRecord **allObjects = getAllObjects(&numObjects);

	// first, find all biomes
	SimpleVector<int> biomeList;

	for (int i = 0; i < numObjects; i++)
	{
		ObjectRecord *o = allObjects[i];

		if (o->mapChance > 0)
		{

			for (int j = 0; j < o->numBiomes; j++)
			{
				int b = o->biomes[j];

				if (biomeList.getElementIndex(b) == -1) { biomeList.push_back(b); }
			}
		}
	}

	// manually controll order
	SimpleVector<int> *biomeOrderList = SettingsManager::getIntSettingMulti("biomeOrder");

	SimpleVector<float> *biomeWeightList = SettingsManager::getFloatSettingMulti("biomeWeights");

	for (int i = 0; i < biomeOrderList->size(); i++)
	{
		int b = biomeOrderList->getElementDirect(i);

		if (biomeList.getElementIndex(b) == -1)
		{
			biomeOrderList->deleteElement(i);
			biomeWeightList->deleteElement(i);
			i--;
		}
	}

	// now add any discovered biomes to end of list
	for (int i = 0; i < biomeList.size(); i++)
	{
		int b = biomeList.getElementDirect(i);
		if (biomeOrderList->getElementIndex(b) == -1)
		{
			biomeOrderList->push_back(b);
			// default weight
			biomeWeightList->push_back(0.1);
		}
	}

	numBiomes        = biomeOrderList->size();
	biomes           = biomeOrderList->getElementArray();
	biomeWeights     = biomeWeightList->getElementArray();
	biomeCumuWeights = new float[numBiomes];

	biomeTotalWeight = 0;
	for (int i = 0; i < numBiomes; i++)
	{
		biomeTotalWeight += biomeWeights[i];
		biomeCumuWeights[i] = biomeTotalWeight;
	}

	delete biomeOrderList;
	delete biomeWeightList;

	SimpleVector<int> *specialBiomeList = SettingsManager::getIntSettingMulti("specialBiomes");

	numSpecialBiomes = specialBiomeList->size();
	specialBiomes    = specialBiomeList->getElementArray();

	regularBiomeLimit = numBiomes - numSpecialBiomes;

	delete specialBiomeList;

	specialBiomeCumuWeights = new float[numSpecialBiomes];

	specialBiomeTotalWeight = 0;
	for (int i = regularBiomeLimit; i < numBiomes; i++)
	{
		specialBiomeTotalWeight += biomeWeights[i];
		specialBiomeCumuWeights[i - regularBiomeLimit] = specialBiomeTotalWeight;
	}

	naturalMapIDs     = new SimpleVector<int>[numBiomes];
	naturalMapChances = new SimpleVector<float>[numBiomes];
	totalChanceWeight = new float[numBiomes];

	for (int j = 0; j < numBiomes; j++)
	{
		totalChanceWeight[j] = 0;
	}

	CustomRandomSource phaseRandSource(randSeed);

	for (int i = 0; i < numObjects; i++)
	{
		ObjectRecord *o = allObjects[i];

		if (strstr(o->description, "eveSecondaryLoc") != NULL) { eveSecondaryLocObjectIDs.push_back(o->id); }
		if (strstr(o->description, "eveHomeMarker") != NULL) { eveHomeMarkerObjectID = o->id; }

		float p = o->mapChance;
		if (p > 0)
		{
			int id = o->id;

			allNaturalMapIDs.push_back(id);

			char *gridPlacementLoc = strstr(o->description, "gridPlacement");

			if (gridPlacementLoc != NULL)
			{
				// special grid placement

				int spacing = 10;
				sscanf(gridPlacementLoc, "gridPlacement%d", &spacing);

				if (strstr(o->description, "evePrimaryLoc") != NULL)
				{
					evePrimaryLocObjectID = id;
					evePrimaryLocSpacing  = spacing;
				}

				SimpleVector<int> permittedBiomes;
				for (int b = 0; b < o->numBiomes; b++)
				{
					permittedBiomes.push_back(getBiomeIndex(o->biomes[b]));
				}

				int wiggleScale = 4;

				if (spacing > 12) { wiggleScale = spacing / 3; }

				MapGridPlacement gp = {id,
									   spacing,
									   0,
						// phaseRandSource.getRandomBoundedInt( 0,
						//                                     spacing - 1 ),
									   wiggleScale,
									   permittedBiomes};

				gridPlacements.push_back(gp);
			}
			else
			{
				// regular fractal placement

				for (int j = 0; j < o->numBiomes; j++)
				{
					int b = o->biomes[j];

					int bIndex = getBiomeIndex(b);
					naturalMapIDs[bIndex].push_back(id);
					naturalMapChances[bIndex].push_back(p);

					totalChanceWeight[bIndex] += p;
				}
			}
		}
	}

	for (int j = 0; j < numBiomes; j++)
	{
		AppLog::infoF("Biome %d:  Found %d natural objects with total weight %f",
					  biomes[j],
					  naturalMapIDs[j].size(),
					  totalChanceWeight[j]);
	}

	delete[] allObjects;

	skipRemovedObjectCleanup = SettingsManager::getIntSetting("skipRemovedObjectCleanup", 0);

	FILE *dummyFile = fopen("mapDummyRecall.txt", "r");

	if (dummyFile != NULL)
	{
		AppLog::info("Found mapDummyRecall.txt file, restoring dummy objects "
					 "on map");

		skipTrackingMapChanges = true;

		int numRead = 5;

		int numSet = 0;

		int numStale = 0;

		while (numRead == 5 || numRead == 7)
		{

			int x, y, parentID, dummyIndex, slot, b;

			char marker;

			slot = -1;
			b    = 0;

			numRead =
					fscanf(dummyFile, "(%d,%d) %c %d %d [%d %d]\n", &x, &y, &marker, &parentID, &dummyIndex, &slot, &b);
			if (numRead == 5 || numRead == 7)
			{

				if (dbLookTimeGet(x, y) <= 0)
				{
					// stale area of map
					numStale++;
					continue;
				}

				ObjectRecord *parent = getObject(parentID);

				int dummyID = -1;

				if (parent != NULL)
				{

					if (marker == 'u' && parent->numUses - 1 > dummyIndex)
					{ dummyID = parent->useDummyIDs[dummyIndex]; }
					else if (marker == 'v' && parent->numVariableDummyIDs > dummyIndex)
					{
						dummyID = parent->variableDummyIDs[dummyIndex];
					}
				}
				if (dummyID > 0)
				{
					if (numRead == 5) { setMapObjectRaw(x, y, dummyID); }
					else
					{
						changeContained(x, y, slot, b, dummyID);
					}
					numSet++;
				}
			}
		}
		skipTrackingMapChanges = false;

		fclose(dummyFile);

		AppLog::infoF("Restored %d dummy objects to map "
					  "(%d skipped as stale)",
					  numSet,
					  numStale);

		remove("mapDummyRecall.txt");

		printf("\n");
	}

	// clean map after restoring dummy objects
	int totalSetCount = 1;

	if (!skipRemovedObjectCleanup) { totalSetCount = cleanMap(); }
	else
	{
		AppLog::info("Skipping cleaning map of removed objects");
	}

	if (totalSetCount == 0)
	{
		// map has been cleared

		// ignore old value for placements
		clearRecentPlacements();
	}

	globalTriggerStates.deleteAll();

	int numTriggers = getNumGlobalTriggers();
	for (int i = 0; i < numTriggers; i++)
	{
		GlobalTriggerState s;
		globalTriggerStates.push_back(s);
	}

	useTestMap = SettingsManager::getIntSetting("useTestMap", 0);

	if (useTestMap)
	{

		FILE *testMapFile      = fopen("testMap.txt", "r");
		FILE *testMapStaleFile = fopen("testMapStale.txt", "r");

		if (testMapFile != NULL && testMapStaleFile == NULL)
		{

			testMapStaleFile = fopen("testMapStale.txt", "w");

			if (testMapStaleFile != NULL)
			{
				fprintf(testMapStaleFile, "1");
				fclose(testMapStaleFile);
				testMapStaleFile = NULL;
			}

			printf("Loading testMap.txt\n");

			loadIntoMapFromFile(testMapFile);

			fclose(testMapFile);
			testMapFile = NULL;
		}

		if (testMapFile != NULL) { fclose(testMapFile); }
		if (testMapStaleFile != NULL) { fclose(testMapStaleFile); }
	}

	SimpleVector<char *> *specialPlacements = SettingsManager::getSetting("specialMapPlacements");

	if (specialPlacements != NULL)
	{

		for (int i = 0; i < specialPlacements->size(); i++)
		{
			char *line = specialPlacements->getElementDirect(i);

			int x, y, id;
			id          = -1;
			int numRead = sscanf(line, "%d_%d_%d", &x, &y, &id);

			if (numRead == 3 && id > -1) {}
			setMapObject(x, y, id);
		}

		specialPlacements->deallocateStringElements();
		delete specialPlacements;
	}

	// for debugging the map
	// printObjectSamples();
	// printBiomeSamples();
	// outputMapImage();

	// outputBiomeFractals();

	setupMapChangeLogFile();

	return true;
}


/**
 *
 * @param chunkDataBuffer
 * @param inStartX
 * @param inStartY
 * @param inWidth
 * @param inHeight
 * @note from getChunkMessage(...) in map.cpp
 */
void OneLife::server::Map::writeRegion(SimpleVector<unsigned char> *chunkDataBuffer,
	 int inStartX,
	 int inStartY,
	 int inWidth,
	 int inHeight)
{
	int chunkCells = inWidth * inHeight;

	int *chunk = new int[chunkCells];

	int *chunkBiomes = new int[chunkCells];
	int *chunkFloors = new int[chunkCells];

	int * containedStackSizes = new int[chunkCells];
	int **containedStacks     = new int *[chunkCells];

	int ** subContainedStackSizes = new int *[chunkCells];
	int ***subContainedStacks     = new int **[chunkCells];

	int endY = inStartY + inHeight;
	int endX = inStartX + inWidth;

	timeSec_t curTime = MAP_TIMESEC;

	// look at four corners of chunk whenever we fetch one
	dbLookTimePut(inStartX, inStartY, curTime);
	dbLookTimePut(inStartX, endY, curTime);
	dbLookTimePut(endX, inStartY, curTime);
	dbLookTimePut(endX, endY, curTime);

	for (int y = inStartY; y < endY; y++)
	{
		int chunkY = y - inStartY;

		for (int x = inStartX; x < endX; x++)
		{
			int chunkX = x - inStartX;

			int cI = chunkY * inWidth + chunkX;

			lastCheckedBiome = -1;

			chunk[cI] = getMapObject(x, y);

			if (lastCheckedBiome == -1 || lastCheckedBiomeX != x || lastCheckedBiomeY != y)
			{
				// biome wasn't checked in order to compute
				// getMapObject

				// get it ourselves

				lastCheckedBiome = biomes[getMapBiomeIndex(x, y)];
			}
			chunkBiomes[cI] = lastCheckedBiome;

			chunkFloors[cI] = getMapFloor(x, y);

			int  numContained;
			int *contained = NULL;

			if (chunk[cI] > 0 && getObject(chunk[cI])->numSlots > 0) { contained = getContained(x, y, &numContained); }

			if (contained != NULL)
			{
				containedStackSizes[cI] = numContained;
				containedStacks[cI]     = contained;

				subContainedStackSizes[cI] = new int[numContained];
				subContainedStacks[cI]     = new int *[numContained];

				for (int i = 0; i < numContained; i++)
				{
					subContainedStackSizes[cI][i] = 0;
					subContainedStacks[cI][i]     = NULL;

					if (containedStacks[cI][i] < 0)
					{
						// a sub container
						containedStacks[cI][i] *= -1;

						int  numSubContained;
						int *subContained = getContained(x, y, &numSubContained, i + 1);
						if (subContained != NULL)
						{
							subContainedStackSizes[cI][i] = numSubContained;
							subContainedStacks[cI][i]     = subContained;
						}
					}
				}
			}
			else
			{
				containedStackSizes[cI]    = 0;
				containedStacks[cI]        = NULL;
				subContainedStackSizes[cI] = NULL;
				subContainedStacks[cI]     = NULL;
			}
		}
	}

	for (int i = 0; i < chunkCells; i++)
	{

		if (i > 0) { chunkDataBuffer->appendArray((unsigned char *)" ", 1); }

		char *cell =
				autoSprintf("%d:%d:%d", chunkBiomes[i], hideIDForClient(chunkFloors[i]), hideIDForClient(chunk[i]));

		chunkDataBuffer->appendArray((unsigned char *)cell, strlen(cell));
		delete[] cell;

		if (containedStacks[i] != NULL)
		{
			for (int c = 0; c < containedStackSizes[i]; c++)
			{
				char *containedString = autoSprintf(",%d", hideIDForClient(containedStacks[i][c]));

				chunkDataBuffer->appendArray((unsigned char *)containedString, strlen(containedString));
				delete[] containedString;

				if (subContainedStacks[i][c] != NULL)
				{

					for (int s = 0; s < subContainedStackSizes[i][c]; s++)
					{

						char *subContainedString = autoSprintf(":%d", hideIDForClient(subContainedStacks[i][c][s]));

						chunkDataBuffer->appendArray((unsigned char *)subContainedString, strlen(subContainedString));
						delete[] subContainedString;
					}
					delete[] subContainedStacks[i][c];
				}
			}

			delete[] subContainedStackSizes[i];
			delete[] subContainedStacks[i];

			delete[] containedStacks[i];
		}
	}

	delete[] chunk;
	delete[] chunkBiomes;
	delete[] chunkFloors;

	delete[] containedStackSizes;
	delete[] containedStacks;

	delete[] subContainedStackSizes;
	delete[] subContainedStacks;
}

/**********************************************************************************************************************/

int getMapBiomeIndex(int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap)
{

	int secondPlaceBiome = -1;

	int dbBiome = -1;

	if (anyBiomesInDB && inX >= minBiomeXLoc && inX <= maxBiomeXLoc && inY >= minBiomeYLoc && inY <= maxBiomeYLoc)
	{
		// don't bother with this call unless biome DB has
		// something in it, and this inX,inY is in the region where biomes
		// exist in the database (tutorial loading, or test maps)
		dbBiome = biomeDBGet(inX, inY, &secondPlaceBiome, outSecondPlaceGap);
	}

	if (dbBiome != -1)
	{

		int index = getBiomeIndex(dbBiome);

		if (index != -1)
		{
			// biome still exists!

			char secondPlaceFailed = false;

			if (outSecondPlaceIndex != NULL)
			{
				int secondIndex = getBiomeIndex(secondPlaceBiome);

				if (secondIndex != -1) { *outSecondPlaceIndex = secondIndex; }
				else
				{
					secondPlaceFailed = true;
				}
			}

			if (!secondPlaceFailed) { return index; }
		}
		else
		{
			dbBiome = -1;
		}

		// else a biome or second place in biome.db that isn't in game anymore
		// ignore it
	}

	int secondPlace = -1;

	double secondPlaceGap = 0;

	int pickedBiome = computeMapBiomeIndex(inX, inY, &secondPlace, &secondPlaceGap);

	if (outSecondPlaceIndex != NULL) { *outSecondPlaceIndex = secondPlace; }
	if (outSecondPlaceGap != NULL) { *outSecondPlaceGap = secondPlaceGap; }

	if (dbBiome == -1 || secondPlaceBiome == -1)
	{
		// not stored, OR some part of stored stale, re-store it

		secondPlaceBiome = 0;
		if (secondPlace != -1) { secondPlaceBiome = biomes[secondPlace]; }

		// skip saving proc-genned biomes for now
		// huge RAM impact as players explore distant areas of map

		// we still check the biomeDB above for loading test maps
		/*
		biomeDBPut( inX, inY, biomes[pickedBiome],
					secondPlaceBiome, secondPlaceGap );
		*/
	}

	return pickedBiome;
}

void initDBCaches()
{
	DBCacheRecord blankRecord = {0, 0, 0, 0, -2};
	for (int i = 0; i < DB_CACHE_SIZE; i++)
	{
		dbCache[i] = blankRecord;
	}
	// 1 for empty (because 0 is a valid value)
	DBTimeCacheRecord blankTimeRecord = {0, 0, 0, 0, 1};
	for (int i = 0; i < DB_CACHE_SIZE; i++)
	{
		dbTimeCache[i] = blankTimeRecord;
	}
	// -1 for empty
	BlockingCacheRecord blankBlockingRecord = {0, 0, -1};
	for (int i = 0; i < DB_CACHE_SIZE; i++)
	{
		blockingCache[i] = blankBlockingRecord;
	}
}

void initBiomeCache()
{
	BiomeCacheRecord blankRecord = {0, 0, -2, 0, 0};
	for (int i = 0; i < BIOME_CACHE_SIZE; i++)
	{
		biomeCache[i] = blankRecord;
	}
}

void mapCacheClear()
{
	for (int y = 0; y < BASE_MAP_CACHE_SIZE; y++)
	{
		for (int x = 0; x < BASE_MAP_CACHE_SIZE; x++)
		{
			baseMapCache[y][x].x  = 0;
			baseMapCache[y][x].y  = 0;
			baseMapCache[y][x].id = -1;
		}
	}
}

int countNewlines(char *inString)
{
	int len = strlen(inString);
	int num = 0;
	for (int i = 0; i < len; i++)
	{
		if (inString[i] == '\n') { num++; }
	}
	return num;
}

BaseMapCacheRecord *mapCacheRecordLookup(int inX, int inY)
{
	// apply bitmask to x and y
	return &(baseMapCache[inY & mapCacheBitMask][inX & mapCacheBitMask]);
}

int getBiomeIndex(int inBiome)
{
	for (int i = 0; i < numBiomes; i++)
	{
		if (biomes[i] == inBiome) { return i; }
	}
	return -1;
}

// returns -1 if not found
int biomeDBGet(int inX, int inY, int *outSecondPlaceBiome, double *outSecondPlaceGap)
{
	unsigned char key[8];
	unsigned char value[12];

	// look for changes to default in database
	intPairToKey(inX, inY, key);

	int result = DB_get(&biomeDB, key, value);

	if (result == 0)
	{
		// found
		int biome = valueToInt(&(value[0]));

		if (outSecondPlaceBiome != NULL) { *outSecondPlaceBiome = valueToInt(&(value[4])); }

		if (outSecondPlaceGap != NULL) { *outSecondPlaceGap = valueToInt(&(value[8])) / gapIntScale; }

		return biome;
	}
	else
	{
		return -1;
	}
}

void deleteFileByName(const char *inFileName)
{
	File f(NULL, inFileName);

	if (f.exists()) { f.remove(); }
}

void setContained(int inX, int inY, int inNumContained, int *inContained, int inSubCont)
{
	dbPut(inX, inY, NUM_CONT_SLOT, inNumContained, inSubCont);
	for (int i = 0; i < inNumContained; i++)
	{
		changeContained(inX, inY, i, inSubCont, inContained[i]);
	}
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 * for performance reasons, when the true decayed version of the object doesn't matter, this skips some expensive steps
 */
int getMapObjectRaw(int inX, int inY)
{

	int barrier = getPossibleBarrier(inX, inY);

	if (barrier != 0) { return barrier; }

	int result = dbGet(inX, inY, 0);

	if (result == -1) { result = getTweakedBaseMap(inX, inY); }

	return result;
}

int getPossibleBarrier(int inX, int inY)
{
	if (barrierOn)
		if (inX == barrierRadius || inX == -barrierRadius || inY == barrierRadius || inY == -barrierRadius)
		{

			// along barrier line

			// now make sure that we don't stick out beyond square

			if (inX <= barrierRadius && inX >= -barrierRadius && inY <= barrierRadius && inY >= -barrierRadius)
			{

				setXYRandomSeed(9238597);

				int numOptions = barrierItemList.size();

				if (numOptions > 0)
				{

					// random doesn't always look good
					int pick = floor(numOptions * getXYRandom(inX * 10, inY * 10));

					if (pick >= numOptions) { pick = numOptions - 1; }

					int barrierID = barrierItemList.getElementDirect(pick);

					if (getObject(barrierID) != NULL)
					{
						// actually exists
						return barrierID;
					}
				}
			}
		}

	return 0;
}

int getTweakedBaseMap(int inX, int inY)
{

	// nothing in map
	char wasGridPlacement = false;

	int result = getBaseMap(inX, inY, &wasGridPlacement);

	if (result > 0)
	{
		ObjectRecord *o = getObject(result);

		if (o->wide)
		{
			// make sure there's not possibly another wide object too close
			int maxR = getMaxWideRadius();

			for (int dx = -(o->leftBlockingRadius + maxR); dx <= (o->rightBlockingRadius + maxR); dx++)
			{

				if (dx != 0)
				{
					int nID = getBaseMap(inX + dx, inY);

					if (nID > 0)
					{
						ObjectRecord *nO = getObject(nID);

						if (nO->wide)
						{

							int minDist;
							int dist;

							if (dx < 0)
							{
								minDist = nO->rightBlockingRadius + o->leftBlockingRadius;
								dist    = -dx;
							}
							else
							{
								minDist = nO->leftBlockingRadius + o->rightBlockingRadius;
								dist    = dx;
							}

							if (dist <= minDist)
							{
								// collision
								// don't allow this wide object here
								return 0;
							}
						}
					}
				}
			}
		}
		else if (!wasGridPlacement && getObjectHeight(result) < CELL_D)
		{
			// a short object should be here
			// and it wasn't forced by a grid placement

			// make sure there's not any semi-short objects below already

			// this avoids vertical stacking of short objects
			// and ensures that the map is sparse with short object
			// clusters, even in very dense map areas
			// (we don't want the floor tiled with berry bushes)

			// this used to be an unintentional bug, but it was in place
			// for a year, and we got used to it.

			// when the bug was fixed, the map became way too dense
			// in short-object areas

			// actually, fully replicate the bug for now
			// only block short objects with objects to the south
			// that extend above the tile midline

			// So we can have clusters of very short objects, like stones
			// but not less-short ones like berry bushes, rabbit holes, etc.

			// use the old buggy "2 pixels" and "3 pixels" above the
			// midline measure just to keep the map the same

			// south
			int sID = getBaseMap(inX, inY - 1);

			if (sID > 0 && getObjectHeight(sID) >= 2) { return 0; }

			int s2ID = getBaseMap(inX, inY - 2);

			if (s2ID > 0 && getObjectHeight(s2ID) >= 3) { return 0; }
		}
	}
	return result;
}

int getBaseMap(int inX, int inY, char *outGridPlacement)
{

	if (inX > xLimit || inX < -xLimit || inY > yLimit || inY < -yLimit) { return edgeObjectID; }

	int cachedID = mapCacheLookup(inX, inY, outGridPlacement);

	if (cachedID != -1) { return cachedID; }

	getBaseMapCallCount++;

	if (outGridPlacement != NULL) { *outGridPlacement = false; }

	// see if any of our grids apply
	setXYRandomSeed(9753);

	for (int g = 0; g < gridPlacements.size(); g++)
	{
		MapGridPlacement *gp = gridPlacements.getElement(g);

		/*
		double gridWiggleX = getXYFractal( inX / gp->spacing,
										   inY / gp->spacing,
										   0.1, 0.25 );

		double gridWiggleY = getXYFractal( inX / gp->spacing,
										   inY / gp->spacing + 392387,
										   0.1, 0.25 );
		*/
		// turn wiggle off for now
		double gridWiggleX = 0;
		double gridWiggleY = 0;

		if ((inX + gp->phase + lrint(gridWiggleX * gp->wiggleScale)) % gp->spacing == 0
			&& (inY + gp->phase + lrint(gridWiggleY * gp->wiggleScale)) % gp->spacing == 0)
		{

			// hits this grid

			// make sure this biome is on the list for this object
			int    secondPlace;
			double secondPlaceGap;

			int pickedBiome = getMapBiomeIndex(inX, inY, &secondPlace, &secondPlaceGap);

			if (pickedBiome == -1)
			{
				mapCacheInsert(inX, inY, 0);
				return 0;
			}

			if (gp->permittedBiomes.getElementIndex(pickedBiome) != -1)
			{
				mapCacheInsert(inX, inY, gp->id, true);

				if (outGridPlacement != NULL) { *outGridPlacement = true; }
				return gp->id;
			}
		}
	}

	setXYRandomSeed(5379);

	// first step:  save rest of work if density tells us that
	// nothing is here anyway
	double density = getXYFractal(inX, inY, 0.1, 0.25);

	// correction
	density = sigmoid(density, 0.1);

	// scale
	density *= .4;
	// good for zoom in to map for teaser
	// density = 1;

	setXYRandomSeed(9877);
	int    secondPlace;
	double secondPlaceGap;

	int pickedBiome = getMapBiomeIndex(inX, inY, &secondPlace, &secondPlaceGap);

	if (biomes[pickedBiome] == 7) { density = 1; }
	setXYRandomSeed(9877);
	if (getXYRandom(inX, inY) < density)
	{

		// next step, pick top two biomes
		int    secondPlace;
		double secondPlaceGap;

		int pickedBiome = getMapBiomeIndex(inX, inY, &secondPlace, &secondPlaceGap);

		if (pickedBiome == -1)
		{
			mapCacheInsert(inX, inY, 0);
			return 0;
		}

		// only override if it's not already set
		// if it's already set, then we're calling getBaseMap for neighboring
		// map cells (wide, tall, moving objects, etc.)
		// getBaseMap is always called for our cell in question first
		// before examining neighboring cells if needed
		if (lastCheckedBiome == -1)
		{
			lastCheckedBiome  = biomes[pickedBiome];
			lastCheckedBiomeX = inX;
			lastCheckedBiomeY = inY;
		}

		// randomly let objects from second place biome peek through

		// if gap is 0, this should happen 50 percent of the time

		// if gap is 1.0, it should never happen

		// larger values make second place less likely
		// double secondPlaceReduction = 10.0;

		// printf( "Second place gap = %f, random(%d,%d)=%f\n", secondPlaceGap,
		//        inX, inY, getXYRandom( 2087 + inX, 793 + inY ) );

		setXYRandomSeed(348763);

		int numObjects = naturalMapIDs[pickedBiome].size();

		if (numObjects == 0)
		{
			mapCacheInsert(inX, inY, 0);
			return 0;
		}

		// something present here

		// special object in this region is 10x more common than it
		// would be otherwise

		int    specialObjectIndex = -1;
		double maxValue           = -DBL_MAX;

		for (int i = 0; i < numObjects; i++)
		{

			setXYRandomSeed(793 * i + 123);

			double randVal = getXYFractal(inX, inY, 0.3, 0.15 + 0.016666 * numObjects);

			if (randVal > maxValue)
			{
				maxValue           = randVal;
				specialObjectIndex = i;
			}
		}

		float oldSpecialChance = naturalMapChances[pickedBiome].getElementDirect(specialObjectIndex);

		float newSpecialChance = oldSpecialChance * 10;

		*(naturalMapChances[pickedBiome].getElement(specialObjectIndex)) = newSpecialChance;

		float oldTotalChanceWeight = totalChanceWeight[pickedBiome];

		totalChanceWeight[pickedBiome] -= oldSpecialChance;
		totalChanceWeight[pickedBiome] += newSpecialChance;

		// pick one of our natural objects at random

		// pick value between 0 and total weight

		setXYRandomSeed(4593873);

		double randValue = totalChanceWeight[pickedBiome] * getXYRandom(inX, inY);

		// walk through objects, summing weights, until one crosses threshold
		int   i         = 0;
		float weightSum = 0;

		while (weightSum < randValue && i < numObjects)
		{

			weightSum += naturalMapChances[pickedBiome].getElementDirect(i);
			i++;
		}
		i--;

		// restore chance of special object
		*(naturalMapChances[pickedBiome].getElement(specialObjectIndex)) = oldSpecialChance;

		totalChanceWeight[pickedBiome] = oldTotalChanceWeight;

		if (i >= 0)
		{
			int returnID = naturalMapIDs[pickedBiome].getElementDirect(i);

			if (pickedBiome == secondPlace)
			{
				// object peeking through from second place biome

				// make sure it's not a moving object (animal)
				// those are locked to their target biome only
				TransRecord *t = getPTrans(-1, returnID);
				if (t != NULL && t->move != 0)
				{
					// put empty tile there instead
					returnID = 0;
				}
			}

			mapCacheInsert(inX, inY, returnID);
			return returnID;
		}
		else
		{
			mapCacheInsert(inX, inY, 0);
			return 0;
		}
	}
	else
	{
		if (biomes[pickedBiome] != 7)
		{
			mapCacheInsert(inX, inY, 0);
			return 0;
		}
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param outGridPlacement
 * @return
 * @note from server/map.cpp
 * // returns -1 if not in cache
 */
int mapCacheLookup(int inX, int inY, char *outGridPlacement)
{
	BaseMapCacheRecord *r = mapCacheRecordLookup(inX, inY);

	if (r->x == inX && r->y == inY)
	{
		if (outGridPlacement != NULL) { *outGridPlacement = r->gridPlacement; }
		return r->id;
	}

	return -1;
}

void mapCacheInsert(int inX, int inY, int inID, char inGridPlacement)
{
	BaseMapCacheRecord *r = mapCacheRecordLookup(inX, inY);

	r->x             = inX;
	r->y             = inY;
	r->id            = inID;
	r->gridPlacement = inGridPlacement;
}