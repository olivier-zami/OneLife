//
// Created by olivier on 14/04/2022.
//

#include "Map.h"

#include <float.h>

#include "../arcReport.h"
#include "../component/Log.h"
#include "../CoordinateTimeTracking.h"
#include "../dataType/LiveObject.h"
#include "../dbCommon.h"
#include "../HashTable.h"
#include "../map.h"
#include "../monument.h"
#include "../spiral.h"
#include "../../commonSource/fractalNoise.h"
#include "../../commonSource/math/misc.h"
#include "../../gameSource/objectMetadata.h"
#include "../../gameSource/categoryBank.h"
#include "../../third_party/minorGems/io/file/File.h"
#include "../../third_party/minorGems/util/MinPriorityQueue.h"
#include "../../third_party/minorGems/util/stringUtils.h"
#include "../../third_party/minorGems/util/SettingsManager.h"
#include "../../third_party/minorGems/util/log/AppLog.h"
#include "../../third_party/minorGems/util/random/CustomRandomSource.h"
#include "../../third_party/minorGems/util/crc32.h"
#include "../../third_party/minorGems/util/random/JenkinsRandomSource.h"

#include "database/LinearDB.h"

// can replace with frozenTime to freeze time
// or slowTime to slow it down
#define MAP_TIMESEC Time::timeSec()
#define CELL_D 128// cell pixel dimension on client

//!from Speech.cpp TODO: encapsulate this
extern int numSpeechPipes;
extern SimpleVector<GridPos> *speechPipesIn;
extern SimpleVector<GridPos> *speechPipesOut;
extern BlockingCacheRecord blockingCache[];
extern RecentPlacement recentPlacements[];
extern BiomeCacheRecord biomeCache[];
extern DBTimeCacheRecord dbTimeCache[];
extern DBCacheRecord dbCache[];
extern FILE *mapChangeLogFile;
extern double mapChangeLogTimeStart;
extern int currentResponsiblePlayer;
extern int evePrimaryLocObjectID;
extern int evePrimaryLocSpacing;
extern int apocalypsePossible;
extern char apocalypseTriggered;
extern GridPos apocalypseLocation;
extern HashTable<timeSec_t> liveDecayRecordLastLookTimeHashTable;
extern HashTable<double> liveMovementEtaTimes;
extern MinPriorityQueue<MovementRecord> liveMovements;
extern SimpleVector<LiveObject> players;
extern char doesEveLineExist(int inEveID);

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
int eveHomeMarkerObjectID = -1;
int cellsLookedAtToInit = 0;// if lookTimeDBEmpty, then we init all map cell look times to NOW
float *totalChanceWeight;
int randSeed = 124567;
//JenkinsRandomSource randSource( randSeed );
CustomRandomSource randSource(randSeed);
int getBaseMapCallCount = 0;
unsigned int biomeRandSeed = 723;
double recentlyUsedPrimaryEvePositionTimeout = 3600;
GridPos eveStartSpiralPos = {0, 0};
char eveStartSpiralPosSet = false;
double eveAngle = 2 * M_PI;// eves are placed along an Archimedean spiral// we track the angle of the last Eve to compute the position on// the spiral of the next Eve
int maxEveLocationUsage = 3;
int eveLocationUsage = 0;
GridPos lastEvePrimaryLocation = {0, 0};
int maxSecondsForActiveDecayTracking = 900;// decay slots for contained items start after container slots// 15 minutes
CoordinateTimeTracking lookTimeTracking;

// for quick lookup of existing records in liveDecayQueue
// store the eta time here
// before storing a new record in the queue, we can check this hash
// table to see whether it already exists
HashTable<timeSec_t> liveDecayRecordPresentHashTable(1024);

MinPriorityQueue<LiveDecayRecord> liveDecayQueue;

// track max tracked contained for each x,y
// this allows us to update last look times without getting contained count
// from map
// indexed as x, y, 0, 0
static ContRecord defaultContRecord = {0, 0};
HashTable<ContRecord> liveDecayRecordLastLookTimeMaxContainedHashTable(1024, defaultContRecord);

// live players look at their surrounding map region every 5 seconds
// we count a region as stale after no one looks at it for 10 seconds
// (we actually purge the live tracking of that region after 15 seconds).
// This gives us some wiggle room with the timing, so we always make
// sure to re-look at a region (when walking back into it) that is >10
// seconds old, because it may (or may not) have fallen out of our live
// tracking (if our re-look time was 15 seconds to match the time stuff actually
// is dropped from live tracking, we might miss some stuff, depending
// on how the check calls are interleaved time-wise).
int noLookCountAsStaleSeconds = 10;
static int maxSecondsNoLookDecayTracking = 15;// 15 seconds (before no-look regions are purged from live tracking)

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
SimpleVector<GridPos> flightLandingPos;
SimpleVector<ChangePosition> mapChangePosSinceLastStep;// track all map changes that happened since the last// call to stepMap


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

static int mapCacheBitMask = BASE_MAP_CACHE_SIZE - 1;// if BASE_MAP_CACHE_SIZE is a power of 2, then this is the bit mask of solid 1's that can limit an integer to that range
BaseMapCacheRecord baseMapCache[BASE_MAP_CACHE_SIZE][BASE_MAP_CACHE_SIZE];

// 1671 shy of int max
static int xLimit = 2147481977;
static int yLimit = 2147481977;

static char equal(GridPos inA, GridPos inB)
{
	if (inA.x == inB.x && inA.y == inB.y) { return true; }
	return false;
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

/**
 *
 * @param inX
 * @param inY
 * @param inNumContained
 * @param inContained
 * @param inSubCont
 * @note from server/map.cpp
 * // setting negative elements indicates sub containers
 */
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

void reseedMap(char inForceFresh)
{

	FILE *seedFile = NULL;

	if (!inForceFresh) { seedFile = fopen("biomeRandSeed.txt", "r"); }

	if (seedFile != NULL)
	{
		fscanf(seedFile, "%d", &biomeRandSeed);
		fclose(seedFile);
		AppLog::infoF("Reading map rand seed from file: %u\n", biomeRandSeed);
	}
	else
	{
		// no seed set, or ignoring it, make a new one

		if (!inForceFresh)
		{
			// not forced (not internal apocalypse)
			// seed file wiped externally, so it's like a manual apocalypse
			// report a fresh arc starting
			reportArcEnd();
		}

		char *secret = SettingsManager::getStringSetting("statsServerSharedSecret", "secret");

		unsigned int seedBase = crc32((unsigned char *)secret, strlen(secret));

		unsigned int modTimeSeed = (unsigned int)fmod(Time::getCurrentTime() + seedBase, 4294967295U);

		JenkinsRandomSource tempRandSource(modTimeSeed);

		biomeRandSeed = tempRandSource.getRandomInt();

		AppLog::infoF("Generating fresh map rand seed and saving to file: "
					  "%u\n",
					  biomeRandSeed);

		// and save it
		seedFile = fopen("biomeRandSeed.txt", "w");
		if (seedFile != NULL)
		{

			fprintf(seedFile, "%d", biomeRandSeed);
			fclose(seedFile);
		}
	}
}

/**
 * @note from server/map.cpp
 */
void setupMapChangeLogFile()
{
	File logFolder(NULL, "mapChangeLogs");

	if (!logFolder.exists()) { logFolder.makeDirectory(); }

	if (mapChangeLogFile != NULL)
	{
		fclose(mapChangeLogFile);
		mapChangeLogFile = NULL;
	}

	if (logFolder.isDirectory())
	{

		char *biomeSeedString = autoSprintf("%d", biomeRandSeed);

		// does log file already exist?

		int    numFiles;
		File **childFiles = logFolder.getChildFiles(&numFiles);

		for (int i = 0; i < numFiles; i++)
		{
			File *f = childFiles[i];

			char *name = f->getFileName();

			if (strstr(name, biomeSeedString) != NULL)
			{
				// found!
				char *fullFileName = f->getFullFileName();
				mapChangeLogFile   = fopen(fullFileName, "a");
				delete[] fullFileName;
			}
			delete[] name;
			if (mapChangeLogFile != NULL) { break; }
		}
		for (int i = 0; i < numFiles; i++)
		{
			delete childFiles[i];
		}
		delete[] childFiles;

		delete[] biomeSeedString;

		if (mapChangeLogFile == NULL)
		{

			// file does not exist
			char *newFileName = autoSprintf("%.ftime_%useed_mapLog.txt", Time::getCurrentTime(), biomeRandSeed);

			File *f = logFolder.getChildFile(newFileName);

			char *fullName = f->getFullFileName();

			delete f;

			mapChangeLogFile = fopen(fullName, "a");
			delete[] fullName;
		}
	}

	mapChangeLogTimeStart = Time::getCurrentTime();
	fprintf(mapChangeLogFile, "startTime: %.2f\n", mapChangeLogTimeStart);
}

/**
 *
 * @param inX
 * @param inY
 * @param inID
 * @note from server/map.cpp
 */
void setMapObject(int inX, int inY, int inID)
{

	logMapChange(inX, inY, inID);

	setMapObjectRaw(inX, inY, inID);

	// actually need to set decay here
	// otherwise, if we wait until getObject, it will assume that
	// this is a never-before-seen object and randomize the decay.
	TransRecord *newDecayT = getMetaTrans(-1, inID);

	timeSec_t mapETA = 0;

	if (newDecayT != NULL && newDecayT->autoDecaySeconds > 0) { mapETA = MAP_TIMESEC + newDecayT->autoDecaySeconds; }

	setEtaDecay(inX, inY, mapETA);

	// note that we also potentially set up decay for objects on get
	// if they have no decay set already
	// We do this because there are loads
	// of gets that have no set (for example, getting a map chunk)
	// Those decay times get randomized to avoid lock-step in newly-seen
	// objects

	if (inID > 0)
	{

		char found = false;

		for (int i = 0; i < NUM_RECENT_PLACEMENTS; i++)
		{

			if (inX == recentPlacements[i].pos.x && inY == recentPlacements[i].pos.y)
			{

				found = true;
				// update depth
				int newDepth = getObjectDepth(inID);

				if (newDepth != UNREACHABLE) { recentPlacements[i].depth = getObjectDepth(inID); }
				break;
			}
		}

		if (!found)
		{

			int newDepth = getObjectDepth(inID);
			if (newDepth != UNREACHABLE)
			{

				recentPlacements[nextPlacementIndex].pos.x = inX;
				recentPlacements[nextPlacementIndex].pos.y = inY;
				recentPlacements[nextPlacementIndex].depth = newDepth;

				nextPlacementIndex++;

				if (nextPlacementIndex >= NUM_RECENT_PLACEMENTS)
				{
					nextPlacementIndex = 0;

					// write again every time we have a fresh 100
					writeRecentPlacements();
				}
			}
		}
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inID
 * @note from server/map.cpp
 */
void setMapObjectRaw(int inX, int inY, int inID)
{
	dbPut(inX, inY, 0, inID);

	// global trigger and speech pipe stuff

	if (inID <= 0) { return; }

	ObjectRecord *o = getObject(inID);

	if (o == NULL) { return; }

	if (o->isFlightLanding)
	{
		GridPos p = {inX, inY};

		char found = false;

		for (int i = 0; i < flightLandingPos.size(); i++)
		{
			GridPos otherP = flightLandingPos.getElementDirect(i);

			if (equal(p, otherP))
			{

				// make sure this other strip really still exists
				int oID = getMapObject(otherP.x, otherP.y);

				if (oID <= 0 || !getObject(oID)->isFlightLanding)
				{

					// not even a valid landing pos anymore
					flightLandingPos.deleteElement(i);
					i--;
				}
				else
				{
					found = true;
					break;
				}
			}
		}

		if (!found) { flightLandingPos.push_back(p); }
	}

	if (o->speechPipeIn)
	{
		GridPos p = {inX, inY};

		int foundIndex = findGridPos(&(speechPipesIn[o->speechPipeIndex]), p);

		if (foundIndex == -1) { speechPipesIn[o->speechPipeIndex].push_back(p); }
	}
	else if (o->speechPipeOut)
	{
		GridPos p = {inX, inY};

		int foundIndex = findGridPos(&(speechPipesOut[o->speechPipeIndex]), p);

		if (foundIndex == -1) { speechPipesOut[o->speechPipeIndex].push_back(p); }
	}
	else if (o->isGlobalTriggerOn)
	{
		GlobalTriggerState *s = globalTriggerStates.getElement(o->globalTriggerIndex);

		GridPos p = {inX, inY};

		int foundIndex = findGridPos(&(s->triggerOnLocations), p);

		if (foundIndex == -1)
		{
			s->triggerOnLocations.push_back(p);

			if (s->triggerOnLocations.size() == 1)
			{
				// just turned on globally

				/// process all receivers
				for (int i = 0; i < s->receiverLocations.size(); i++)
				{
					GridPos q = s->receiverLocations.getElementDirect(i);

					int id = getMapObjectRaw(q.x, q.y);

					if (id <= 0)
					{
						// receiver no longer here
						s->receiverLocations.deleteElement(i);
						i--;
						continue;
					}

					ObjectRecord *oR = getObject(id);

					if (oR->isGlobalReceiver && oR->globalTriggerIndex == o->globalTriggerIndex)
					{
						// match

						int metaID = getMetaTriggerObject(o->globalTriggerIndex);

						if (metaID > 0)
						{
							TransRecord *tR = getPTrans(metaID, id);

							if (tR != NULL)
							{

								dbPut(q.x, q.y, 0, tR->newTarget);

								// save this to our "triggered" list
								int foundIndex = findGridPos(&(s->triggeredLocations), q);

								if (foundIndex != -1)
								{
									// already exists
									// replace
									*(s->triggeredIDs.getElement(foundIndex))       = tR->newTarget;
									*(s->triggeredRevertIDs.getElement(foundIndex)) = tR->target;
								}
								else
								{
									s->triggeredLocations.push_back(q);
									s->triggeredIDs.push_back(tR->newTarget);
									s->triggeredRevertIDs.push_back(tR->target);
								}
							}
						}
					}
					// receiver no longer here
					// (either wasn't here anymore for other reasons,
					//  or we just changed it into its triggered state)
					// remove it
					s->receiverLocations.deleteElement(i);
					i--;
				}
			}
		}
	}
	else if (o->isGlobalTriggerOff)
	{
		GlobalTriggerState *s = globalTriggerStates.getElement(o->globalTriggerIndex);

		GridPos p = {inX, inY};

		int foundIndex = findGridPos(&(s->triggerOnLocations), p);

		if (foundIndex != -1)
		{
			s->triggerOnLocations.deleteElement(foundIndex);

			if (s->triggerOnLocations.size() == 0)
			{
				// just turned off globally, no on triggers left on map

				/// process all triggered elements back to off

				for (int i = 0; i < s->triggeredLocations.size(); i++)
				{
					GridPos q = s->triggeredLocations.getElementDirect(i);

					int curID = getMapObjectRaw(q.x, q.y);

					int triggeredID = s->triggeredIDs.getElementDirect(i);

					if (curID == triggeredID)
					{
						// cell still in triggered state

						// revert it
						int revertID = s->triggeredRevertIDs.getElementDirect(i);

						dbPut(q.x, q.y, 0, revertID);

						// no longer triggered, remove it
						s->triggeredLocations.deleteElement(i);
						s->triggeredIDs.deleteElement(i);
						s->triggeredRevertIDs.deleteElement(i);
						i--;

						// remember it as a reciever (it has gone back
						// to being a receiver again)
						s->receiverLocations.push_back(q);
					}
				}
			}
		}
	}
	else if (o->isGlobalReceiver)
	{
		GlobalTriggerState *s = globalTriggerStates.getElement(o->globalTriggerIndex);

		GridPos p = {inX, inY};

		int foundIndex = findGridPos(&(s->receiverLocations), p);

		if (foundIndex == -1) { s->receiverLocations.push_back(p); }

		if (s->triggerOnLocations.size() > 0)
		{
			// this receiver is currently triggered

			// trigger it now, right away, as soon as it is placed on map

			int metaID = getMetaTriggerObject(o->globalTriggerIndex);

			if (metaID > 0)
			{
				TransRecord *tR = getPTrans(metaID, inID);

				if (tR != NULL)
				{

					dbPut(inX, inY, 0, tR->newTarget);

					GridPos q = {inX, inY};

					// save this to our "triggered" list
					int foundIndex = findGridPos(&(s->triggeredLocations), q);

					if (foundIndex != -1)
					{
						// already exists
						// replace
						*(s->triggeredIDs.getElement(foundIndex))       = tR->newTarget;
						*(s->triggeredRevertIDs.getElement(foundIndex)) = tR->target;
					}
					else
					{
						s->triggeredLocations.push_back(q);
						s->triggeredIDs.push_back(tR->newTarget);
						s->triggeredRevertIDs.push_back(tR->target);
					}
				}
			}
		}
	}
	else if (o->isTapOutTrigger)
	{
		// this object, when created, taps out other objects in grid around

		char playerHasPrimaryHomeland = false;

		if (currentResponsiblePlayer != -1)
		{
			int pID = currentResponsiblePlayer;
			if (pID < 0) { pID = -pID; }
			// primaryHomeland is not in 2HOL
			// int lineage = getPlayerLineage( pID );

			// if( lineage != -1 ) {
			// playerHasPrimaryHomeland = hasPrimaryHomeland( lineage );
			// }
		}

		// don't make current player responsible for all these changes
		int restoreResponsiblePlayer = currentResponsiblePlayer;
		currentResponsiblePlayer     = -1;

		TapoutRecord *r = getTapoutRecord(inID);

		if (r != NULL)
		{

			// char tappedOutPrimaryHomeland = false; //primaryHomeland is not in 2HOL

			// tappedOutPrimaryHomeland =
			runTapoutOperation(
					inX, inY, r->limitX, r->limitY, r->gridSpacingX, r->gridSpacingY, inID, playerHasPrimaryHomeland);

			r->buildCount++;

			if (r->buildCountLimit != -1 && r->buildCount >= r->buildCountLimit)
			{
				// hit limit!
				// tapout a larger radius now
				// tappedOutPrimaryHomeland =
				runTapoutOperation(inX,
								   inY,
								   r->postBuildLimitX,
								   r->postBuildLimitY,
								   r->gridSpacingX,
								   r->gridSpacingY,
								   inID,
								   playerHasPrimaryHomeland,
								   true);
			}
		}

		currentResponsiblePlayer = restoreResponsiblePlayer;
	}
}

/**
 *
 * @param inList
 * @param inP
 * @return
 * @note from server/map.cpp
 */
int findGridPos(SimpleVector<GridPos> *inList, GridPos inP)
{
	for (int i = 0; i < inList->size(); i++)
	{
		GridPos q = inList->getElementDirect(i);
		if (equal(inP, q)) { return i; }
	}
	return -1;
}

/**
 *
 * @param inX
 * @param inY
 * @param inRadiusX
 * @param inRadiusY
 * @param inSpacingX
 * @param inSpacingY
 * @param inTriggerID
 * @param inPlayerHasPrimaryHomeland
 * @param inIsPost
 * @return
 * @note // returns true if tapout-triggered a +primaryHomeland object
 */
char runTapoutOperation(
		int inX,
		int                            inY,
							   int                            inRadiusX,
							   int                            inRadiusY,
							   int                            inSpacingX,
							   int                            inSpacingY,
							   int                            inTriggerID,
							   char                           inPlayerHasPrimaryHomeland,
							   char                           inIsPost)
{

	char returnVal = false;

	for (int y = inY - inRadiusY; y <= inY + inRadiusY; y += inSpacingY)
	{

		for (int x = inX - inRadiusX; x <= inX + inRadiusX; x += inSpacingX)
		{

			if (inX == x && inY == y)
			{
				// skip center
				continue;
			}

			int id = getMapObjectRaw(x, y);

			// change triggered by tapout represented by
			// tapoutTrigger object getting used as actor
			// on tapoutTarget
			TransRecord *t = NULL;

			int newTarget = -1;

			if (!inIsPost)
			{
				// last use target signifies what happens in
				// same row or column as inX, inY

				// get eastward
				t = getPTrans(inTriggerID, id, false, true);

				if (t != NULL) { newTarget = t->newTarget; }

				if (newTarget > 0) { newTarget = applyTapoutGradientRotate(inX, inY, x, y, newTarget); }
			}

			if (newTarget == -1)
			{
				// not same row or post or last-use-target trans undefined
				t = getPTrans(inTriggerID, id);

				if (t != NULL) { newTarget = t->newTarget; }
			}

			if (newTarget != -1)
			{
				ObjectRecord *nt = getObject(newTarget);

				if (strstr(nt->description, "+primaryHomeland") != NULL)
				{
					if (inPlayerHasPrimaryHomeland)
					{
						// block creation of objects that require
						// +primaryHomeland
						// player already has a primary homeland

						newTarget = -1;
					}
					else
					{
						// created a +primaryHomeland object
						returnVal = true;
					}
				}
			}

			if (newTarget != -1)
			{
				setMapObjectRaw(x, y, newTarget);

				TransRecord *newDecayT = getMetaTrans(-1, newTarget);

				timeSec_t mapETA = 0;

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

				setEtaDecay(x, y, mapETA, newDecayT);
			}
		}
	}

	return returnVal;
}

/**
 *
 * @param inX
 * @param inY
 * @param inTargetX
 * @param inTargetY
 * @param inEastwardGradientID
 * @return
 */
int applyTapoutGradientRotate(int inX, int inY, int inTargetX, int inTargetY, int inEastwardGradientID)
{
	// apply result to itself to flip it
	// and point gradient in other direction

	// eastward + eastward = westward, etc

	// order:  e, w, s, n, ne, se, sw, nw

	int numRepeat = 0;

	int curObjectID = inEastwardGradientID;

	if (inX > inTargetX && inY == inTargetY) { numRepeat = 0; }
	else if (inX < inTargetX && inY == inTargetY)
	{
		numRepeat = 1;
	}
	else if (inX == inTargetX && inY < inTargetY)
	{
		numRepeat = 2;
	}
	else if (inX == inTargetX && inY > inTargetY)
	{
		numRepeat = 3;
	}
	else if (inX > inTargetX && inY > inTargetY)
	{
		numRepeat = 4;
	}
	else if (inX > inTargetX && inY < inTargetY)
	{
		numRepeat = 5;
	}
	else if (inX < inTargetX && inY < inTargetY)
	{
		numRepeat = 6;
	}
	else if (inX < inTargetX && inY > inTargetY)
	{
		numRepeat = 7;
	}

	for (int i = 0; i < numRepeat; i++)
	{
		if (curObjectID == 0) { break; }
		TransRecord *flipTrans = getPTrans(curObjectID, curObjectID);

		if (flipTrans != NULL) { curObjectID = flipTrans->newTarget; }
	}

	if (curObjectID == 0) { return -1; }

	return curObjectID;
}

void removeLandingPos(GridPos inPos)
{
	for (int i = 0; i < flightLandingPos.size(); i++)
	{
		if (equal(inPos, flightLandingPos.getElementDirect(i)))
		{
			flightLandingPos.deleteElement(i);
			return;
		}
	}
}

/**
 *
 * @param inEmail
 * @param inID
 * @param outX
 * @param outY
 * @param inOtherPeoplePos
 * @param inAllowRespawn
 * // gets new Eve position on outskirts of civilization
// if inAllowRespawn, this player's last Eve old-age-death will be
// considered.
 */
void getEvePosition(const char *inEmail,
					int                         inID,
					int *                       outX,
					int *                       outY,
					SimpleVector<GridPos> *     inOtherPeoplePos,
					char                        inAllowRespawn)
{

	int currentEveRadius = eveRadius;

	char forceEveToBorder = false;

	doublePair ave = {0, 0};

	printf("Placing new Eve:  ");

	int pX, pY, pR;

	int result = eveDBGet(inEmail, &pX, &pY, &pR);

	if (inAllowRespawn && result == 1 && pR > 0)
	{
		printf("Found camp center (%d,%d) r=%d in db for %s\n", pX, pY, pR, inEmail);

		ave.x            = pX;
		ave.y            = pY;
		currentEveRadius = pR;
	}
	else
	{
		// player has never been an Eve that survived to old age before
		// or such repawning forbidden by caller

		maxEveLocationUsage = SettingsManager::getIntSetting("maxEveStartupLocationUsage", 10);

		// first try new grid placement method

		// actually skip this for now and go back to normal Eve spiral
		if (false)
			if (eveLocationUsage >= maxEveLocationUsage && evePrimaryLocObjectID > 0)
			{

				GridPos centerP = lastEvePrimaryLocation;

				if (inOtherPeoplePos->size() > 0)
				{

					centerP = inOtherPeoplePos->getElementDirect(
							randSource.getRandomBoundedInt(0, inOtherPeoplePos->size() - 1));

					// round to nearest whole spacing multiple
					centerP.x /= evePrimaryLocSpacing;
					centerP.y /= evePrimaryLocSpacing;

					centerP.x *= evePrimaryLocSpacing;
					centerP.y *= evePrimaryLocSpacing;
				}

				GridPos tryP   = centerP;
				char    found  = false;
				GridPos foundP = tryP;

				double curTime = Time::getCurrentTime();

				int r;

				int maxSearchRadius = 10;

				// first, clean any that have timed out
				// or gone extinct
				for (int p = 0; p < recentlyUsedPrimaryEvePositions.size(); p++)
				{

					char reusePos = false;

					if (curTime - recentlyUsedPrimaryEvePositionTimes.getElementDirect(p)
						> recentlyUsedPrimaryEvePositionTimeout)
					{
						// timed out
						reusePos = true;
					}
					else if (!doesEveLineExist(recentlyUsedPrimaryEvePositionPlayerIDs.getElementDirect(p)))
					{
						// eve line extinct
						reusePos = true;
					}

					if (reusePos)
					{
						recentlyUsedPrimaryEvePositions.deleteElement(p);
						recentlyUsedPrimaryEvePositionTimes.deleteElement(p);
						recentlyUsedPrimaryEvePositionPlayerIDs.deleteElement(p);
						p--;
					}
				}

				for (r = 1; r < maxSearchRadius; r++)
				{

					for (int y = -r; y <= r; y++)
					{
						for (int x = -r; x <= r; x++)
						{
							tryP = centerP;

							tryP.x += x * evePrimaryLocSpacing;
							tryP.y += y * evePrimaryLocSpacing;

							char existsAlready = false;

							for (int p = 0; p < recentlyUsedPrimaryEvePositions.size(); p++)
							{

								GridPos pos = recentlyUsedPrimaryEvePositions.getElementDirect(p);

								if (equal(pos, tryP))
								{
									existsAlready = true;
									break;
								}
							}

							if (existsAlready) { continue; }
							else
							{
							}

							int mapID = getMapObject(tryP.x, tryP.y);

							if (mapID == evePrimaryLocObjectID)
							{
								printf("Found primary Eve object at %d,%d\n", tryP.x, tryP.y);
								found  = true;
								foundP = tryP;
							}
							else if (eveSecondaryLocObjectIDs.getElementIndex(mapID) != -1)
							{
								// a secondary ID, allowed
								printf("Found secondary Eve object at %d,%d\n", tryP.x, tryP.y);
								found  = true;
								foundP = tryP;
							}
						}

						if (found) break;
					}
					if (found) break;
				}

				if (found)
				{

					if (r >= maxSearchRadius / 2)
					{
						// exhausted central window around last eve center
						// save this as the new eve center
						// next time, we'll search a window around that

						AppLog::infoF("Eve pos %d,%d not in center of "
									  "grid window, recentering window for "
									  "next time",
									  foundP.x,
									  foundP.y);

						lastEvePrimaryLocation = foundP;
					}

					AppLog::infoF("Sticking Eve at unused primary grid pos "
								  "of %d,%d\n",
								  foundP.x,
								  foundP.y);

					recentlyUsedPrimaryEvePositions.push_back(foundP);
					recentlyUsedPrimaryEvePositionTimes.push_back(curTime);
					recentlyUsedPrimaryEvePositionPlayerIDs.push_back(inID);

					// stick Eve directly to south
					*outX = foundP.x;
					*outY = foundP.y - 1;

					if (eveHomeMarkerObjectID > 0)
					{
						// stick home marker there
						setMapObject(*outX, *outY, eveHomeMarkerObjectID);
					}
					else
					{
						// make it empty
						setMapObject(*outX, *outY, 0);
					}

					// clear a few more objects to the south, to make
					// sure Eve's spring doesn't spawn behind a tree
					setMapObject(*outX, *outY - 1, 0);
					setMapObject(*outX, *outY - 2, 0);
					setMapObject(*outX, *outY - 3, 0);

					// finally, prevent Eve entrapment by sticking
					// her at a random location around the spring

					doublePair v = {14, 0};
					v            = rotate(v, randSource.getRandomBoundedInt(0, 2 * M_PI));

					*outX += v.x;
					*outY += v.y;

					return;
				}
				else
				{
					AppLog::info("Unable to find location for Eve "
								 "on primary grid.");
				}
			}

		// Spiral method:
		GridPos eveLocToUse = eveLocation;

		int jumpUsed = 0;

		if (eveLocationUsage < maxEveLocationUsage)
		{
			eveLocationUsage++;
			// keep using same location

			printf("Reusing same eve start-up location "
				   "of %d,%d for %dth time\n",
				   eveLocation.x,
				   eveLocation.y,
				   eveLocationUsage);

			// remember it for when we exhaust it
			if (evePrimaryLocObjectID > 0 && evePrimaryLocSpacing > 0)
			{

				lastEvePrimaryLocation = eveLocation;
				// round to nearest whole spacing multiple
				lastEvePrimaryLocation.x /= evePrimaryLocSpacing;
				lastEvePrimaryLocation.y /= evePrimaryLocSpacing;

				lastEvePrimaryLocation.x *= evePrimaryLocSpacing;
				lastEvePrimaryLocation.y *= evePrimaryLocSpacing;

				printf("Saving eve start-up location close grid pos "
					   "of %d,%d for later\n",
					   lastEvePrimaryLocation.x,
					   lastEvePrimaryLocation.y);
			}
		}
		else
		{
			// post-startup eve location has been used too many times
			// place eves on spiral instead

			if (abs(eveLocToUse.x) > 100000 || abs(eveLocToUse.y) > 100000)
			{
				// we've gotten to far away from center over time

				// re-center spiral on center to rein things in

				// we'll end up saving a position on the arm of this new
				// centered spiral for future start-ups, so the eve
				// location can move out from here
				eveLocToUse.x = 0;
				eveLocToUse.y = 0;

				eveStartSpiralPosSet = false;
			}

			if (eveStartSpiralPosSet && longTermCullEnabled)
			{

				int longTermCullingSeconds = SettingsManager::getIntSetting("longTermNoLookCullSeconds", 3600 * 12);

				// see how long center has not been seen
				// if it's old enough, we can reset Eve angle and restart
				// spiral there again
				// this will bring Eves closer together again, after
				// rim of spiral gets too far away

				timeSec_t lastLookTime = dbLookTimeGet(eveStartSpiralPos.x, eveStartSpiralPos.y);

				if (Time::getCurrentTime() - lastLookTime > longTermCullingSeconds * 2)
				{
					// double cull start time
					// that should be enough for the center to actually have
					// started getting culled, and then some

					// restart the spiral
					eveAngle    = 2 * M_PI;
					eveLocToUse = eveLocation;

					eveStartSpiralPosSet = false;
				}
			}

			int jump = SettingsManager::getIntSetting("nextEveJump", 2000);
			jumpUsed = jump;

			// advance eve angle along spiral
			// approximate recursive form
			eveAngle = eveAngle + (2 * M_PI) / eveAngle;

			// exact formula for radius along spiral from angle
			double radius = (jump * eveAngle) / (2 * M_PI);

			doublePair delta = {radius, 0};
			delta            = rotate(delta, eveAngle);

			// but don't update the post-startup location
			// keep jumping away from startup-location as center of spiral
			eveLocToUse.x += lrint(delta.x);
			eveLocToUse.y += lrint(delta.y);

			if (barrierOn &&
				// we use jumpUsed / 3 as randomizing radius below
				// so jumpUsed / 2 is safe here
				(abs(eveLocToUse.x) > barrierRadius - jumpUsed / 2
				 || abs(eveLocToUse.y) > barrierRadius - jumpUsed / 2))
			{

				// Eve has gotten too close to the barrier

				// hard reset of location back to (0,0)-centered spiral
				eveAngle = 2 * M_PI;

				eveLocation.x = 0;
				eveLocation.y = 0;
				eveLocToUse   = eveLocation;

				eveStartSpiralPosSet = false;
			}

			// but do save it as a possible post-startup location for next time
			File  eveLocFile(NULL, "lastEveLocation.txt");
			char *locString = autoSprintf("%d,%d", eveLocToUse.x, eveLocToUse.y);
			eveLocFile.writeToFile(locString);
			delete[] locString;
		}

		ave.x = eveLocToUse.x;
		ave.y = eveLocToUse.y;

		// put Eve in radius 50 around this location
		forceEveToBorder = true;
		currentEveRadius = 50;

		if (jumpUsed > 3 && currentEveRadius > jumpUsed / 3) { currentEveRadius = jumpUsed / 3; }
	}

	// pick point in box according to eve radius

	char found = 0;

	if (currentEveRadius < 1) { currentEveRadius = 1; }

	while (!found)
	{
		printf("Placing new Eve:  "
			   "trying radius of %d from camp\n",
			   currentEveRadius);

		int tryCount = 0;

		while (!found && tryCount < 100)
		{

			doublePair p = {randSource.getRandomBoundedDouble(-currentEveRadius, +currentEveRadius),
							randSource.getRandomBoundedDouble(-currentEveRadius, +currentEveRadius)};

			if (forceEveToBorder)
			{
				// or pick ap point on the circle instead
				p.x = currentEveRadius;
				p.y = 0;

				double a = randSource.getRandomBoundedDouble(0, 2 * M_PI);
				p        = rotate(p, a);
			}

			p = add(p, ave);

			GridPos pInt = {(int)lrint(p.x), (int)lrint(p.y)};

			if (getMapObjectRaw(pInt.x, pInt.y) == 0)
			{

				*outX = pInt.x;
				*outY = pInt.y;

				if (!eveStartSpiralPosSet)
				{
					eveStartSpiralPos    = pInt;
					eveStartSpiralPosSet = true;
				}

				found = true;
			}

			tryCount++;
		}

		// tried too many times, expand radius
		currentEveRadius *= 2;
	}

	// clear recent placements after placing a new Eve
	// let her make new placements in her life which we will remember
	// later

	clearRecentPlacements();
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlotNumber
 * @param inNewObjectID
 * @note from server/server.cpp => server/main.cpp
 */
void changeContained(int inX, int inY, int inSlotNumber, int inNewObjectID ) {

	int numContained = 0;
	int *contained = getContained( inX, inY, &numContained );

	timeSec_t *containedETA =
			getContainedEtaDecay( inX, inY, &numContained );

	timeSec_t curTimeSec = Time::timeSec();

	if( contained != NULL && containedETA != NULL &&
		numContained > inSlotNumber ) {

		int oldObjectID = contained[ inSlotNumber ];
		timeSec_t oldETA = containedETA[ inSlotNumber ];

		if( oldObjectID > 0 ) {

			TransRecord *oldDecayTrans = getTrans( -1, oldObjectID );

			TransRecord *newDecayTrans = getTrans( -1, inNewObjectID );


			timeSec_t newETA = 0;

			if( newDecayTrans != NULL ) {
				newETA = curTimeSec + newDecayTrans->autoDecaySeconds;
			}

			if( oldDecayTrans != NULL && newDecayTrans != NULL &&
				oldDecayTrans->autoDecaySeconds ==
				newDecayTrans->autoDecaySeconds ) {
				// preserve remaining seconds from old object
				newETA = oldETA;
			}

			contained[ inSlotNumber ] = inNewObjectID;
			containedETA[ inSlotNumber ] = newETA;

			setContained( inX, inY, numContained, contained );
			setContainedEtaDecay( inX, inY, numContained, containedETA );
		}
	}

	if( contained != NULL ) {
		delete [] contained;
	}
	if( containedETA != NULL ) {
		delete [] containedETA;
	}
}


/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @param inID
 * @note from server/map.cpp
 */
inline void changeContained(int inX, int inY, int inSlot, int inSubCont, int inID)
{
	dbPut(inX, inY, FIRST_CONT_SLOT + inSlot, inID, inSubCont);
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inValue
 * @param inSubCont
 * @note from server/map.cpp
 */
void dbPut(int inX, int inY, int inSlot, int inValue, int inSubCont)
{

	if (inSlot == 0 && inSubCont == 0)
	{
		// object has changed
		// clear blocking cache
		blockingClearCached(inX, inY);
	}

	if (!skipTrackingMapChanges)
	{

		// count all slot changes as changes, because we're storing
		// time in a separate database now (so we don't need to worry
		// about time changes being reported as map changes)

		char found = false;
		for (int i = 0; i < mapChangePosSinceLastStep.size(); i++)
		{

			ChangePosition *p = mapChangePosSinceLastStep.getElement(i);

			if (p->x == inX && p->y == inY)
			{
				found = true;

				// update it
				p->responsiblePlayerID = currentResponsiblePlayer;
				break;
			}
		}

		if (!found)
		{
			ChangePosition p = {inX, inY, false, currentResponsiblePlayer, 0, 0, 0.0};
			mapChangePosSinceLastStep.push_back(p);
		}
	}

	if (apocalypsePossible && inValue > 0 && inSlot == 0 && inSubCont == 0)
	{
		// a primary tile put
		// check if this triggers the apocalypse
		if (isApocalypseTrigger(inValue))
		{
			apocalypseTriggered  = true;
			apocalypseLocation.x = inX;
			apocalypseLocation.y = inY;
		}
	}
	if (inValue > 0 && inSlot == 0 && inSubCont == 0)
	{

		int status = getMonumentStatus(inValue);

		if (status > 0)
		{
			int player = currentResponsiblePlayer;
			if (player < 0) { player = -player; }
			monumentAction(inX, inY, inValue, player, status);
		}
	}

	unsigned char key[16];
	unsigned char value[4];

	intQuadToKey(inX, inY, inSlot, inSubCont, key);
	intToValue(inValue, value);

	DB_put(&db, key, value);

	dbPutCached(inX, inY, inSlot, inSubCont, inValue);
}

/**
 *
 * @param db
 * @param path
 * @param mode
 * @param hash_table_size
 * @param key_size
 * @param value_size
 * @return
 * @note from server/map.cpp
 * // version of open call that checks whether look time exists in lookTimeDB
// for each record in opened DB, and clears any entries that are not
// rebuilding file storage for DB in the process
// lookTimeDB MUST be open before calling this
//
// If lookTimeDBEmpty, this call just opens the target DB normally without
// shrinking it.
//
// Can handle max key and value size of 16 and 12 bytes
// Assumes that first 8 bytes of key are xy as 32-bit ints
 */
int DB_open_timeShrunk(DB *db,
					   const char *           path,
					   int                    mode,
					   unsigned long          hash_table_size,
					   unsigned long          key_size,
					   unsigned long          value_size)
{

	File dbFile(NULL, path);

	if (!dbFile.exists() || lookTimeDBEmpty || skipLookTimeCleanup)
	{

		if (lookTimeDBEmpty) { AppLog::infoF("No lookTimes present, not cleaning %s", path); }

		int error = DB_open(db, path, mode, hash_table_size, key_size, value_size);

		if (!error && !skipLookTimeCleanup)
		{
			// add look time for cells in this DB to present
			// essentially resetting all look times to NOW

			DB_Iterator dbi;

			DB_Iterator_init(db, &dbi);

			// key and value size that are big enough to handle all of our DB
			unsigned char key[16];

			unsigned char value[12];

			while (DB_Iterator_next(&dbi, key, value) > 0)
			{
				int x = valueToInt(key);
				int y = valueToInt(&(key[4]));

				cellsLookedAtToInit++;

				dbLookTimePut(x, y, MAP_TIMESEC);
			}
		}
		return error;
	}

	char *dbTempName = autoSprintf("%s.temp", path);
	File  dbTempFile(NULL, dbTempName);

	if (dbTempFile.exists()) { dbTempFile.remove(); }

	if (dbTempFile.exists())
	{
		AppLog::errorF("Failed to remove temp DB file %s", dbTempName);

		delete[] dbTempName;

		return DB_open(db, path, mode, hash_table_size, key_size, value_size);
	}

	DB oldDB;

	int error = DB_open(&oldDB, path, mode, hash_table_size, key_size, value_size);
	if (error)
	{
		AppLog::errorF("Failed to open DB file %s in DB_open_timeShrunk", path);
		delete[] dbTempName;

		return error;
	}

	DB_Iterator dbi;

	DB_Iterator_init(&oldDB, &dbi);

	// key and value size that are big enough to handle all of our DB
	unsigned char key[16];

	unsigned char value[12];

	int total    = 0;
	int stale    = 0;
	int nonStale = 0;

	// first, just count
	while (DB_Iterator_next(&dbi, key, value) > 0)
	{
		total++;

		int x = valueToInt(key);
		int y = valueToInt(&(key[4]));

		if (dbLookTimeGet(x, y) > 0)
		{
			// keep
			nonStale++;
		}
		else
		{
			// stale
			// ignore
			stale++;
		}
	}

	// optimial size for DB of remaining elements
	unsigned int newSize = DB_getShrinkSize(&oldDB, nonStale);

	AppLog::infoF("Shrinking hash table in %s from %d down to %d", path, DB_getCurrentSize(&oldDB), newSize);

	DB tempDB;

	error = DB_open(&tempDB, dbTempName, mode, newSize, key_size, value_size);
	if (error)
	{
		AppLog::errorF("Failed to open DB file %s in DB_open_timeShrunk", dbTempName);
		delete[] dbTempName;
		DB_close(&oldDB);
		return error;
	}

	// now that we have new temp db properly sized,
	// iterate again and insert, but don't count
	DB_Iterator_init(&oldDB, &dbi);

	while (DB_Iterator_next(&dbi, key, value) > 0)
	{
		int x = valueToInt(key);
		int y = valueToInt(&(key[4]));

		if (dbLookTimeGet(x, y) > 0)
		{
			// keep
			// insert it in temp
			DB_put_new(&tempDB, key, value);
		}
		else
		{
			// stale
			// ignore
		}
	}

	AppLog::infoF("Cleaned %d / %d stale map cells from %s", stale, total, path);

	printf("\n");

	DB_close(&tempDB);
	DB_close(&oldDB);

	dbTempFile.copy(&dbFile);
	dbTempFile.remove();

	delete[] dbTempName;

	// now open new, shrunk file
	return DB_open(db, path, mode, hash_table_size, key_size, value_size);
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 */
// unlike normal objects, there is no live tracking of floors and when
// they will decay in stepMap
// Decay of floors only applied on next call to getMapFloor
// Thus, short-term floor decay isn't supported (e.g., burning floor that
// finishes burning while player still has it on the screen).
int getMapFloor(int inX, int inY)
{
	int id = dbFloorGet(inX, inY);

	if (id <= 0) { return 0; }

	TransRecord *t = getPTrans(-1, id);

	if (t == NULL)
	{
		// no auto-decay for this floor
		return id;
	}

	timeSec_t etaTime = getFloorEtaDecay(inX, inY);

	timeSec_t curTime = MAP_TIMESEC;

	if (etaTime == 0)
	{
		// not set
		// start decay now for future

		setFloorEtaDecay(inX, inY, curTime + t->autoDecaySeconds);

		return id;
	}

	if (etaTime > curTime) { return id; }

	// else eta expired, apply decay

	int newID = t->newTarget;

	setMapFloor(inX, inY, newID);

	return newID;
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 */
int dbFloorGet(int inX, int inY)
{
	unsigned char key[9];
	unsigned char value[4];

	// look for changes to default in database
	intPairToKey(inX, inY, key);

	int result = DB_get(&floorDB, key, value);

	if (result == 0)
	{
		// found
		int returnVal = valueToInt(value);

		return returnVal;
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
 * @param outNumContained
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 */
int *getContained(int inX, int inY, int *outNumContained, int inSubCont)
{
	if (!getSlotItemsNoDecay(inX, inY, inSubCont)) { checkDecayContained(inX, inY, inSubCont); }
	int *result = getContainedRaw(inX, inY, outNumContained, inSubCont);

	// look at these slots if they are subject to live decay
	timeSec_t currentTime = MAP_TIMESEC;

	for (int i = 0; i < *outNumContained; i++)
	{
		timeSec_t *oldLookTime = liveDecayRecordLastLookTimeHashTable.lookupPointer(inX, inY, i + 1, inSubCont);
		if (oldLookTime != NULL)
		{
			// look at it now
			*oldLookTime = currentTime;
		}
	}

	return result;
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
 * @param inSubCont
 * @note from server/map.cpp
 */
void checkDecayContained(int inX, int inY, int inSubCont)
{

	if (getNumContained(inX, inY, inSubCont) == 0) { return; }

	int  numContained;
	int *contained = getContainedRaw(inX, inY, &numContained, inSubCont);

	SimpleVector<int>       newContained;
	SimpleVector<timeSec_t> newDecayEta;

	SimpleVector<SimpleVector<int>>       newSubCont;
	SimpleVector<SimpleVector<timeSec_t>> newSubContDecay;

	char change = false;

	// track last ID we saw with no decay, so we don't have to keep
	// looking it up over and over.
	int lastIDWithNoDecay = 0;

	for (int i = 0; i < numContained; i++)
	{
		int oldID = contained[i];

		if (oldID == lastIDWithNoDecay)
		{
			// same ID we've already seen before
			newContained.push_back(oldID);
			newDecayEta.push_back(0);
			continue;
		}

		char isSubCont = false;

		if (oldID < 0)
		{
			// negative ID means this is a sub-container
			isSubCont = true;
			oldID *= -1;
		}

		TransRecord *t = getPTrans(-1, oldID);

		if (t == NULL)
		{
			// no auto-decay for this object
			if (isSubCont) { oldID *= -1; }
			lastIDWithNoDecay = oldID;

			newContained.push_back(oldID);
			newDecayEta.push_back(0);
			continue;
		}

		// else decay exists for this object

		int newID = oldID;

		// is eta stored in map?
		timeSec_t mapETA = getSlotEtaDecay(inX, inY, i, inSubCont);

		if (mapETA != 0)
		{

			if ((int)mapETA <= MAP_TIMESEC)
			{

				// object in container slot has decayed (eta expired)

				// apply the transition
				newID = t->newTarget;

				if (newID != oldID) { change = true; }

				if (newID != 0)
				{

					TransRecord *newDecayT = getMetaTrans(-1, newID);

					if (newDecayT != NULL)
					{

						// add some random variation to avoid lock-step
						// especially after a server restart
						int tweakedSeconds = randSource.getRandomBoundedInt(
								lrint(newDecayT->autoDecaySeconds * 0.9), newDecayT->autoDecaySeconds);

						if (tweakedSeconds < 1) { tweakedSeconds = 1; }

						mapETA = MAP_TIMESEC + tweakedSeconds / getMapContainerTimeStretch(inX, inY, inSubCont);
					}
					else
					{
						// no further decay
						mapETA = 0;
					}
				}
			}
		}

		if (newID != 0)
		{
			if (isSubCont)
			{

				int oldSlots = getNumContainerSlots(oldID);

				int newSlots = getNumContainerSlots(newID);

				if (newSlots < oldSlots) { shrinkContainer(inX, inY, newSlots, i + 1); }
				if (newSlots > 0)
				{
					restretchMapContainedDecays(inX, inY, oldID, newID, i + 1);

					// negative IDs indicate sub-containment
					newID *= -1;
				}
			}
			newContained.push_back(newID);
			newDecayEta.push_back(mapETA);
		}
	}

	if (change)
	{
		int *containedArray = newContained.getElementArray();
		int  numContained   = newContained.size();

		setContained(inX, inY, newContained.size(), containedArray, inSubCont);
		delete[] containedArray;

		for (int i = 0; i < numContained; i++)
		{
			timeSec_t mapETA = newDecayEta.getElementDirect(i);

			if (mapETA != 0) { trackETA(inX, inY, 1 + i, mapETA, inSubCont); }

			setSlotEtaDecay(inX, inY, i, mapETA, inSubCont);
		}
	}

	if (contained != NULL) { delete[] contained; }
}

/**
 *
 * @param inX
 * @param inY
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 */
float getMapContainerTimeStretch(int inX, int inY, int inSubCont)
{

	float stretch = 1.0f;

	int containerID;

	if (inSubCont == 0) { containerID = getMapObjectRaw(inX, inY); }
	else
	{
		containerID = getContained(inX, inY, inSubCont - 1);
	}

	if (containerID < 0) { containerID *= -1; }

	if (containerID != 0) { stretch = getObject(containerID)->slotTimeStretch; }
	return stretch;
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
 * @param inSlot
 * @param inETA
 * @param inSubCont
 * @param inApplicableTrans
 * @note from server/map.cpp
 * // slot is 0 for main map cell, or higher for container slots
 */
void trackETA(int inX, int inY, int inSlot, timeSec_t inETA, int inSubCont, TransRecord *inApplicableTrans)
{

	timeSec_t timeLeft = inETA - MAP_TIMESEC;

	if (timeLeft < maxSecondsForActiveDecayTracking)
	{
		// track it live

		// duplicates okay
		// we'll deal with them when they ripen
		// (we check the true ETA stored in map before acting
		//   on one stored in this queue)
		LiveDecayRecord r = {inX, inY, inSlot, inETA, inSubCont, inApplicableTrans};

		char      exists;
		timeSec_t existingETA = liveDecayRecordPresentHashTable.lookup(inX, inY, inSlot, inSubCont, &exists);

		if (!exists || existingETA != inETA)
		{

			liveDecayQueue.insert(r, inETA);

			liveDecayRecordPresentHashTable.insert(inX, inY, inSlot, inSubCont, inETA);

			char exists;

			liveDecayRecordLastLookTimeHashTable.lookup(inX, inY, inSlot, inSubCont, &exists);

			if (!exists)
			{
				// don't overwrite old one
				liveDecayRecordLastLookTimeHashTable.insert(inX, inY, inSlot, inSubCont, MAP_TIMESEC);
				if (inSlot > 0 || inSubCont > 0)
				{

					ContRecord *oldCount =
							liveDecayRecordLastLookTimeMaxContainedHashTable.lookupPointer(inX, inY, 0, 0);

					if (oldCount != NULL)
					{
						// update if needed
						if (oldCount->maxSlots < inSlot) { oldCount->maxSlots = inSlot; }
						if (oldCount->maxSubSlots < inSubCont) { oldCount->maxSubSlots = inSubCont; }
					}
					else
					{
						// insert new
						ContRecord r = {inSlot, inSubCont};

						liveDecayRecordLastLookTimeMaxContainedHashTable.insert(inX, inY, 0, 0, r);
					}
				}
			}
		}
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param outSecondPlaceIndex
 * @param outSecondPlaceGap
 * @return
 * @note from server/map.cpp
 * // new code, topographic rings
 */
int computeMapBiomeIndex(int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap)
{

	int secondPlace = -1;

	double secondPlaceGap = 0;

	int pickedBiome = biomeGetCached(inX, inY, &secondPlace, &secondPlaceGap);

	if (pickedBiome != -2)
	{
		// hit cached

		if (outSecondPlaceIndex != NULL) { *outSecondPlaceIndex = secondPlace; }
		if (outSecondPlaceGap != NULL) { *outSecondPlaceGap = secondPlaceGap; }

		return pickedBiome;
	}

	// else cache miss
	pickedBiome = -1;

	// try topographical altitude mapping

	setXYRandomSeed(biomeRandSeed);

	double randVal = (getXYFractal(inX, inY, 0.55, 0.83332 + 0.08333 * numBiomes));

	// push into range 0..1, based on sampled min/max values
	randVal -= 0.099668;
	randVal *= 1.268963;

	// flatten middle
	// randVal = ( pow( 2*(randVal - 0.5 ), 3 ) + 1 ) / 2;

	// push into range 0..1 with manually tweaked values
	// these values make it pretty even in terms of distribution:
	// randVal -= 0.319;
	// randVal *= 3;

	// these values are more intuitve to make a map that looks good
	// randVal -= 0.23;
	// randVal *= 1.9;

	// apply gamma correction
	// randVal = pow( randVal, 1.5 );
	/*
	randVal += 0.4* sin( inX / 40.0 );
	randVal += 0.4 *sin( inY / 40.0 );

	randVal += 0.8;
	randVal /= 2.6;
	*/

	// slow arc n to s:

	// pow version has flat area in middle
	// randVal += 0.7 * pow( ( inY / 354.0 ), 3 ) ;

	// sin version
	// randVal += 0.3 * sin( 0.5 * M_PI * inY / 354.0 );

	/*
		( sin( M_PI * inY / 708 ) +
		  (1/3.0) * sin( 3 * M_PI * inY / 708 ) );
	*/
	// randVal += 0.5;
	// randVal /= 2.0;

	float i = randVal * biomeTotalWeight;

	pickedBiome = 0;
	while (pickedBiome < numBiomes && i > biomeCumuWeights[pickedBiome])
	{
		pickedBiome++;
	}
	if (pickedBiome >= numBiomes) { pickedBiome = numBiomes - 1; }

	if (pickedBiome >= regularBiomeLimit && numSpecialBiomes > 0)
	{
		// special case:  on a peak, place a special biome here

		// use patches mode for these
		pickedBiome = -1;

		double maxValue     = -10;
		double secondMaxVal = -10;

		for (int i = regularBiomeLimit; i < numBiomes; i++)
		{
			int biome = biomes[i];

			setXYRandomSeed(biome * 263 + biomeRandSeed + 38475);

			double randVal = getXYFractal(inX, inY, 0.55, 2.4999 + 0.2499 * numSpecialBiomes);

			if (randVal > maxValue)
			{
				if (maxValue != -10) { secondMaxVal = maxValue; }
				maxValue    = randVal;
				pickedBiome = i;
			}
		}

		if (maxValue - secondMaxVal < 0.03)
		{
			// close!  that means we're on a boundary between special biomes

			// stick last regular biome on this boundary, so special
			// biomes never touch
			secondPlace    = pickedBiome;
			secondPlaceGap = 0.1;
			pickedBiome    = regularBiomeLimit - 1;
		}
		else
		{
			secondPlace    = regularBiomeLimit - 1;
			secondPlaceGap = 0.1;
		}
	}
	else
	{
		// second place for regular biome rings

		secondPlace = pickedBiome - 1;
		if (secondPlace < 0) { secondPlace = pickedBiome + 1; }
		secondPlaceGap = 0.1;
	}

	biomePutCached(inX, inY, pickedBiome, secondPlace, secondPlaceGap);

	if (outSecondPlaceIndex != NULL) { *outSecondPlaceIndex = secondPlace; }
	if (outSecondPlaceGap != NULL) { *outSecondPlaceGap = secondPlaceGap; }

	return pickedBiome;
}

/**
 *
 * @param inX
 * @param inY
 * @param outSecondPlaceIndex
 * @param outSecondPlaceGap
 * @return
 * @note from server/map.cpp
 * // returns -2 on miss
 */
int biomeGetCached(int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap)
{
	BiomeCacheRecord r = biomeCache[computeXYCacheHash(inX, inY)];

	if (r.x == inX && r.y == inY)
	{
		*outSecondPlaceIndex = r.secondPlace;
		*outSecondPlaceGap   = r.secondPlaceGap;

		return r.biome;
	}
	else
	{
		return -2;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inBiome
 * @param inSecondPlace
 * @param inSecondPlaceGap
 * @note from server/map.cpp
 */
void biomePutCached(int inX, int inY, int inBiome, int inSecondPlace, double inSecondPlaceGap)
{
	BiomeCacheRecord r = {inX, inY, inBiome, inSecondPlace, inSecondPlaceGap};

	biomeCache[computeXYCacheHash(inX, inY)] = r;
}

/**
 *
 * @param inX
 * @param inY
 * @param inAbsoluteTimeInSeconds
 * @param inApplicableTrans
 * @note from server/map.cpp
 */
void setEtaDecay(int inX, int inY, timeSec_t inAbsoluteTimeInSeconds, TransRecord *inApplicableTrans)
{
	dbTimePut(inX, inY, DECAY_SLOT, inAbsoluteTimeInSeconds);
	if (inAbsoluteTimeInSeconds != 0) { trackETA(inX, inY, 0, inAbsoluteTimeInSeconds, 0, inApplicableTrans); }
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inTime
 * @param inSubCont
 * @note from server/map.cpp
 */
void dbTimePut(int inX, int inY, int inSlot, timeSec_t inTime, int inSubCont)
{
	// ETA decay changes don't get reported as map changes

	unsigned char key[16];
	unsigned char value[8];

	intQuadToKey(inX, inY, inSlot, inSubCont, key);
	timeToValue(inTime, value);

	DB_put(&timeDB, key, value);

	dbTimePutCached(inX, inY, inSlot, inSubCont, inTime);
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @param inValue
 * @note from server/map.cpp
 */
void dbTimePutCached(int inX, int inY, int inSlot, int inSubCont, timeSec_t inValue)
{
	DBTimeCacheRecord r = {inX, inY, inSlot, inSubCont, inValue};

	dbTimeCache[computeDBCacheHash(inX, inY, inSlot, inSubCont)] = r;
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 */
int getMapObject(int inX, int inY)
{

	// look at this map cell
	timeSec_t *oldLookTime = liveDecayRecordLastLookTimeHashTable.lookupPointer(inX, inY, 0, 0);

	timeSec_t curTime = MAP_TIMESEC;

	if (oldLookTime != NULL)
	{
		// we're tracking decay for this cell
		*oldLookTime = curTime;
	}

	// apply any decay that should have happened by now
	return checkDecayObject(inX, inY, getMapObjectRaw(inX, inY));
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
 * @return
 * @note from server/map.cpp
 * // returns 0 if not found
 */
timeSec_t dbLookTimeGet(int inX, int inY)
{
	unsigned char key[8];
	unsigned char value[8];

	intPairToKey(inX / 100, inY / 100, key);

	int result = DB_get(&lookTimeDB, key, value);

	if (result == 0)
	{
		// found
		return valueToTime(value);
	}
	else
	{
		return 0;
	}
}

/**
 * @note from server/map.cpp
 */
void clearRecentPlacements()
{
	for (int i = 0; i < NUM_RECENT_PLACEMENTS; i++)
	{
		recentPlacements[i].pos.x = 0;
		recentPlacements[i].pos.y = 0;
		recentPlacements[i].depth = 0;
	}

	writeRecentPlacements();
}


/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 * // gets contained item from specified slot, or from top of stack
// if inSlot is -1
// negative elements indicate sub-containers
 */
int getContained(int inX, int inY, int inSlot, int inSubCont)
{
	int num = getNumContained(inX, inY, inSubCont);

	if (num == 0) { return 0; }

	if (inSlot == -1 || inSlot > num - 1) { inSlot = num - 1; }

	int result = dbGet(inX, inY, FIRST_CONT_SLOT + inSlot, inSubCont);

	if (result != -1) { return result; }
	else
	{
		// nothing in that slot
		return 0;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param outNumContained
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 * // destroyed by caller, returns NULL if empty // negative elements indicate sub-containers
 */
timeSec_t *getContainedEtaDecay(int inX, int inY, int *outNumContained, int inSubCont)
{
	int num = getNumContained(inX, inY, inSubCont);

	*outNumContained = num;

	if (num == 0) { return NULL; }

	timeSec_t *containedEta = new timeSec_t[num];

	for (int i = 0; i < num; i++)
	{
		// can be 0 if not found, which is okay
		containedEta[i] = dbTimeGet(inX, inY, getContainerDecaySlot(inX, inY, i, inSubCont, num), inSubCont);
	}
	return containedEta;
}

/**
 *
 * @param inX
 * @param inY
 * @param inNumContained
 * @param inContainedEtaDecay
 * @param inSubCont
 * @note from server/map.cpp
 * // setting negative elements indicates sub containers
 */
void setContainedEtaDecay(int inX, int inY, int inNumContained, timeSec_t *inContainedEtaDecay, int inSubCont)
{
	char someDecay = false;
	for (int i = 0; i < inNumContained; i++)
	{
		dbTimePut(
				inX, inY, getContainerDecaySlot(inX, inY, i, inSubCont, inNumContained), inContainedEtaDecay[i], inSubCont);

		if (inContainedEtaDecay[i] != 0)
		{
			someDecay = true;
			trackETA(inX, inY, i + 1, inContainedEtaDecay[i], inSubCont);
		}
	}
	setSlotItemsNoDecay(inX, inY, inSubCont, !someDecay);
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @param inNumContained
 * @return
 * @note from server/map.cpp
 * // gets DB slot number where a given container slot's decay time is stored
// if inNumContained is -1, it will be looked up in database
 */
int getContainerDecaySlot(int inX, int inY, int inSlot, int inSubCont, int inNumContained)
{
	if (inNumContained == -1) { inNumContained = getNumContained(inX, inY, inSubCont); }

	return FIRST_CONT_SLOT + inNumContained + inSlot;
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 * // returns 0 if not found
 */
timeSec_t dbTimeGet(int inX, int inY, int inSlot, int inSubCont)
{

	timeSec_t cachedVal = dbTimeGetCached(inX, inY, inSlot, inSubCont);
	if (cachedVal != 1) { return cachedVal; }

	unsigned char key[16];
	unsigned char value[8];

	// look for changes to default in database
	intQuadToKey(inX, inY, inSlot, inSubCont, key);

	int result = DB_get(&timeDB, key, value);

	timeSec_t timeVal;

	if (result == 0)
	{
		// found
		timeVal = valueToTime(value);
	}
	else
	{
		timeVal = 0;
	}

	dbTimePutCached(inX, inY, inSlot, inSubCont, timeVal);

	return timeVal;
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @return
 * // returns 1 on miss
 */
int dbTimeGetCached(int inX, int inY, int inSlot, int inSubCont)
{
	DBTimeCacheRecord r = dbTimeCache[computeDBCacheHash(inX, inY, inSlot, inSubCont)];

	if (r.x == inX && r.y == inY && r.slot == inSlot && r.subCont == inSubCont && r.timeVal != 1) { return r.timeVal; }
	else
	{
		return 1;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 */
timeSec_t getFloorEtaDecay(int inX, int inY)
{
	return dbFloorTimeGet(inX, inY);
}

/**
 *
 * @param inX
 * @param inY
 * @param inAbsoluteTimeInSeconds
 * @note from server/map.cp
 */
void setFloorEtaDecay(int inX, int inY, timeSec_t inAbsoluteTimeInSeconds)
{
	dbFloorTimePut(inX, inY, inAbsoluteTimeInSeconds);
}

/**
 *
 * @param inX
 * @param inY
 * @param inID
 * @note from server/map.cpp
 */
void setMapFloor(int inX, int inY, int inID)
{

	logMapChange(inX, inY, inID);

	dbFloorPut(inX, inY, inID);

	// further decay from here
	TransRecord *newT = getMetaTrans(-1, inID);

	timeSec_t newEta = 0;

	if (newT != NULL)
	{
		timeSec_t curTime = MAP_TIMESEC;
		newEta            = curTime + newT->autoDecaySeconds;
	}

	setFloorEtaDecay(inX, inY, newEta);
}

/**
 *
 * @param inX
 * @param inY
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 */
int getNumContained(int inX, int inY, int inSubCont)
{
	int result = dbGet(inX, inY, NUM_CONT_SLOT, inSubCont);

	if (result != -1)
	{
		// found
		return result;
	}
	else
	{
		// default, empty container
		return 0;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inSubCont
 * @return
 * @note from server/map.cpp
 * // for all these calls, inSubCont indexes the main container (when 0)
// or sub-containers (when > 0).
// So, if inSubCont=3 and inSlot=2, we get information about the 2nd
// slot in the 3rd sub-container (the 3rd slot in the main container)
// for container slots
 */
timeSec_t getSlotEtaDecay(int inX, int inY, int inSlot, int inSubCont)
{
	// 0 if not found
	return dbTimeGet(inX, inY, getContainerDecaySlot(inX, inY, inSlot, inSubCont), inSubCont);
}

/**
 *
 * @param inX
 * @param inY
 * @param inSlot
 * @param inAbsoluteTimeInSeconds
 * @param inSubCont
 * @note from server/map.cpp
 * // for all these calls, inSubCont indexes the main container (when 0)
// or sub-containers (when > 0).
// So, if inSubCont=3 and inSlot=2, we get information about the 2nd
// slot in the 3rd sub-container (the 3rd slot in the main container)

// for container slots
 */
void setSlotEtaDecay(int inX, int inY, int inSlot, timeSec_t inAbsoluteTimeInSeconds, int inSubCont)
{
	dbTimePut(inX, inY, getContainerDecaySlot(inX, inY, inSlot, inSubCont), inAbsoluteTimeInSeconds, inSubCont);
	if (inAbsoluteTimeInSeconds != 0)
	{
		setSlotItemsNoDecay(inX, inY, inSubCont, false);

		trackETA(inX, inY, inSlot + 1, inAbsoluteTimeInSeconds, inSubCont);
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inOldContainerID
 * @param inNewContainerID
 * @param inSubCont
 * @note from server/map.cpp
 */
void restretchMapContainedDecays(int inX, int inY, int inOldContainerID, int inNewContainerID, int inSubCont)
{

	float oldStrech = getObject(inOldContainerID)->slotTimeStretch;
	float newStetch = getObject(inNewContainerID)->slotTimeStretch;

	if (oldStrech != newStetch)
	{

		int        oldNum;
		timeSec_t *oldContDecay = getContainedEtaDecay(inX, inY, &oldNum, inSubCont);

		restretchDecays(oldNum, oldContDecay, inOldContainerID, inNewContainerID);

		setContainedEtaDecay(inX, inY, oldNum, oldContDecay, inSubCont);
		delete[] oldContDecay;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note returns 0 if not found
 */
timeSec_t dbFloorTimeGet(int inX, int inY)
{
	unsigned char key[8];
	unsigned char value[8];

	intPairToKey(inX, inY, key);

	int result = DB_get(&floorTimeDB, key, value);

	if (result == 0)
	{
		// found
		return valueToTime(value);
	}
	else
	{
		return 0;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inTime
 * @note from server/map.cpp
 */
void dbFloorTimePut(int inX, int inY, timeSec_t inTime)
{
	// ETA decay changes don't get reported as map changes

	unsigned char key[8];
	unsigned char value[8];

	intPairToKey(inX, inY, key);
	timeToValue(inTime, value);

	DB_put(&floorTimeDB, key, value);
}

/**
 *
 * @param inX
 * @param inY
 * @param inValue
 * @note from server/map.cpp
 */
void dbFloorPut(int inX, int inY, int inValue)
{

	if (!skipTrackingMapChanges)
	{

		char found = false;
		for (int i = 0; i < mapChangePosSinceLastStep.size(); i++)
		{

			ChangePosition *p = mapChangePosSinceLastStep.getElement(i);

			if (p->x == inX && p->y == inY)
			{
				found = true;

				// update it
				p->responsiblePlayerID = currentResponsiblePlayer;
				break;
			}
		}

		if (!found)
		{
			ChangePosition p = {inX, inY, false, currentResponsiblePlayer, 0, 0, 0.0};
			mapChangePosSinceLastStep.push_back(p);
		}
	}

	unsigned char key[8];
	unsigned char value[4];

	intPairToKey(inX, inY, key);
	intToValue(inValue, value);

	DB_put(&floorDB, key, value);
}

/**
 *
 * @param inX
 * @param inY
 * @param inContainedID
 * @param inEtaDecay
 * @param inSubCont
 * @note from server/map.cpp
 * // adds to top of stack
// negative elements indicate sub-containers
 */
void addContained(int inX, int inY, int inContainedID, timeSec_t inEtaDecay, int inSubCont)
{
	int oldNum;

	timeSec_t curTime = MAP_TIMESEC;

	if (inEtaDecay != 0)
	{
		timeSec_t etaOffset = inEtaDecay - curTime;

		inEtaDecay = curTime + etaOffset / getMapContainerTimeStretch(inX, inY, inSubCont);
	}

	int *oldContained = getContained(inX, inY, &oldNum, inSubCont);

	timeSec_t *oldContainedETA = getContainedEtaDecay(inX, inY, &oldNum, inSubCont);

	int *newContained = new int[oldNum + 1];

	if (oldNum != 0) { memcpy(newContained, oldContained, oldNum * sizeof(int)); }

	newContained[oldNum] = inContainedID;

	if (oldContained != NULL) { delete[] oldContained; }

	timeSec_t *newContainedETA = new timeSec_t[oldNum + 1];

	if (oldNum != 0) { memcpy(newContainedETA, oldContainedETA, oldNum * sizeof(timeSec_t)); }

	newContainedETA[oldNum] = inEtaDecay;

	if (oldContainedETA != NULL) { delete[] oldContainedETA; }

	int newNum = oldNum + 1;

	setContained(inX, inY, newNum, newContained, inSubCont);
	setContainedEtaDecay(inX, inY, newNum, newContainedETA, inSubCont);

	delete[] newContained;
	delete[] newContainedETA;
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 */
timeSec_t getEtaDecay(int inX, int inY)
{
	// 0 if not found
	return dbTimeGet(inX, inY, DECAY_SLOT);
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 */
int getMapBiome(int inX, int inY)
{
	return biomes[getMapBiomeIndex(inX, inY)];
}

/**
 *
 * @param inNumDecays
 * @param inDecayEtas
 * @param inOldContainerID
 * @param inNewContainerID
 * @note from server/map.cpp
 */
void restretchDecays(int inNumDecays, timeSec_t *inDecayEtas, int inOldContainerID, int inNewContainerID)
{

	float oldStrech = getObject(inOldContainerID)->slotTimeStretch;
	float newStetch = getObject(inNewContainerID)->slotTimeStretch;

	if (oldStrech != newStetch)
	{
		timeSec_t curTime = MAP_TIMESEC;

		for (int i = 0; i < inNumDecays; i++)
		{
			if (inDecayEtas[i] != 0)
			{
				int offset = inDecayEtas[i] - curTime;

				offset         = offset * oldStrech;
				offset         = offset / newStetch;
				inDecayEtas[i] = curTime + offset;
			}
		}
	}
}

/**
 *
 * @param inSkipCleanup
 * @note from server/map.cpp
 */
void freeMap(char inSkipCleanup)
{
	if (mapChangeLogFile != NULL)
	{
		fclose(mapChangeLogFile);
		mapChangeLogFile = NULL;
	}

	printf("%d calls to getBaseMap\n", getBaseMapCallCount);

	skipTrackingMapChanges = true;

	if (lookTimeDBOpen)
	{
		DB_close(&lookTimeDB);
		lookTimeDBOpen = false;
	}

	if (dbOpen && !inSkipCleanup)
	{

		AppLog::infoF("Cleaning up map database on server shutdown.");

		// iterate through DB and look for useDummy objects
		// replace them with unused version object
		// useDummy objects aren't real objects in objectBank,
		// and their IDs may change in the future, so they're
		// not safe to store in the map between server runs.

		DB_Iterator dbi;

		DB_Iterator_init(&db, &dbi);

		unsigned char key[16];

		unsigned char value[4];

		// keep list of x,y coordinates in map that need replacing
		SimpleVector<int> xToPlace;
		SimpleVector<int> yToPlace;

		SimpleVector<int> idToPlace;

		// container slots that need replacing
		SimpleVector<int> xContToCheck;
		SimpleVector<int> yContToCheck;
		SimpleVector<int> bContToCheck;

		int skipUseDummyCleanup = SettingsManager::getIntSetting("skipUseDummyCleanup", 0);

		if (!skipUseDummyCleanup)
		{

			FILE *dummyFile = fopen("mapDummyRecall.txt", "w");

			while (DB_Iterator_next(&dbi, key, value) > 0)
			{

				int s = valueToInt(&(key[8]));
				int b = valueToInt(&(key[12]));

				if (s == 0)
				{
					int id = valueToInt(value);

					if (id > 0)
					{

						ObjectRecord *mapO = getObject(id);

						if (mapO != NULL)
						{
							if (mapO->isUseDummy)
							{
								int x = valueToInt(key);
								int y = valueToInt(&(key[4]));

								xToPlace.push_back(x);
								yToPlace.push_back(y);
								idToPlace.push_back(mapO->useDummyParent);

								rememberDummy(dummyFile, x, y, mapO);
							}
							else if (mapO->isVariableDummy)
							{
								int x = valueToInt(key);
								int y = valueToInt(&(key[4]));

								xToPlace.push_back(x);
								yToPlace.push_back(y);
								idToPlace.push_back(mapO->variableDummyParent);

								rememberDummy(dummyFile, x, y, mapO);
							}
						}
					}
				}
				else if (s == 2)
				{
					int numSlots = valueToInt(value);
					if (numSlots > 0)
					{
						int x = valueToInt(key);
						int y = valueToInt(&(key[4]));
						xContToCheck.push_back(x);
						yContToCheck.push_back(y);
						bContToCheck.push_back(b);
					}
				}
			}

			for (int i = 0; i < xToPlace.size(); i++)
			{
				int x = xToPlace.getElementDirect(i);
				int y = yToPlace.getElementDirect(i);

				setMapObjectRaw(x, y, idToPlace.getElementDirect(i));
			}

			int numContChanged = 0;

			for (int i = 0; i < xContToCheck.size(); i++)
			{
				int x = xContToCheck.getElementDirect(i);
				int y = yContToCheck.getElementDirect(i);
				int b = bContToCheck.getElementDirect(i);

				if (getMapObjectRaw(x, y) != 0)
				{

					int  numCont;
					int *cont = getContainedRaw(x, y, &numCont, b);

					char anyChanged = false;

					for (int c = 0; c < numCont; c++)
					{

						char subCont = false;

						if (cont[c] < 0)
						{
							cont[c] *= -1;
							subCont = true;
						}

						ObjectRecord *contObj = getObject(cont[c]);

						if (contObj != NULL)
						{
							if (contObj->isUseDummy)
							{
								cont[c] = contObj->useDummyParent;
								rememberDummy(dummyFile, x, y, contObj, c, b);

								anyChanged = true;
								numContChanged++;
							}
							else if (contObj->isVariableDummy)
							{
								cont[c] = contObj->variableDummyParent;
								rememberDummy(dummyFile, x, y, contObj, c, b);

								anyChanged = true;
								numContChanged++;
							}
						}

						if (subCont) { cont[c] *= -1; }
					}

					if (anyChanged) { setContained(x, y, numCont, cont, b); }

					delete[] cont;
				}
			}

			if (dummyFile != NULL) { fclose(dummyFile); }

			AppLog::infoF("...%d useDummy/variable objects present that were changed "
						  "back into their unused parent.",
						  xToPlace.size());
			AppLog::infoF("...%d contained useDummy/variable objects present and changed "
						  "back to usused parent.",
						  numContChanged);
		}
		else
		{
			AppLog::info("Skipping use dummy cleanup.");
		}

		printf("\n");

		if (!skipRemovedObjectCleanup)
		{
			AppLog::info("Now running normal map clean...");
			cleanMap();
		}
		else
		{
			AppLog::info("Skipping running normal map clean.");
		}

		DB_close(&db);
		dbOpen = false;
	}
	else if (dbOpen)
	{
		// just close with no cleanup
		DB_close(&db);
		dbOpen = false;
	}

	if (timeDBOpen)
	{
		DB_close(&timeDB);
		timeDBOpen = false;
	}

	if (biomeDBOpen)
	{
		DB_close(&biomeDB);
		biomeDBOpen = false;
	}

	if (floorDBOpen)
	{
		DB_close(&floorDB);
		floorDBOpen = false;
	}

	if (floorTimeDBOpen)
	{
		DB_close(&floorTimeDB);
		floorTimeDBOpen = false;
	}

	if (graveDBOpen)
	{
		DB_close(&graveDB);
		graveDBOpen = false;
	}

	if (eveDBOpen)
	{
		DB_close(&eveDB);
		eveDBOpen = false;
	}

	if (metaDBOpen)
	{
		DB_close(&metaDB);
		metaDBOpen = false;
	}

	writeEveRadius();
	writeRecentPlacements();

	delete[] biomes;
	delete[] biomeWeights;
	delete[] biomeCumuWeights;
	delete[] specialBiomes;
	delete[] specialBiomeCumuWeights;

	delete[] naturalMapIDs;
	delete[] naturalMapChances;
	delete[] totalChanceWeight;

	allNaturalMapIDs.deleteAll();

	liveDecayQueue.clear();
	liveDecayRecordPresentHashTable.clear();
	liveDecayRecordLastLookTimeHashTable.clear();
	liveMovementEtaTimes.clear();

	liveMovements.clear();

	mapChangePosSinceLastStep.deleteAll();

	skipTrackingMapChanges = false;

	delete[] speechPipesIn;
	delete[] speechPipesOut;

	speechPipesIn  = NULL;
	speechPipesOut = NULL;

	flightLandingPos.deleteAll();
}

/**
 *
 * @param inMapChanges
 * @param inChangePosList
 * @note server/map.cpp
 * // any change lines resulting from step are appended to inMapChanges
// any change positions are added to end of inChangePosList
 */
void stepMap(SimpleVector<MapChangeRecord> *inMapChanges, SimpleVector<ChangePosition> *inChangePosList)
{

	timeSec_t curTime = MAP_TIMESEC;

	lookTimeTracking.cleanStale(curTime - noLookCountAsStaleSeconds);

	while (liveDecayQueue.size() > 0 && liveDecayQueue.checkMinPriority() <= curTime)
	{

		// another expired

		LiveDecayRecord r = liveDecayQueue.removeMin();

		char      storedFound;
		timeSec_t storedETA = liveDecayRecordPresentHashTable.lookup(r.x, r.y, r.slot, r.subCont, &storedFound);

		if (storedFound && storedETA == r.etaTimeSeconds)
		{

			liveDecayRecordPresentHashTable.remove(r.x, r.y, r.slot, r.subCont);

			timeSec_t lastLookTime =
					liveDecayRecordLastLookTimeHashTable.lookup(r.x, r.y, r.slot, r.subCont, &storedFound);

			if (storedFound)
			{

				if (MAP_TIMESEC - lastLookTime > maxSecondsNoLookDecayTracking
					&& !isDecayTransAlwaysLiveTracked(r.applicableTrans))
				{

					// this cell or slot hasn't been looked at in too long
					// AND it's not a trans that's live tracked even when
					// not watched

					// don't even apply this decay now
					liveDecayRecordLastLookTimeHashTable.remove(r.x, r.y, r.slot, r.subCont);
					cleanMaxContainedHashTable(r.x, r.y);
					continue;
				}
				// else keep lastlook time around in case
				// this cell will decay further and we're still tracking it
				// (but maybe delete it if cell is no longer tracked, below)
			}
		}

		if (r.slot == 0)
		{

			int oldID = getMapObjectRaw(r.x, r.y);

			// apply real eta from map (to ignore stale duplicates in live list)
			// and update live list if new object is decaying too

			// this call will append changes to our global lists, which
			// we process below
			checkDecayObject(r.x, r.y, oldID);
		}
		else
		{
			if (!getSlotItemsNoDecay(r.x, r.y, r.subCont)) { checkDecayContained(r.x, r.y, r.subCont); }
		}

		char stillExists;
		liveDecayRecordPresentHashTable.lookup(r.x, r.y, r.slot, r.subCont, &stillExists);

		if (!stillExists)
		{
			// cell or slot no longer tracked
			// forget last look time
			liveDecayRecordLastLookTimeHashTable.remove(r.x, r.y, r.slot, r.subCont);

			cleanMaxContainedHashTable(r.x, r.y);
		}
	}

	while (liveMovements.size() > 0 && liveMovements.checkMinPriority() <= curTime)
	{
		MovementRecord r = liveMovements.removeMin();
		liveMovementEtaTimes.remove(r.x, r.y, 0, 0);
	}

	// all of them, including these new ones and others acculuated since
	// last step are accumulated in these global vectors

	int numPos = mapChangePosSinceLastStep.size();

	for (int i = 0; i < numPos; i++)
	{
		ChangePosition p = mapChangePosSinceLastStep.getElementDirect(i);

		inChangePosList->push_back(p);

		MapChangeRecord changeRecord = getMapChangeRecord(p);
		inMapChanges->push_back(changeRecord);
	}

	mapChangePosSinceLastStep.deleteAll();
}

/**
 *
 * @return
 * @note from server/map.cpp
 * // returns number of seconds from now until when next decay is supposed
// to happen
// returns -1 if no decay pending
 */
int getNextDecayDelta()
{
	if (liveDecayQueue.size() == 0) { return -1; }

	timeSec_t curTime = MAP_TIMESEC;

	timeSec_t minTime = liveDecayQueue.checkMinPriority();

	if (minTime <= curTime) { return 0; }

	return minTime - curTime;
}

/**
 *
 * @param inFile
 * @param inX
 * @param inY
 * @param inDummyO
 * @param inSlot
 * @param inB
 * @note from server/map.cpp
 */
void rememberDummy(FILE *inFile, int inX, int inY, ObjectRecord *inDummyO, int inSlot, int inB)
{

	if (inFile == NULL) { return; }

	int parent     = -1;
	int dummyIndex = -1;

	char marker = 'x';

	if (inDummyO->isUseDummy)
	{
		marker = 'u';

		parent                = inDummyO->useDummyParent;
		ObjectRecord *parentO = getObject(parent);

		if (parentO != NULL)
		{
			for (int i = 0; i < parentO->numUses - 1; i++)
			{
				if (parentO->useDummyIDs[i] == inDummyO->id)
				{
					dummyIndex = i;
					break;
				}
			}
		}
	}
	else if (inDummyO->isVariableDummy)
	{
		marker = 'v';

		parent                = inDummyO->variableDummyParent;
		ObjectRecord *parentO = getObject(parent);

		if (parentO != NULL)
		{
			for (int i = 0; i < parentO->numVariableDummyIDs; i++)
			{
				if (parentO->variableDummyIDs[i] == inDummyO->id)
				{
					dummyIndex = i;
					break;
				}
			}
		}
	}

	if (parent > 0 && dummyIndex >= 0)
	{
		if (inSlot == -1 && inB == 0) { fprintf(inFile, "(%d,%d) %c %d %d\n", inX, inY, marker, parent, dummyIndex); }
		else
		{
			fprintf(inFile, "(%d,%d) %c %d %d [%d %d]\n", inX, inY, marker, parent, dummyIndex, inSlot, inB);
		}
	}
}

/**
 *
 * @param inXStart
 * @param inYStart
 * @param inXEnd
 * @param inYEnd
 * @note from server/map.cpp
 * // marks region as looked at, so that live decay tracking continues
// there
 */
void lookAtRegion(int inXStart, int inYStart, int inXEnd, int inYEnd)
{
	timeSec_t currentTime = MAP_TIMESEC;

	for (int y = inYStart; y <= inYEnd; y++)
	{
		for (int x = inXStart; x <= inXEnd; x++)
		{

			if (!lookTimeTracking.checkExists(x, y, currentTime))
			{

				// we haven't looked at this spot in a while

				// see if any decays apply
				// if so, get that part of the tile once to re-trigger
				// live tracking

				timeSec_t floorEtaDecay = getFloorEtaDecay(x, y);

				if (floorEtaDecay != 0 && floorEtaDecay < currentTime + maxSecondsForActiveDecayTracking)
				{ getMapFloor(x, y); }

				timeSec_t etaDecay = getEtaDecay(x, y);

				int objID = 0;

				if (etaDecay != 0 && etaDecay < currentTime + maxSecondsForActiveDecayTracking)
				{ objID = getMapObject(x, y); }

				// also check all contained items to trigger
				// live tracking of their decays too
				if (objID != 0)
				{

					int numCont = getNumContained(x, y);

					for (int c = 0; c < numCont; c++)
					{
						int contID = getContained(x, y, c);

						if (contID < 0)
						{
							// sub cont
							int numSubCont = getNumContained(x, y, c + 1);

							for (int s = 0; s < numSubCont; s++)
							{
								getContained(x, y, c, s + 1);
							}
						}
					}
				}
			}

			timeSec_t *oldLookTime = liveDecayRecordLastLookTimeHashTable.lookupPointer(x, y, 0, 0);

			if (oldLookTime != NULL)
			{
				// we're tracking decay for this cell
				*oldLookTime = currentTime;
			}

			ContRecord *contRec = liveDecayRecordLastLookTimeMaxContainedHashTable.lookupPointer(x, y, 0, 0);

			if (contRec != NULL)
			{

				for (int c = 1; c <= contRec->maxSlots; c++)
				{

					oldLookTime = liveDecayRecordLastLookTimeHashTable.lookupPointer(x, y, c, 0);
					if (oldLookTime != NULL)
					{
						// look at it now
						*oldLookTime = currentTime;
					}

					for (int s = 1; s <= contRec->maxSubSlots; s++)
					{

						oldLookTime = liveDecayRecordLastLookTimeHashTable.lookupPointer(x, y, c, s);
						if (oldLookTime != NULL)
						{
							// look at it now
							*oldLookTime = currentTime;
						}
					}
				}
			}
		}
	}
}

/**
 *
 * @param inTrans
 * @return
 * @note from server/map.cpp
 * // certain types of movement transitions should always be live
// tracked, even when out of view (NSEW moves, for human-made traveling objects
// for example)
 */
char isDecayTransAlwaysLiveTracked(TransRecord *inTrans)
{
	if (inTrans != NULL && inTrans->move >= 4 && inTrans->move <= 7) { return true; }

	return false;
}

/**
 *
 * @param inX
 * @param inY
 * @note from server/map.cpp
 */
void cleanMaxContainedHashTable(int inX, int inY)
{

	ContRecord *oldCount = liveDecayRecordLastLookTimeMaxContainedHashTable.lookupPointer(inX, inY, 0, 0);

	if (oldCount != NULL)
	{

		int maxFoundSlot    = 0;
		int maxFoundSubSlot = 0;

		for (int c = 1; c <= oldCount->maxSlots; c++)
		{

			for (int s = 0; s <= oldCount->maxSubSlots; s++)
			{
				timeSec_t *val = liveDecayRecordLastLookTimeHashTable.lookupPointer(inX, inY, c, s);

				if (val != NULL)
				{
					maxFoundSlot    = c;
					maxFoundSubSlot = s;
				}
			}
		}

		if (maxFoundSlot == 0 && maxFoundSubSlot == 0)
		{ liveDecayRecordLastLookTimeMaxContainedHashTable.remove(inX, inY, 0, 0); }
		else
		{
			if (maxFoundSlot < oldCount->maxSlots) { oldCount->maxSlots = maxFoundSlot; }
			if (maxFoundSubSlot < oldCount->maxSubSlots) { oldCount->maxSubSlots = maxFoundSubSlot; }
		}
	}
}

/**
 * @note from server/map.cpp
 */
void doubleEveRadius()
{
	if (eveRadius < 1024)
	{
		eveRadius *= 2;
		writeEveRadius();
	}
}

/**
 * @note from server/map.cpp
 */
void resetEveRadius()
{
	eveRadius = eveRadiusStart;
	writeEveRadius();
}
/**
 *
 * @return
 * @note from server/map.cpp
 * // returns num set after
 */
int cleanMap()
{
	AppLog::info("\nCleaning map of objects that have been removed...");

	skipTrackingMapChanges = true;

	DB_Iterator dbi;

	DB_Iterator_init(&db, &dbi);

	unsigned char key[16];

	unsigned char value[4];

	// keep list of x,y coordinates in map that need clearing
	SimpleVector<int> xToClear;
	SimpleVector<int> yToClear;

	// container slots that need clearing
	SimpleVector<int> xContToCheck;
	SimpleVector<int> yContToCheck;

	int totalDBRecordCount = 0;

	int totalSetCount       = 0;
	int numClearedCount     = 0;
	int totalNumContained   = 0;
	int numContainedCleared = 0;

	while (DB_Iterator_next(&dbi, key, value) > 0)
	{
		totalDBRecordCount++;

		int s = valueToInt(&(key[8]));
		int b = valueToInt(&(key[12]));

		if (s == 0)
		{
			int id = valueToInt(value);

			if (id > 0)
			{
				totalSetCount++;

				ObjectRecord *o = getObject(id);

				if (o == NULL || getIsCategory(id) || o->description[0] == '@' || o->isOwned)
				{
					// id doesn't exist anymore

					// OR it's a non-pattern category
					// those should never exist in map
					// may be left over from a non-clean shutdown

					// OR object is flagged with @
					// this may be a pattern category that is actually
					// a place-holder

					// OR it's owned (no owned objects should be left
					// on map after server restarts... server must have
					// crashed and not shut down properly)

					numClearedCount++;

					int x = valueToInt(key);
					int y = valueToInt(&(key[4]));

					xToClear.push_back(x);
					yToClear.push_back(y);
				}
			}
		}
		if (s == 2 && b == 0)
		{
			int numSlots = valueToInt(value);
			if (numSlots > 0)
			{
				totalNumContained += numSlots;

				int x = valueToInt(key);
				int y = valueToInt(&(key[4]));
				xContToCheck.push_back(x);
				yContToCheck.push_back(y);
			}
		}
	}

	for (int i = 0; i < xToClear.size(); i++)
	{
		int x = xToClear.getElementDirect(i);
		int y = yToClear.getElementDirect(i);

		clearAllContained(x, y);
		setMapObject(x, y, 0);
	}

	for (int i = 0; i < xContToCheck.size(); i++)
	{
		int x = xContToCheck.getElementDirect(i);
		int y = yContToCheck.getElementDirect(i);

		if (getMapObjectRaw(x, y) != 0)
		{
			int        numCont;
			int *      cont  = getContainedRaw(x, y, &numCont);
			timeSec_t *decay = getContainedEtaDecay(x, y, &numCont);

			SimpleVector<int>       newCont;
			SimpleVector<timeSec_t> newDecay;

			SimpleVector<SimpleVector<int>>       newSubCont;
			SimpleVector<SimpleVector<timeSec_t>> newSubContDecay;

			char anyRemoved = false;

			for (int c = 0; c < numCont; c++)
			{

				SimpleVector<int>       subCont;
				SimpleVector<timeSec_t> subContDecay;

				char thisKept = false;

				if (cont[c] < 0)
				{

					ObjectRecord *o = getObject(-cont[c]);

					if (o != NULL && !getIsCategory(-cont[c]))
					{

						thisKept = true;

						newCont.push_back(cont[c]);
						newDecay.push_back(decay[c]);

						int numSub;

						int *      contSub  = getContainedRaw(x, y, &numSub, c + 1);
						timeSec_t *decaySub = getContainedEtaDecay(x, y, &numSub, c + 1);

						for (int s = 0; s < numSub; s++)
						{

							if (getObject(contSub[s]) != NULL && !getIsCategory(contSub[s]))
							{

								subCont.push_back(contSub[s]);
								subContDecay.push_back(decaySub[s]);
							}
							else
							{
								anyRemoved = true;
							}
						}

						if (contSub != NULL) { delete[] contSub; }
						if (decaySub != NULL) { delete[] decaySub; }
						numContainedCleared += numSub - subCont.size();
					}
				}
				else
				{
					ObjectRecord *o = getObject(cont[c]);
					if (o != NULL && !getIsCategory(cont[c]))
					{

						thisKept = true;
						newCont.push_back(cont[c]);
						newDecay.push_back(decay[c]);
					}
					else
					{
						anyRemoved = true;
					}
				}

				if (thisKept)
				{
					newSubCont.push_back(subCont);
					newSubContDecay.push_back(subContDecay);
				}
				else
				{
					anyRemoved = true;
				}
			}

			delete[] cont;
			delete[] decay;

			if (anyRemoved)
			{

				numContainedCleared += (numCont - newCont.size());

				int *      newContArray  = newCont.getElementArray();
				timeSec_t *newDecayArray = newDecay.getElementArray();

				setContained(x, y, newCont.size(), newContArray);
				setContainedEtaDecay(x, y, newDecay.size(), newDecayArray);

				for (int c = 0; c < newCont.size(); c++)
				{
					int numSub = newSubCont.getElementDirect(c).size();

					if (numSub > 0)
					{
						int *      newSubArray      = newSubCont.getElementDirect(c).getElementArray();
						timeSec_t *newSubDecayArray = newSubContDecay.getElementDirect(c).getElementArray();

						setContained(x, y, numSub, newSubArray, c + 1);

						setContainedEtaDecay(x, y, numSub, newSubDecayArray, c + 1);

						delete[] newSubArray;
						delete[] newSubDecayArray;
					}
					else
					{
						clearAllContained(x, y, c + 1);
					}
				}

				delete[] newContArray;
				delete[] newDecayArray;
			}
		}
	}

	AppLog::infoF("...%d map cells were set, and %d needed to be cleared.", totalSetCount, numClearedCount);
	AppLog::infoF(
			"...%d contained objects present, and %d needed to be cleared.", totalNumContained, numContainedCleared);
	AppLog::infoF("...%d database records total (%d max hash bin depth).", totalDBRecordCount, DB_maxStack);

	printf("\n");

	skipTrackingMapChanges = false;

	return totalSetCount;
}

/**
 *
 * @param inID
 * @return
 * @note server/map.cpp
 * // true if ID is a non-pattern category
 */
char getIsCategory(int inID)
{
	CategoryRecord *r = getCategory(inID);

	if (r == NULL) { return false; }
	if (r->isPattern) { return false; }
	return true;
}

/**
 *
 * @param inPos
 * @return
 * @note form server/map.cpp
 * // formatString in returned record destroyed by caller
 */
MapChangeRecord getMapChangeRecord(ChangePosition inPos)
{

	MapChangeRecord r;
	r.absoluteX     = inPos.x;
	r.absoluteY     = inPos.y;
	r.oldCoordsUsed = false;

	// compose format string
	SimpleVector<char> buffer;

	char *header = autoSprintf("%%d %%d %d ", hideIDForClient(getMapFloor(inPos.x, inPos.y)));

	buffer.appendElementString(header);

	delete[] header;

	char *idString = autoSprintf("%d", hideIDForClient(getMapObjectNoLook(inPos.x, inPos.y)));

	buffer.appendElementString(idString);

	delete[] idString;

	int  numContained;
	int *contained = getContainedNoLook(inPos.x, inPos.y, &numContained);

	for (int i = 0; i < numContained; i++)
	{

		char subCont = false;

		if (contained[i] < 0)
		{
			subCont = true;
			contained[i] *= -1;
		}

		char *idString = autoSprintf(",%d", hideIDForClient(contained[i]));

		buffer.appendElementString(idString);

		delete[] idString;

		if (subCont)
		{

			int  numSubContained;
			int *subContained = getContainedNoLook(inPos.x, inPos.y, &numSubContained, i + 1);
			for (int s = 0; s < numSubContained; s++)
			{

				idString = autoSprintf(":%d", hideIDForClient(subContained[s]));

				buffer.appendElementString(idString);

				delete[] idString;
			}
			if (subContained != NULL) { delete[] subContained; }
		}
	}

	if (contained != NULL) { delete[] contained; }

	char *player = autoSprintf(" %d", inPos.responsiblePlayerID);

	buffer.appendElementString(player);

	delete[] player;

	if (inPos.speed > 0)
	{
		r.absoluteOldX  = inPos.oldX;
		r.absoluteOldY  = inPos.oldY;
		r.oldCoordsUsed = true;

		char *moveString = autoSprintf(" %%d %%d %f", inPos.speed);

		buffer.appendElementString(moveString);

		delete[] moveString;
	}

	buffer.appendElementString("\n");

	r.formatString = buffer.getElementString();

	return r;
}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note form server/map.cpp
 */
int getMapObjectNoLook(int inX, int inY)
{

	// apply any decay that should have happened by now
	return checkDecayObject(inX, inY, getMapObjectRaw(inX, inY));
}

/**
 *
 * @param inX
 * @param inY
 * @param outNumContained
 * @param inSubCont
 * @return
 * @note form server/map.cpp
 */
int *getContainedNoLook(int inX, int inY, int *outNumContained, int inSubCont)
{
	if (!getSlotItemsNoDecay(inX, inY, inSubCont)) { checkDecayContained(inX, inY, inSubCont); }
	return getContainedRaw(inX, inY, outNumContained, inSubCont);
}