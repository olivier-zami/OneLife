//
// Created by olivier on 18/04/2022.
//

#include "Server.h"

#include <cstdio>
#include <cstddef>

#include "dbCommon.h"
#include "map.h"
#include "component/Map.h"
#include "component/Speech.h"
#include "HashTable.h"
#include "spiral.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/objectMetadata.h"
#include "../third_party/minorGems/io/file/File.h"
#include "../third_party/minorGems/system/Time.h"
#include "../third_party/minorGems/util/log/AppLog.h"
#include "../third_party/minorGems/util/MinPriorityQueue.h"
#include "../third_party/minorGems/util/random/CustomRandomSource.h"
#include "../third_party/minorGems/util/SettingsManager.h"
#include "../third_party/minorGems/util/SimpleVector.h"

#define MAP_TIMESEC Time::timeSec()// can replace with frozenTime to freeze time or slowTime to slow it down

extern SimpleVector<int> eveSecondaryLocObjectIDs;
extern SimpleVector<double> recentlyUsedPrimaryEvePositionTimes;
extern SimpleVector<GridPos> recentlyUsedPrimaryEvePositions;
extern SimpleVector<int> recentlyUsedPrimaryEvePositionPlayerIDs;
extern SimpleVector<int> barrierItemList;
extern SimpleVector<GlobalTriggerState> globalTriggerStates;
extern SimpleVector<GridPos> *speechPipesIn;
extern SimpleVector<GridPos> *speechPipesOut;
extern SimpleVector<int> *naturalMapIDs;// one vector per biome
extern SimpleVector<float> *naturalMapChances;
extern SimpleVector<int> allNaturalMapIDs;
extern SimpleVector<MapGridPlacement> gridPlacements;
extern BlockingCacheRecord blockingCache[];
extern RecentPlacement recentPlacements[];
extern BiomeCacheRecord biomeCache[];
extern DBTimeCacheRecord dbTimeCache[];
extern DBCacheRecord dbCache[];
extern GridPos eveLocation;
extern int barrierRadius;
extern int barrierOn;
extern float minEveCampRespawnAge;
extern int longTermCullEnabled;
extern int numSpeechPipes;
extern int edgeObjectID;
extern int eveRadius;
extern int maxBiomeXLoc;
extern int maxBiomeYLoc;
extern int minBiomeXLoc;
extern int minBiomeYLoc;
extern char lookTimeDBEmpty;
extern char skipLookTimeCleanup;
extern int nextPlacementIndex;
extern char anyBiomesInDB;
extern int cellsLookedAtToInit;
extern int numBiomes;
extern int* biomes;
extern float* biomeWeights;
extern float* biomeCumuWeights;
extern float biomeTotalWeight;
extern int regularBiomeLimit;
extern int numSpecialBiomes;
extern int* specialBiomes;
extern float* specialBiomeCumuWeights;
extern float specialBiomeTotalWeight;
extern float* totalChanceWeight;
extern int eveHomeMarkerObjectID;
extern int randSeed;
extern char skipRemovedObjectCleanup;
extern char skipTrackingMapChanges;
// extern JenkinsRandomSource randSource;
extern CustomRandomSource randSource;
extern SimpleVector<ChangePosition> mapChangePosSinceLastStep;

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


HashTable<timeSec_t> liveDecayRecordLastLookTimeHashTable(1024);// times in seconds that a tracked live decay map cell or slot// was last looked at
HashTable<double> liveMovementEtaTimes(1024, 0);// clock time in fractional seconds of destination ETA indexed as x, y, 0
int evePrimaryLocObjectID = -1;
int evePrimaryLocSpacing  = 0;
MinPriorityQueue<MovementRecord> liveMovements;
SimpleVector<LiveObject> players;

static char useTestMap = false;// if true, rest of natural map is blank

OneLife::Server::Server() {}
OneLife::Server::~Server() {}

/**
 *
 * @return
 * @note from initMap in map.cpp
 * returns true on success
 */
bool OneLife::Server::initMap()
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
 * @param inX
 * @param inY
 * @param outNumContained
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 */
int *getContainedRaw(int inX, int inY, int *outNumContained, int inSubCont)
{
	int num = getNumContained(inX, inY, inSubCont);

	*outNumContained = num;

	if (num == 0) { return NULL; }

	int *contained = new int[num];

	int trueNum = 0;

	for (int i = 0; i < num; i++)
	{
		int result = dbGet(inX, inY, FIRST_CONT_SLOT + i, inSubCont);
		if (result == -1) { result = 0; }
		if (result != 0)
		{
			contained[trueNum] = result;
			trueNum++;
		}
	}

	*outNumContained = trueNum;

	if (trueNum < num)
	{
		// fix filled count permanently in DB
		dbPut(inX, inY, NUM_CONT_SLOT, trueNum, inSubCont);
	}

	if (trueNum > 0) { return contained; }
	else
	{
		delete[] contained;
		return NULL;
	}
}


/**
 *
 * @param inX
 * @param inY
 * @param inSubCont
 * @param inNoDecay
 * @note from server/map.cpp
 */
void setSlotItemsNoDecay(int inX, int inY, int inSubCont, char inNoDecay)
{
	int val = 1;
	if (inNoDecay) { val = 0; }
	dbPut(inX, inY, NO_DECAY_SLOT, val, inSubCont);
}

/**
 *
 * @param inX
 * @param inY
 * @param inID
 * @return
 * @note from server/map.cpp
 */
int checkDecayObject(int inX, int inY, int inID)
{
	if (inID == 0) { return inID; }

	TransRecord *t = getPTrans(-1, inID);

	if (t == NULL)
	{
		// no auto-decay for this object
		return inID;
	}

	// else decay exists for this object

	int newID        = inID;
	int movingObjID  = newID;
	int leftBehindID = 0;

	// in case of movement
	int newX = inX;
	int newY = inY;

	// is eta stored in map?
	timeSec_t mapETA = getEtaDecay(inX, inY);

	if (mapETA != 0)
	{

		if ((int)mapETA <= MAP_TIMESEC)
		{

			// object in map has decayed (eta expired)

			// apply the transition
			newID       = t->newTarget;
			movingObjID = newID;

			int oldSlots = getNumContainerSlots(inID);

			int newSlots = getNumContainerSlots(newID);

			if (newSlots < oldSlots) { shrinkContainer(inX, inY, newSlots); }
			if (newSlots > 0) { restretchMapContainedDecays(inX, inY, inID, newID); }

			if (t->move != 0)
			{
				// moving
				doublePair dir = {0, 0};

				TransRecord *destTrans = NULL;

				int desiredMoveDist = t->desiredMoveDist;

				char stayInBiome = false;

				char avoidFloor = false;

				if (t->newTarget > 0 && strstr(getObject(t->newTarget)->description, "groundOnly"))
				{ avoidFloor = true; }

				if (t->move < 3)
				{

					GridPos p = getClosestPlayerPos(inX, inY);

					double dX = (double)p.x - (double)inX;
					double dY = (double)p.y - (double)inY;

					double dist = sqrt(dX * dX + dY * dY);

					if (dist <= 7 && (p.x != 0 || p.y != 0))
					{

						if (t->move == 1 && dist <= desiredMoveDist)
						{
							// chase.  Try to land exactly on them
							// if they're close enough to do it in one move
							desiredMoveDist = lrint(dist);
						}

						dir.x = dX;
						dir.y = dY;
						dir   = normalize(dir);

						// round to one of 8 cardinal directions

						double a = angle(dir);

						a = 2 * M_PI * lrint((a / (2 * M_PI)) * 8) / 8.0;

						dir.x = 1;
						dir.y = 0;
						dir   = rotate(dir, a);
					}
					if (t->move == 2)
					{
						// flee
						stayInBiome = true;
						dir         = mult(dir, -1);
					}
				}
				else if (t->move > 3 && t->move < 8)
				{
					// NSEW

					switch (t->move)
					{
						case 4: dir.y = 1; break;
						case 5: dir.y = -1; break;
						case 6: dir.x = 1; break;
						case 7: dir.x = -1; break;
					}
				}
				else if (t->move == 8)
				{

					// prioritize picking the direction with
					// transition-able object on the path

					int startDirX = randSource.getRandomBoundedInt(-1, 1);
					int startDirY = randSource.getRandomBoundedInt(-1, 1);

					for (int dx = 0; dx < 3; dx++)
					{
						for (int dy = 0; dy < 3; dy++)
						{

							int newDirX = ((startDirX + 1) + dx) % 3 - 1;
							int newDirY = ((startDirY + 1) + dy) % 3 - 1;

							if (newDirX == 0 && newDirY == 0) continue;

							int tryDist = t->desiredMoveDist;

							if (tryDist < 1) { tryDist = 1; }

							int tryRadius = 4;

							while (newX == inX && newY == inY && tryDist > 0)
							{

								for (int i = 1; i <= tryDist + tryRadius; i++)
								{

									int testX = lrint(inX + newDirX * i);
									int testY = lrint(inY + newDirY * i);

									int oID = getMapObjectRaw(testX, testY);

									TransRecord *trans = NULL;

									if (oID > 0)
									{
										trans = getPTrans(inID, oID);

										if (trans == NULL) { trans = getPTrans(newID, oID); }
									}

									if (i >= tryDist && trans != NULL)
									{
										dir.x     = (double)newDirX;
										dir.y     = (double)newDirY;
										newX      = testX;
										newY      = testY;
										destTrans = trans;
										break;
									}
									else if (oID > 0 && getObject(oID) != NULL && getObject(oID)->blocksMoving)
									{
										break;
									}
								}

								tryDist--;

								if (tryRadius != 0) { tryRadius = 1; }
							}

							if (newX != inX && newY != inY) break;
						}
					}
				}

				// round to 1000ths to avoid rounding errors
				// that can separate our values from zero

				dir.x = lrint(dir.x * 1000) / 1000.0;
				dir.y = lrint(dir.y * 1000) / 1000.0;

				if (dir.x == 0 && dir.y == 0)
				{
					// random instead

					stayInBiome = true;

					dir.x = 1;
					dir.y = 0;

					// 8 cardinal directions
					dir = rotate(dir, 2 * M_PI * randSource.getRandomBoundedInt(0, 7) / 8.0);
				}

				if (dir.x != 0 && dir.y != 0)
				{
					// diag

					// push both up to full step

					if (dir.x < 0) { dir.x = -1; }
					else if (dir.x > 0)
					{
						dir.x = 1;
					}

					if (dir.y < 0) { dir.y = -1; }
					else if (dir.y > 0)
					{
						dir.y = 1;
					}
				}

				// now we have the dir we want to go in

				int tryDist = desiredMoveDist;

				if (tryDist < 1) { tryDist = 1; }

				int tryRadius = 4;

				if (t->move > 3 && tryDist == 1)
				{
					// single-step NSEW moves never go beyond
					// their intended distance
					tryRadius = 0;
				}

				// try again and again with smaller distances until we
				// find an empty spot
				while (newX == inX && newY == inY && tryDist > 0)
				{

					// walk up to 4 steps past our dist in that direction,
					// looking for non-blocking objects or an empty spot

					for (int i = 1; i <= tryDist + tryRadius; i++)
					{
						int testX = lrint(inX + dir.x * i);
						int testY = lrint(inY + dir.y * i);

						int oID = getMapObjectRaw(testX, testY);

						// does trans exist for this object used on destination
						// obj?
						TransRecord *trans = NULL;

						if (oID > 0)
						{
							trans = getPTrans(inID, oID);

							if (trans == NULL)
							{
								// does trans exist for newID applied to
								// destination?
								trans = getPTrans(newID, oID);
							}
						}
						else if (oID == 0)
						{
							// check for bare ground trans
							trans = getPTrans(inID, -1);

							if (trans == NULL)
							{
								// does trans exist for newID applied to
								// bare ground
								trans = getPTrans(newID, -1);
							}
						}

						char blockedByFloor = false;

						if (oID == 0 && avoidFloor)
						{
							int floorID = getMapFloor(testX, testY);

							if (floorID > 0) { blockedByFloor = true; }
						}

						if (i >= tryDist && oID == 0)
						{
							if (!blockedByFloor)
							{
								// found a bare ground spot for it to move
								newX = testX;
								newY = testY;
								// keep any bare ground transition (or NULL)
								destTrans = trans;
								break;
							}
						}
						else if (i >= tryDist && trans != NULL)
						{
							newX      = testX;
							newY      = testY;
							destTrans = trans;
							break;
						}
						else if (oID > 0 && getObject(oID) != NULL && getObject(oID)->blocksMoving)
						{
							// blocked, stop now
							break;
						}
						// else walk through it
					}

					tryDist--;

					if (tryRadius != 0)
					{
						// 4 on first try, but then 1 on remaining tries to
						// avoid overlap with previous tries
						tryRadius = 1;
					}
				}

				int curBiome = -1;
				if (stayInBiome)
				{
					curBiome = getMapBiome(inX, inY);

					if (newX != inX || newY != inY)
					{
						int newBiome = getMapBiome(newX, newY);

						if (newBiome != curBiome)
						{
							// block move
							newX = inX;
							newY = inY;

							// forget about trans that we found above
							// it crosses biome boundary
							// (and this fixes the infamous sliding
							//  penguin ice-hole bug)
							destTrans = NULL;
						}
					}
				}

				if (newX == inX && newY == inY && t->move <= 3)
				{
					// can't move where we want to go in flee/chase/random

					// pick some random spot to go instead

					int possibleX[8];
					int possibleY[8];
					int numPossibleDirs = 0;

					for (int d = 0; d < 8; d++)
					{

						dir.x = 1;
						dir.y = 0;

						// 8 cardinal directions
						dir = rotate(dir, 2 * M_PI * d / 8.0);

						tryDist = t->desiredMoveDist;

						if (tryDist < 1) { tryDist = 1; }

						tryRadius = 4;

						// try again and again with smaller distances until we
						// find an empty spot
						char stopCheckingDir = false;

						while (!stopCheckingDir && tryDist > 0)
						{

							// walk up to 4 steps in that direction, looking
							// for non-blocking objects or an empty spot

							for (int i = 0; i < tryDist + tryRadius; i++)
							{
								int testX = lrint(inX + dir.x * i);
								int testY = lrint(inY + dir.y * i);

								int oID = getMapObjectRaw(testX, testY);

								if (i >= tryDist && oID == 0)
								{
									// found a spot for it to move

									if (stayInBiome && curBiome != getMapBiome(testX, testY)) { continue; }
									if (avoidFloor)
									{
										int floorID = getMapFloor(testX, testY);

										if (floorID > 0)
										{
											// blocked by floor
											continue;
										}
									}

									possibleX[numPossibleDirs] = testX;
									possibleY[numPossibleDirs] = testY;
									numPossibleDirs++;
									stopCheckingDir = true;
									break;
								}
								else if (oID > 0 && getObject(oID) != NULL && getObject(oID)->blocksMoving)
								{
									// blocked, stop now
									break;
								}
								// else walk through it
							}

							tryDist--;
							// 1 on remaining tries to avoid overlap
							tryRadius = 1;
						}
					}

					if (numPossibleDirs > 0)
					{
						int pick = randSource.getRandomBoundedInt(0, numPossibleDirs - 1);

						newX = possibleX[pick];
						newY = possibleY[pick];
					}
				}

				if (newX != inX || newY != inY)
				{
					// a reall move!

					printf("Object moving from (%d,%d) to (%d,%d)\n", inX, inY, newX, newY);

					// move object

					if (destTrans != NULL) { newID = destTrans->newTarget; }

					dbPut(newX, newY, 0, newID);

					// update old spot
					// do this second, so that it is reported to client
					// after move is reported

					if (destTrans == NULL || destTrans->newActor == 0)
					{
						// try bare ground trans
						destTrans = getPTrans(inID, -1);

						if (destTrans == NULL)
						{
							// another attempt at bare ground transition
							destTrans = getPTrans(movingObjID, -1);
						}

						if (destTrans != NULL && destTrans->newTarget != newID && destTrans->newTarget != movingObjID)
						{
							// for bare ground, make sure newTarget
							// matches newTarget of our orginal move transition
							destTrans = NULL;
						}
					}

					if (destTrans != NULL)
					{
						// leave new actor behind

						leftBehindID = destTrans->newActor;

						dbPut(inX, inY, 0, leftBehindID);

						TransRecord *leftDecayT = getMetaTrans(-1, leftBehindID);

						double leftMapETA = 0;

						if (leftDecayT != NULL)
						{

							// add some random variation to avoid lock-step
							// especially after a server restart
							int tweakedSeconds = randSource.getRandomBoundedInt(
									lrint(leftDecayT->autoDecaySeconds * 0.9), leftDecayT->autoDecaySeconds);

							if (tweakedSeconds < 1) { tweakedSeconds = 1; }
							leftMapETA = MAP_TIMESEC + tweakedSeconds;
						}
						else
						{
							// no further decay
							leftMapETA = 0;
						}
						// for movement from posA to posB, we want posA to be potentially always live tracked as well
						// leftDecayT is passed to check if it should be always live tracked
						setEtaDecay(inX, inY, leftMapETA, leftDecayT);
					}
					else
					{
						// leave empty spot behind
						dbPut(inX, inY, 0, 0);
						leftBehindID = 0;
					}

					// move contained
					int        numCont;
					int *      cont    = getContained(inX, inY, &numCont);
					timeSec_t *contEta = getContainedEtaDecay(inX, inY, &numCont);

					if (numCont > 0)
					{
						setContained(newX, newY, numCont, cont);
						setContainedEtaDecay(newX, newY, numCont, contEta);

						for (int c = 0; c < numCont; c++)
						{
							if (cont[c] < 0)
							{
								// sub cont
								int        numSub;
								int *      subCont    = getContained(inX, inY, &numSub, c + 1);
								timeSec_t *subContEta = getContainedEtaDecay(inX, inY, &numSub, c + 1);

								if (numSub > 0)
								{
									setContained(newX, newY, numSub, subCont, c + 1);
									setContainedEtaDecay(newX, newY, numSub, subContEta, c + 1);
								}
								delete[] subCont;
								delete[] subContEta;
							}
						}

						clearAllContained(inX, inY);

						delete[] cont;
						delete[] contEta;
					}

					double moveDist = sqrt((newX - inX) * (newX - inX) + (newY - inY) * (newY - inY));

					double speed = 4.0f;

					if (newID > 0)
					{
						ObjectRecord *newObj = getObject(newID);

						if (newObj != NULL) { speed *= newObj->speedMult; }
					}

					double moveTime = moveDist / speed;

					double etaTime = Time::getCurrentTime() + moveTime;

					MovementRecord moveRec = {newX, newY, etaTime};

					liveMovementEtaTimes.insert(newX, newY, 0, 0, etaTime);

					liveMovements.insert(moveRec, etaTime);

					// now patch up change record marking this as a move

					for (int i = 0; i < mapChangePosSinceLastStep.size(); i++)
					{

						ChangePosition *p = mapChangePosSinceLastStep.getElement(i);

						if (p->x == newX && p->y == newY)
						{

							// update it
							p->oldX  = inX;
							p->oldY  = inY;
							p->speed = (float)speed;

							break;
						}
					}
				}
				else
				{
					// failed to find a spot to move

					// default to applying bare-ground transition, if any
					TransRecord *trans = getPTrans(inID, -1);

					if (trans == NULL)
					{
						// does trans exist for newID applied to
						// bare ground
						trans = getPTrans(newID, -1);
					}
					if (trans != NULL)
					{
						newID = trans->newTarget;

						// what was SUPPOSED to be left behind on ground
						// that object moved away from?
						if (trans->newActor > 0)
						{

							// see if there's anything defined for when
							// the new object moves ONTO this thing

							// (object is standing still in same spot,
							//  effectively on top of what it was supposed
							//  to leave behind)

							TransRecord *inPlaceTrans = getPTrans(newID, trans->newActor);

							if (inPlaceTrans != NULL && inPlaceTrans->newTarget > 0)
							{ newID = inPlaceTrans->newTarget; }
						}
					}
				}
			}

			if (newX == inX && newY == inY)
			{
				// no move happened

				// just set change in DB
				setMapObjectRaw(inX, inY, newID);
			}

			TransRecord *newDecayT = getMetaTrans(-1, newID);

			if (newDecayT != NULL)
			{

				// add some random variation to avoid lock-step
				// especially after a server restart
				int tweakedSeconds = randSource.getRandomBoundedInt(
						lrint(newDecayT->autoDecaySeconds * 0.9), newDecayT->autoDecaySeconds);

				if (tweakedSeconds < 1) { tweakedSeconds = 1; }
				mapETA = MAP_TIMESEC + tweakedSeconds;
			}
			else
			{
				// no further decay
				mapETA = 0;
			}

			if (mapETA != 0 && (newX != inX || newY != inY))
			{

				// copy old last look time from where we came from
				char foundInOldSpot;

				timeSec_t lastLookTime = liveDecayRecordLastLookTimeHashTable.lookup(inX, inY, 0, 0, &foundInOldSpot);

				if (foundInOldSpot)
				{

					char foundInNewSpot;
					liveDecayRecordLastLookTimeHashTable.lookup(newX, newY, 0, 0, &foundInNewSpot);

					if (!foundInNewSpot)
					{
						// we're not tracking decay for this new cell yet
						// but leave a look time here to affect
						// the tracking that we're about to setup

						liveDecayRecordLastLookTimeHashTable.insert(newX, newY, 0, 0, lastLookTime);
					}
				}
			}

			setEtaDecay(newX, newY, mapETA, newDecayT);
		}
	}
	else
	{
		// an object on the map that has never been seen by anyone before
		// not decaying yet

		// update map with decay for the applicable transition

		// randomize it so that every same object on map
		// doesn't cycle at same time
		int decayTime = randSource.getRandomBoundedInt(t->autoDecaySeconds / 2, t->autoDecaySeconds);
		if (decayTime < 1) { decayTime = 1; }

		mapETA = MAP_TIMESEC + decayTime;

		setEtaDecay(inX, inY, mapETA);
	}

	if (newX != inX || newY != inY)
	{
		// object moved and is gone
		return leftBehindID;
	}
	else
	{
		return newID;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inSubCont
 * @note from server/map.cpp
 * // if inSubCont is 0, container and all sub-containers are cleared
// otherwise, clears only a specific sub-container
 */
void clearAllContained(int inX, int inY, int inSubCont)
{
	int oldNum = getNumContained(inX, inY, inSubCont);

	if (inSubCont == 0)
	{
		// clear sub container slots too, if any

		for (int i = 0; i < oldNum; i++)
		{
			if (getNumContained(inX, inY, i + 1) > 0) { dbPut(inX, inY, NUM_CONT_SLOT, 0, i + 1); }
		}
	}

	if (oldNum != 0) { dbPut(inX, inY, NUM_CONT_SLOT, 0, inSubCont); }
}

/**
 *
 * @param inX
 * @param inY
 * @param inNumNewSlots
 * @param inSubCont
 * @note from server/map.cpp
 * // if inNumNewSlots less than number contained, the excess are discarded
 */
void shrinkContainer(int inX, int inY, int inNumNewSlots, int inSubCont)
{
	int oldNum = getNumContained(inX, inY, inSubCont);

	if (oldNum > inNumNewSlots)
	{

		// first, scatter extra contents into empty nearby spots.
		int nextSprialIndex = 1;

		for (int i = inNumNewSlots; i < oldNum; i++)
		{

			int contID = getContained(inX, inY, i, inSubCont);

			char subCont = false;

			if (contID < 0)
			{
				contID *= -1;
				subCont = true;
			}

			int  emptyX, emptyY;
			char foundEmpty = false;

			GridPos center = {inX, inY};

			while (!foundEmpty)
			{
				GridPos sprialPoint = getSpriralPoint(center, nextSprialIndex);
				if (getMapObjectRaw(sprialPoint.x, sprialPoint.y) == 0)
				{
					emptyX     = sprialPoint.x;
					emptyY     = sprialPoint.y;
					foundEmpty = true;
				}
				nextSprialIndex++;
			}

			if (foundEmpty)
			{
				setMapObject(emptyX, emptyY, contID);

				if (subCont)
				{
					int numSub = getNumContained(inX, inY, i + 1);

					for (int s = 0; s < numSub; s++)
					{
						addContained(
								emptyX, emptyY, getContained(inX, inY, s, i + 1), getSlotEtaDecay(inX, inY, s, i + 1));
					}
				}
			}
		}

		// now clear old extra contents from original spot
		dbPut(inX, inY, NUM_CONT_SLOT, inNumNewSlots, inSubCont);

		if (inSubCont == 0)
		{
			// clear sub cont slots too
			for (int i = inNumNewSlots; i < oldNum; i++)
			{
				dbPut(inX, inY, NUM_CONT_SLOT, 0, i + 1);
			}
		}
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 * // returns true if no contained items will decay
 */
char getSlotItemsNoDecay(int inX, int inY, int inSubCont)
{
	int result = dbGet(inX, inY, NO_DECAY_SLOT, inSubCont);

	if (result != -1)
	{
		// found
		return (result == 0);
	}
	else
	{
		// default, some may decay
		return false;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/server.cpp => server/main.cpp
 * // returns (0,0) if no player found
 */
GridPos getClosestPlayerPos( int inX, int inY )
{
	GridPos c = { inX, inY };

	double closeDist = DBL_MAX;
	GridPos closeP = { 0, 0 };

	for( int i=0; i<players.size(); i++ ) {
		LiveObject *o = players.getElement( i );
		if( o->error ) {
			continue;
		}

		GridPos p;

		if( o->xs == o->xd && o->ys == o->yd ) {
			p.x = o->xd;
			p.y = o->yd;
		}
		else {
			p = computePartialMoveSpot( o );
		}

		double d = distance( p, c );

		if( d < closeDist ) {
			closeDist = d;
			closeP = p;
		}
	}
	return closeP;
}

/**
 *
 * @param inPlayer
 * @return
 * @note from server/main.cpp => server/server.cpp
 */
GridPos computePartialMoveSpot( LiveObject *inPlayer ) {

	int c = computePartialMovePathStep( inPlayer );

	if( c >= 0 ) {

		GridPos cPos = inPlayer->pathToDest[c];

		return cPos;
	}
	else {
		GridPos cPos = { inPlayer->xs, inPlayer->ys };

		return cPos;
	}
}

/**
 *
 * @param inPlayer
 * @return
 * @note from server/main.cpp => server/server.cpp
 * // compute closest starting position part way along
// path
// (-1 if closest spot is starting spot not included in path steps)
 */
int computePartialMovePathStep( LiveObject *inPlayer )
{
	double fractionDone =
			( Time::getCurrentTime() -
			  inPlayer->moveStartTime )
			/ inPlayer->moveTotalSeconds;

	if( fractionDone > 1 ) {
		fractionDone = 1;
	}

	int c =
			lrint( ( inPlayer->pathLength ) *
				   fractionDone );
	return c - 1;
}