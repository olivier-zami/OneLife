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
#include "../../third_party/minorGems/util/crc32.h"
#include "../../third_party/minorGems/util/random/JenkinsRandomSource.h"
#include "../../gameSource/objectBank.h"
#include "../../gameSource/objectMetadata.h"
#include "../../commonSource/fractalNoise.h"
#include "../../commonSource/math/misc.h"
#include "../arcReport.h"
#include "../dbCommon.h"
#include "../map.h"
#include "../monument.h"

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
