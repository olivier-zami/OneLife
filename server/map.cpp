#include "map.h"
#include "HashTable.h"
#include "arcReport.h"
#include "monument.h"

#include "CoordinateTimeTracking.h"

#include "component/Map.h"

#include "minorGems/util/random/CustomRandomSource.h"
#include "minorGems/util/random/JenkinsRandomSource.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"

#include "minorGems/formats/encodingUtils.h"

#include "kissdb.h"
//#include "stackdb.h"
//#include "lineardb.h"
#include "objectType/bank/linearDB/lineardb3.h"

#include "minorGems/util/crc32.h"

/*
#define DB KISSDB
#define DB_open KISSDB_open
#define DB_close KISSDB_close
#define DB_get KISSDB_get
#define DB_put KISSDB_put
// no distinction between insert and replace in KISSS
#define DB_put_new KISSDB_put
#define DB_Iterator  KISSDB_Iterator
#define DB_Iterator_init  KISSDB_Iterator_init
#define DB_Iterator_next  KISSDB_Iterator_next
#define DB_maxStack (int)( db.num_hash_tables )
// no support for shrinking
#define DB_getShrinkSize( dbP, n )  dbP->hashTableSize
#define DB_getCurrentSize( dbP )  dbP->hashTableSize
// no support for counting records
#define DB_getNumRecords( dbP ) 0
*/

/*
#define DB STACKDB
#define DB_open STACKDB_open
#define DB_close STACKDB_close
#define DB_get STACKDB_get
#define DB_put STACKDB_put
// stack DB has faster insert
#define DB_put_new STACKDB_put_new
#define DB_Iterator  STACKDB_Iterator
#define DB_Iterator_init  STACKDB_Iterator_init
#define DB_Iterator_next  STACKDB_Iterator_next
#define DB_maxStack db.maxStackDepth
// no support for shrinking
#define DB_getShrinkSize( dbP, n )  dbP->hashTableSize
#define DB_getCurrentSize( dbP )  dbP->hashTableSize
// no support for counting records
#define DB_getNumRecords( dbP ) 0
*/

/*
#define DB LINEARDB
#define DB_open LINEARDB_open
#define DB_close LINEARDB_close
#define DB_get LINEARDB_get
#define DB_put LINEARDB_put
// no distinction between put and put_new in lineardb
#define DB_put_new LINEARDB_put
#define DB_Iterator  LINEARDB_Iterator
#define DB_Iterator_init  LINEARDB_Iterator_init
#define DB_Iterator_next  LINEARDB_Iterator_next
#define DB_maxStack db.maxProbeDepth
#define DB_getShrinkSize  LINEARDB_getShrinkSize
#define DB_getCurrentSize  LINEARDB_getCurrentSize
#define DB_getNumRecords LINEARDB_getNumRecords
*/

#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>

#include "../commonSource/Debug.h"
#include "../gameSource/GridPos.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/transitionBank.h"
#include "../gameSource/GridPos.h"
#include "../gameSource/objectMetadata.h"
#include "component/cache/Biome.h"
#include "component/generator/Biome.h"
#include "dbCommon.h"
#include "game/Server.h"

extern int getBaseMapCallCount;
extern CustomRandomSource randSource;

timeSec_t startFrozenTime = -1;

timeSec_t frozenTime()
{
	if (startFrozenTime == -1) { startFrozenTime = Time::timeSec(); }
	return startFrozenTime;
}

timeSec_t startFastTime = -1;

timeSec_t fastTime()
{
	if (startFastTime == -1) { startFastTime = Time::timeSec(); }
	return 1000 * (Time::timeSec() - startFastTime) + startFastTime;
}

timeSec_t slowTime()
{
	if (startFastTime == -1) { startFastTime = Time::timeSec(); }
	return (Time::timeSec() - startFastTime) / 4 + startFastTime;
}

// can replace with frozenTime to freeze time
// or slowTime to slow it down
#define MAP_TIMESEC Time::timeSec()
//#define MAP_TIMESEC frozenTime()
//#define MAP_TIMESEC fastTime()
//#define MAP_TIMESEC slowTime()

// track recent placements to determine camp where
// we'll stick next Eve
#define NUM_RECENT_PLACEMENTS 100

extern RecentPlacement recentPlacements[NUM_RECENT_PLACEMENTS];
extern int nextPlacementIndex;// ring buffer
extern int eveRadiusStart;
extern int eveRadius;
extern GridPos eveLocation;
extern SimpleVector<int> eveSecondaryLocObjectIDs;
extern SimpleVector<GridPos> recentlyUsedPrimaryEvePositions;
extern SimpleVector<int>     recentlyUsedPrimaryEvePositionPlayerIDs;
// when they were place, so they can time out // one hour
extern SimpleVector<double> recentlyUsedPrimaryEvePositionTimes;

extern int eveHomeMarkerObjectID;

// what human-placed stuff, together, counts as a camp
static int campRadius = 20;

extern float minEveCampRespawnAge;
extern int barrierRadius;
extern int barrierOn;
extern int longTermCullEnabled;
extern SimpleVector<int> barrierItemList;
extern FILE *mapChangeLogFile;
extern double mapChangeLogTimeStart;
extern char apocalypseTriggered;
extern int edgeObjectID;// what object is placed on edge of map
extern int currentResponsiblePlayer;

// object ids that occur naturally on map at random, per biome
extern int    	numBiomes;
extern int*		biomes;
extern int*  	specialBiomes;
extern float  	specialBiomeTotalWeight;
extern SimpleVector<int> *  naturalMapIDs;
extern SimpleVector<float> *naturalMapChances;
extern SimpleVector<int> allNaturalMapIDs;

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

extern int randSeed;

// decay slots for contained items start after container slots

extern int maxSecondsForActiveDecayTracking;

extern HashTable<timeSec_t> liveDecayRecordPresentHashTable;
extern HashTable<timeSec_t> liveDecayRecordLastLookTimeHashTable;
extern HashTable<ContRecord> liveDecayRecordLastLookTimeMaxContainedHashTable;

extern HashTable<double> liveMovementEtaTimes;
extern SimpleVector<ChangePosition> mapChangePosSinceLastStep;
extern int apocalypsePossible;
extern OneLife::server::cache::Biome* cachedBiome;


// read from testMap.txt
// unless testMapStale.txt is present
// each line contains data in this order:
// x y biome floor id_and_contained
// id and contained are in CONTAINER OBJECT FORMAT described in protocol.txt
// biome = -1 means use naturally-occurring biome
typedef struct TestMapRecord
{
	int                             x, y;
	int                             biome;
	int                             floor;
	int                             id;
	SimpleVector<int>               contained;
	SimpleVector<SimpleVector<int>> subContained;
} TestMapRecord;

#include "../commonSource/fractalNoise.h"

/*

// four ints to a 16-byte key
void intQuadToKey( int inX, int inY, int inSlot, int inB,
				   unsigned char *outKey ) {
	for( int i=0; i<4; i++ ) {
		int offset = i * 8;
		outKey[i] = ( inX >> offset ) & 0xFF;
		outKey[i+4] = ( inY >> offset ) & 0xFF;
		outKey[i+8] = ( inSlot >> offset ) & 0xFF;
		outKey[i+12] = ( inB >> offset ) & 0xFF;
		}
	}


// two ints to an 8-byte key
void intPairToKey( int inX, int inY, unsigned char *outKey ) {
	for( int i=0; i<4; i++ ) {
		int offset = i * 8;
		outKey[i] = ( inX >> offset ) & 0xFF;
		outKey[i+4] = ( inY >> offset ) & 0xFF;
		}
	}







// one timeSec_t to an 8-byte double value
void timeToValue( timeSec_t inT, unsigned char *outValue ) {


	// pack double time into 8 bytes in whatever endian order the
	// double is stored on this platform

	union{ timeSec_t doubleTime; uint64_t intTime; };

	doubleTime = inT;

	for( int i=0; i<8; i++ ) {
		outValue[i] = ( intTime >> (i * 8) ) & 0xFF;
		}
	}


timeSec_t valueToTime( unsigned char *inValue ) {

	union{ timeSec_t doubleTime; uint64_t intTime; };

	// get bytes back out in same order they were put in
	intTime =
		(uint64_t)inValue[7] << 56 | (uint64_t)inValue[6] << 48 |
		(uint64_t)inValue[5] << 40 | (uint64_t)inValue[4] << 32 |
		(uint64_t)inValue[3] << 24 | (uint64_t)inValue[2] << 16 |
		(uint64_t)inValue[1] << 8  | (uint64_t)inValue[0];

	// caste back to timeSec_t
	return doubleTime;
	}
*/

void setResponsiblePlayer(int inPlayerID)
{
	currentResponsiblePlayer = inPlayerID;
}

static void eveDBPut(const char *inEmail, int inX, int inY, int inRadius)
{
	unsigned char key[50];
	unsigned char value[12];

	emailToKey(inEmail, key);

	intToValue(inX, &(value[0]));
	intToValue(inY, &(value[4]));
	intToValue(inRadius, &(value[8]));

	DB_put(&eveDB, key, value);
}

// gets procedurally-generated base map at a given spot
// player modifications are overlayed on top of this

// SIDE EFFECT:
// if biome at x,y needed to be determined in order to compute map
// at this spot, it is saved into lastCheckedBiome

extern int lastCheckedBiome;
extern int lastCheckedBiomeX;
extern int lastCheckedBiomeY;

#include "minorGems/graphics/Image.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"
#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"

void printObjectSamples()
{
	int objectToCount = 2285;

	JenkinsRandomSource sampleRandSource;

	int numSamples = 0;

	int range = 500;

	int count = 0;

	for (int y = -range; y < range; y++)
	{
		for (int x = -range; x < range; x++)
		{
			int obj = getMapObjectRaw(x, y);

			if (obj == objectToCount) { count++; }
			numSamples++;
		}
	}

	int rangeSize = (range + range) * (range + range);

	float sampleFraction = numSamples / (float)rangeSize;

	printf("Counted %d objects in %d/%d samples, expect %d total\n",
		count,
		numSamples,
		rangeSize,
		(int)(count / sampleFraction));
}

extern DBTimeCacheRecord dbTimeCache[];

extern char skipLookTimeCleanup;
extern char skipRemovedObjectCleanup;
extern int cellsLookedAtToInit;
extern char skipTrackingMapChanges;



// reads lines from inFile until EOF reached or inTimeLimitSec passes
// leaves file pos at end of last line read, ready to read more lines
// on future calls
// returns true if there's more file to read, or false if end of file reached
char loadIntoMapFromFile(FILE *inFile, int inOffsetX, int inOffsetY, double inTimeLimitSec)
{

	skipTrackingMapChanges = true;

	double startTime = Time::getCurrentTime();

	char moreFileLeft = true;

	// break out when read fails
	// or if time limit passed
	while (inTimeLimitSec == 0 || Time::getCurrentTime() < startTime + inTimeLimitSec)
	{

		TestMapRecord r;

		char stringBuff[1000];

		int numRead = fscanf(inFile, "%d %d %d %d %999s", &(r.x), &(r.y), &(r.biome), &(r.floor), stringBuff);

		if (numRead != 5)
		{
			moreFileLeft = false;
			break;
		}
		r.x += inOffsetX;
		r.y += inOffsetY;

		int numSlots;

		char **slots = split(stringBuff, ",", &numSlots);

		for (int i = 0; i < numSlots; i++)
		{

			if (i == 0) { r.id = atoi(slots[0]); }
			else
			{

				int    numSub;
				char **subSlots = split(slots[i], ":", &numSub);

				for (int j = 0; j < numSub; j++)
				{
					if (j == 0)
					{
						int contID = atoi(subSlots[0]);

						if (numSub > 1) { contID *= -1; }

						r.contained.push_back(contID);
						SimpleVector<int> subVec;

						r.subContained.push_back(subVec);
					}
					else
					{
						SimpleVector<int> *subVec = r.subContained.getElement(i - 1);
						subVec->push_back(atoi(subSlots[j]));
					}
					delete[] subSlots[j];
				}
				delete[] subSlots;
			}

			delete[] slots[i];
		}
		delete[] slots;

		// set all test map directly in database
		biomeDBPut(r.x, r.y, r.biome, r.biome, 0.5);

		dbFloorPut(r.x, r.y, r.floor);

		setMapObject(r.x, r.y, r.id);

		int *contArray = r.contained.getElementArray();

		setContained(r.x, r.y, r.contained.size(), contArray);
		delete[] contArray;

		for (int c = 0; c < r.contained.size(); c++)
		{

			int *subContArray = r.subContained.getElement(c)->getElementArray();

			setContained(r.x, r.y, r.subContained.getElement(c)->size(), subContArray, c + 1);

			delete[] subContArray;
		}
	}

	skipTrackingMapChanges = false;

	return moreFileLeft;
}

extern SimpleVector<GridPos> *speechPipesIn;
extern SimpleVector<GridPos> *speechPipesOut;
extern SimpleVector<GridPos> flightLandingPos;

SimpleVector<GridPos> *getSpeechPipesOut(int inIndex)
{
	// first, filter them to make sure they are all still here
	for (int p = 0; p < speechPipesOut[inIndex].size(); p++)
	{

		GridPos outPos = speechPipesOut[inIndex].getElementDirect(p);
		// make sure pipe-out is still here
		int id = getMapObjectRaw(outPos.x, outPos.y);

		char stillHere = false;

		if (id > 0)
		{
			ObjectRecord *oOut = getObject(id);

			if (oOut->speechPipeOut && oOut->speechPipeIndex == inIndex) { stillHere = true; }
		}
		if (!stillHere)
		{
			speechPipesOut[inIndex].deleteElement(p);
			p--;
		}
	}

	return &(speechPipesOut[inIndex]);
}

char isMapObjectInTransit(int inX, int inY)
{
	char found;

	double etaTime = liveMovementEtaTimes.lookup(inX, inY, 0, 0, &found);

	if (found)
	{
		if (etaTime > Time::getCurrentTime()) { return true; }
	}

	return false;
}

void freeAndNullString(char **inStringPointer)
{
	if (*inStringPointer != NULL)
	{
		delete[] * inStringPointer;
		*inStringPointer = NULL;
	}
}

static char tooClose(GridPos inA, GridPos inB, int inMinComponentDistance)
{
	double xDist = (double)inA.x - (double)inB.x;
	if (xDist < 0) { xDist = -xDist; }
	double yDist = (double)inA.y - (double)inB.y;
	if (yDist < 0) { yDist = -yDist; }

	if (xDist < inMinComponentDistance && yDist < inMinComponentDistance) { return true; }
	return false;
}

void logMapChange(int inX, int inY, int inID)
{
	// log it?
	if (mapChangeLogFile != NULL)
	{

		ObjectRecord *o = getObject(inID);

		const char *extraFlag = "";

		if (o != NULL && o->floor) { extraFlag = "f"; }

		if (o != NULL && o->isUseDummy)
		{
			fprintf(mapChangeLogFile,
				"%.2f %d %d %s%du%d\n",
				Time::getCurrentTime() - mapChangeLogTimeStart,
				inX,
				inY,
				extraFlag,
				o->useDummyParent,
				o->thisUseDummyIndex);
		}
		else if (o != NULL && o->isVariableDummy)
		{
			fprintf(mapChangeLogFile,
				"%.2f %d %d %s%dv%d\n",
				Time::getCurrentTime() - mapChangeLogTimeStart,
				inX,
				inY,
				extraFlag,
				o->variableDummyParent,
				o->thisVariableDummyIndex);
		}
		else
		{
			fprintf(mapChangeLogFile,
				"%.2f %d %d %s%d\n",
				Time::getCurrentTime() - mapChangeLogTimeStart,
				inX,
				inY,
				extraFlag,
				inID);
		}
	}
}

// removes from top of stack
int removeContained(int inX, int inY, int inSlot, timeSec_t *outEtaDecay, int inSubCont)
{
	int num = getNumContained(inX, inY, inSubCont);

	if (num == 0) { return 0; }

	if (inSlot == -1 || inSlot > num - 1) { inSlot = num - 1; }

	int result = dbGet(inX, inY, FIRST_CONT_SLOT + inSlot, inSubCont);

	timeSec_t curTime = MAP_TIMESEC;

	timeSec_t resultEta = dbTimeGet(inX, inY, getContainerDecaySlot(inX, inY, inSlot, inSubCont, num), inSubCont);

	if (resultEta != 0)
	{
		timeSec_t etaOffset = resultEta - curTime;

		etaOffset = etaOffset * getMapContainerTimeStretch(inX, inY);

		resultEta = curTime + etaOffset;
	}

	*outEtaDecay = resultEta;

	int  oldNum;
	int *oldContained = getContained(inX, inY, &oldNum, inSubCont);

	timeSec_t *oldContainedETA = getContainedEtaDecay(inX, inY, &oldNum, inSubCont);

	SimpleVector<int>       newContainedList;
	SimpleVector<timeSec_t> newContainedETAList;

	SimpleVector<int>         newSubContainedNumList;
	SimpleVector<int *>       newSubContainedList;
	SimpleVector<timeSec_t *> newSubContainedEtaList;

	for (int i = 0; i < oldNum; i++)
	{
		if (i != inSlot)
		{
			newContainedList.push_back(oldContained[i]);
			newContainedETAList.push_back(oldContainedETA[i]);

			if (inSubCont == 0)
			{
				int num;

				newSubContainedList.push_back(getContained(inX, inY, &num, i + 1));
				newSubContainedNumList.push_back(num);

				newSubContainedEtaList.push_back(getContainedEtaDecay(inX, inY, &num, i + 1));
			}
		}
	}
	clearAllContained(inX, inY);

	int *      newContained    = newContainedList.getElementArray();
	timeSec_t *newContainedETA = newContainedETAList.getElementArray();

	int newNum = oldNum - 1;

	setContained(inX, inY, newNum, newContained, inSubCont);
	setContainedEtaDecay(inX, inY, newNum, newContainedETA, inSubCont);

	if (inSubCont == 0)
	{
		for (int i = 0; i < newNum; i++)
		{
			int *      idList  = newSubContainedList.getElementDirect(i);
			timeSec_t *etaList = newSubContainedEtaList.getElementDirect(i);

			if (idList != NULL)
			{
				int num = newSubContainedNumList.getElementDirect(i);

				setContained(inX, inY, num, idList, i + 1);
				setContainedEtaDecay(inX, inY, num, etaList, i + 1);

				delete[] idList;
				delete[] etaList;
			}
		}
	}

	delete[] oldContained;
	delete[] oldContainedETA;
	delete[] newContained;
	delete[] newContainedETA;

	if (result != -1) { return result; }
	else
	{
		// nothing in that slot
		return 0;
	}
}

char *getMapChangeLineString(ChangePosition inPos)
{
	MapChangeRecord r = getMapChangeRecord(inPos);

	char *lineString = getMapChangeLineString(&r, 0, 0);

	delete[] r.formatString;

	return lineString;
}

char *getMapChangeLineString(MapChangeRecord *inRecord, int inRelativeToX, int inRelativeToY)
{

	char *lineString;

	if (inRecord->oldCoordsUsed)
	{
		lineString = autoSprintf(inRecord->formatString,
			inRecord->absoluteX - inRelativeToX,
			inRecord->absoluteY - inRelativeToY,
			inRecord->absoluteOldX - inRelativeToX,
			inRecord->absoluteOldY - inRelativeToY);
	}
	else
	{
		lineString = autoSprintf(
			inRecord->formatString, inRecord->absoluteX - inRelativeToX, inRecord->absoluteY - inRelativeToY);
	}

	return lineString;
}

doublePair computeRecentCampAve(int *outNumPosFound)
{
	SimpleVector<doublePair> pos;
	SimpleVector<double>     weight;

	doublePair sum = {0, 0};

	double weightSum = 0;

	// the exponent that we raise depth to in order to squash
	// down higher values
	double depthFactor = 0.5;

	for (int i = 0; i < NUM_RECENT_PLACEMENTS; i++)
	{
		if (recentPlacements[i].pos.x != 0 || recentPlacements[i].pos.y != 0)
		{

			doublePair p = {(double)(recentPlacements[i].pos.x), (double)(recentPlacements[i].pos.y)};

			pos.push_back(p);

			// natural objects can be moved around, and they have depth 0
			// this can result in a total weight sum of 0, causing NAN
			// push all depths up to 1 or greater
			int d = recentPlacements[i].depth + 1;

			double w = pow(d, depthFactor);

			weight.push_back(w);

			// weighted sum, with deeper objects weighing more
			sum = add(sum, mult(p, w));

			weightSum += w;
		}
	}

	*outNumPosFound = pos.size();

	if (pos.size() == 0)
	{
		doublePair zeroPos = {0, 0};
		pos.push_back(zeroPos);
		weight.push_back(1);
		weightSum += 1;
	}

	doublePair ave = mult(sum, 1.0 / weightSum);

	double maxDist = 2.0 * campRadius;

	while (maxDist > campRadius)
	{

		maxDist  = 0;
		int maxI = -1;

		for (int i = 0; i < pos.size(); i++)
		{

			double d = distance(pos.getElementDirect(i), ave);

			if (d > maxDist)
			{
				maxDist = d;
				maxI    = i;
			}
		}

		if (maxDist > campRadius)
		{

			double w = weight.getElementDirect(maxI);

			sum = sub(sum, mult(pos.getElementDirect(maxI), w));

			pos.deleteElement(maxI);
			weight.deleteElement(maxI);

			weightSum -= w;

			ave = mult(sum, 1.0 / weightSum);
		}
	}

	printf("Found an existing camp at (%f,%f) with %d placements "
		   "and %f max radius\n",
		ave.x,
		ave.y,
		pos.size(),
		maxDist);

	// ave is now center of camp
	return ave;
}

void mapEveDeath(const char *inEmail, double inAge, GridPos inDeathMapPos)
{

	// record exists?

	int pX, pY, pR;

	pR = eveRadius;

	printf("Logging Eve death:   ");

	if (inAge < minEveCampRespawnAge)
	{
		printf("Eve died too young (age=%f, min=%f), "
			   "not remembering her camp, and clearing any old camp memory\n",
			inAge,
			minEveCampRespawnAge);

		// 0 for radius means not set
		eveDBPut(inEmail, 0, 0, 0);

		return;
	}

	int result = eveDBGet(inEmail, &pX, &pY, &pR);

	if (result == 1 && pR > 0)
	{

		// don't keep growing radius after it gets too big
		// if one player is dying young over and over, they will
		// eventually overflow 32-bit integers

		if (inAge < 16 && pR < 1024) { pR *= 2; }
		else if (inAge > 20)
		{
			pR = eveRadiusStart;
		}
	}
	else
	{
		// not found in DB

		// must overwrite no matter what
		pR = eveRadiusStart;
	}

	// their next camp will start where they last died
	pX = inDeathMapPos.x;
	pY = inDeathMapPos.y;

	printf("Remembering Eve's camp in database (%d,%d) r=%d for %s\n", pX, pY, pR, inEmail);

	eveDBPut(inEmail, pX, pY, pR);
}

static unsigned int nextLoadID = 0;

char loadTutorialStart(TutorialLoadProgress *inTutorialLoad, const char *inMapFileName, int inX, int inY)
{

	// don't open file yet, because we don't want to have the same
	// file open in parallel

	// save info to open file on first step, which is called one player at a
	// time
	inTutorialLoad->uniqueLoadID = nextLoadID++;
	inTutorialLoad->fileOpened   = false;
	inTutorialLoad->file         = NULL;
	inTutorialLoad->mapFileName  = stringDuplicate(inMapFileName);
	inTutorialLoad->x            = inX;
	inTutorialLoad->y            = inY;
	inTutorialLoad->startTime    = Time::getCurrentTime();
	inTutorialLoad->stepCount    = 0;

	return true;
}

char loadTutorialStep(TutorialLoadProgress *inTutorialLoad, double inTimeLimitSec)
{

	if (!inTutorialLoad->fileOpened)
	{
		// first step, open file

		char returnVal = false;

		// only try opening it once
		inTutorialLoad->fileOpened = true;

		File tutorialFolder(NULL, "tutorialMaps");

		if (tutorialFolder.exists() && tutorialFolder.isDirectory())
		{

			File *mapFile = tutorialFolder.getChildFile(inTutorialLoad->mapFileName);

			if (mapFile->exists() && !mapFile->isDirectory())
			{
				char *fileName = mapFile->getFullFileName();

				FILE *file = fopen(fileName, "r");

				if (file != NULL)
				{
					inTutorialLoad->file = file;

					returnVal = true;
				}

				delete[] fileName;
			}
			delete mapFile;
		}

		delete[] inTutorialLoad->mapFileName;

		return returnVal;
	}

	// else file already open

	if (inTutorialLoad->file == NULL)
	{
		// none left
		return false;
	}

	char moreLeft = loadIntoMapFromFile(inTutorialLoad->file, inTutorialLoad->x, inTutorialLoad->y, inTimeLimitSec);

	inTutorialLoad->stepCount++;

	if (!moreLeft)
	{
		fclose(inTutorialLoad->file);
		inTutorialLoad->file = NULL;
	}
	return moreLeft;
}

char getMetadata(int inMapID, unsigned char *inBuffer)
{
	int metaID = extractMetadataID(inMapID);

	if (metaID == 0) { return false; }

	// look up in metadata DB
	unsigned char key[4];
	intToValue(metaID, key);
	int result = DB_get(&metaDB, key, inBuffer);

	if (result == 0) { return true; }

	return false;
}

// returns full map ID with embedded metadata ID for new metadata record
int addMetadata(int inObjectID, unsigned char *inBuffer)
{
	int metaID = getNewMetadataID();

	int mapID = packMetadataID(inObjectID, metaID);

	// insert into metadata DB
	unsigned char key[4];
	intToValue(metaID, key);
	DB_put(&metaDB, key, inBuffer);

	return mapID;
}

static double distSquared(GridPos inA, GridPos inB)
{
	double xDiff = (double)inA.x - (double)inB.x;
	double yDiff = (double)inA.y - (double)inB.y;

	return xDiff * xDiff + yDiff * yDiff;
}

char isInDir(GridPos inPos, GridPos inOtherPos, doublePair inDir)
{
	double dX = (double)inOtherPos.x - (double)inPos.x;
	double dY = (double)inOtherPos.y - (double)inPos.y;

	if (inDir.x > 0 && dX > 0) { return true; }
	if (inDir.x < 0 && dX < 0) { return true; }
	if (inDir.y > 0 && dY > 0) { return true; }
	if (inDir.y < 0 && dY < 0) { return true; }
	return false;
}

GridPos getNextCloseLandingPos(GridPos inCurPos, doublePair inDir, char *outFound)
{

	int     closestIndex = -1;
	GridPos closestPos;
	double  closestDist = DBL_MAX;

	for (int i = 0; i < flightLandingPos.size(); i++)
	{
		GridPos thisPos = flightLandingPos.getElementDirect(i);

		if (tooClose(inCurPos, thisPos, 250))
		{
			// don't consider landing at spots closer than 250,250 manhattan
			// to takeoff spot
			continue;
		}

		if (isInDir(inCurPos, thisPos, inDir))
		{
			double dist = distSquared(inCurPos, thisPos);

			if (dist < closestDist)
			{
				// check if this is still a valid landing pos
				int oID = getMapObject(thisPos.x, thisPos.y);

				if (oID <= 0 || !getObject(oID)->isFlightLanding)
				{

					// not even a valid landing pos anymore
					flightLandingPos.deleteElement(i);
					i--;
					continue;
				}
				closestDist  = dist;
				closestPos   = thisPos;
				closestIndex = i;
			}
		}
	}

	if (closestIndex == -1) { *outFound = false; }
	else
	{
		*outFound = true;
	}

	return closestPos;
}

GridPos getNextFlightLandingPos(int inCurrentX, int inCurrentY, doublePair inDir, int inRadiusLimit)
{
	int     closestIndex = -1;
	GridPos closestPos;
	double  closestDist = DBL_MAX;

	GridPos curPos = {inCurrentX, inCurrentY};

	char useLimit = false;

	if (abs(inCurrentX) <= inRadiusLimit && abs(inCurrentY) <= inRadiusLimit) { useLimit = true; }

	for (int i = 0; i < flightLandingPos.size(); i++)
	{
		GridPos thisPos = flightLandingPos.getElementDirect(i);

		if (useLimit && (abs(thisPos.x) > inRadiusLimit || abs(thisPos.x) > inRadiusLimit))
		{
			// out of bounds destination
			continue;
		}

		double dist = distSquared(curPos, thisPos);

		if (dist < closestDist)
		{

			// check if this is still a valid landing pos
			int oID = getMapObject(thisPos.x, thisPos.y);

			if (oID <= 0 || !getObject(oID)->isFlightLanding)
			{

				// not even a valid landing pos anymore
				flightLandingPos.deleteElement(i);
				i--;
				continue;
			}
			closestDist  = dist;
			closestPos   = thisPos;
			closestIndex = i;
		}
	}

	if (closestIndex != -1 && flightLandingPos.size() > 1)
	{
		// found closest, and there's more than one
		// look for next valid position in chosen direction

		char found = false;

		GridPos nextPos = getNextCloseLandingPos(curPos, inDir, &found);

		if (found) { return nextPos; }

		// if we got here, we never found a nextPos that was valid
		// closestPos is only option
		return closestPos;
	}
	else if (closestIndex != -1 && flightLandingPos.size() == 1)
	{
		// land at closest, only option
		return closestPos;
	}

	// got here, no place to land

	// crash them at next Eve location

	int eveX, eveY;

	SimpleVector<GridPos> otherPeoplePos;

	getEvePosition("dummyPlaneCrashEmail@test.com", 0, &eveX, &eveY, &otherPeoplePos, false);

	GridPos returnVal = {eveX, eveY};

	if (inRadiusLimit > 0 && (abs(eveX) >= inRadiusLimit || abs(eveY) >= inRadiusLimit))
	{
		// even Eve pos is out of bounds
		// stick them in center
		returnVal.x = 0;
		returnVal.y = 0;
	}

	return returnVal;
}

int getGravePlayerID(int inX, int inY)
{
	unsigned char key[9];
	unsigned char value[4];

	// look for changes to default in database
	intPairToKey(inX, inY, key);

	int result = DB_get(&graveDB, key, value);

	if (result == 0)
	{
		// found
		int returnVal = valueToInt(value);

		return returnVal;
	}
	else
	{
		return 0;
	}
}

void setGravePlayerID(int inX, int inY, int inPlayerID)
{
	unsigned char key[8];
	unsigned char value[4];

	intPairToKey(inX, inY, key);
	intToValue(inPlayerID, value);

	DB_put(&graveDB, key, value);
}

static char        tileCullingIteratorSet = false;
static DB_Iterator tileCullingIterator;

static char        floorCullingIteratorSet = false;
static DB_Iterator floorCullingIterator;

static double lastSettingsLoadTime = 0;
static double settingsLoadInterval = 5 * 60;

static int numTilesExaminedPerCullStep = 10;
static int longTermCullingSeconds      = 3600 * 12;

static int minActivePlayersForLongTermCulling = 15;

static SimpleVector<int> noCullItemList;

static int numTilesSeenByIterator  = 0;
static int numFloorsSeenByIterator = 0;

void stepMapLongTermCulling(int inNumCurrentPlayers)
{

	double curTime = Time::getCurrentTime();

	if (curTime - lastSettingsLoadTime > settingsLoadInterval)
	{

		lastSettingsLoadTime = curTime;

		numTilesExaminedPerCullStep        = SettingsManager::getIntSetting("numTilesExaminedPerCullStep", 10);
		longTermCullingSeconds             = SettingsManager::getIntSetting("longTermNoLookCullSeconds", 3600 * 12);
		minActivePlayersForLongTermCulling = SettingsManager::getIntSetting("minActivePlayersForLongTermCulling", 15);

		longTermCullEnabled = SettingsManager::getIntSetting("longTermNoLookCullEnabled", 1);

		SimpleVector<int> *list = SettingsManager::getIntSettingMulti("noCullItemList");

		noCullItemList.deleteAll();
		noCullItemList.push_back_other(list);
		delete list;

		barrierRadius = SettingsManager::getIntSetting("barrierRadius", 250);
		barrierOn     = SettingsManager::getIntSetting("barrierOn", 1);
	}

	if (!longTermCullEnabled || minActivePlayersForLongTermCulling > inNumCurrentPlayers) { return; }

	if (!tileCullingIteratorSet)
	{
		DB_Iterator_init(&db, &tileCullingIterator);
		tileCullingIteratorSet = true;
		numTilesSeenByIterator = 0;
	}

	unsigned char tileKey[16];
	unsigned char floorKey[8];
	unsigned char value[4];

	for (int i = 0; i < numTilesExaminedPerCullStep; i++)
	{
		int result = DB_Iterator_next(&tileCullingIterator, tileKey, value);

		if (result <= 0)
		{
			// restart the iterator back at the beginning
			DB_Iterator_init(&db, &tileCullingIterator);
			if (numTilesSeenByIterator != 0)
			{ AppLog::infoF("Map cull iterated through %d tile db entries.", numTilesSeenByIterator); }
			numTilesSeenByIterator = 0;
			// end loop when we reach end of list, so we don't cycle through
			// a short iterator list too quickly.
			break;
		}
		else
		{
			numTilesSeenByIterator++;
		}

		int tileID = valueToInt(value);

		// consider 0-values too, where map has been cleared by players, but
		// a natural object should be there
		if (tileID >= 0)
		{
			// next value

			int s = valueToInt(&(tileKey[8]));
			int b = valueToInt(&(tileKey[12]));

			if (s == 0 && b == 0)
			{
				// main object
				int x = valueToInt(tileKey);
				int y = valueToInt(&(tileKey[4]));

				int wildTile = getTweakedBaseMap(x, y);

				if (wildTile != tileID)
				{
					// tile differs from natural tile
					// don't keep checking/resetting tiles that are already
					// in wild state

					// NOTE that we don't check/clear container slots for
					// already-wild tiles.  So a natural container
					// (if one is ever
					// added to the game, like a hidey-hole cave) will
					// keep its items even after that part of the map
					// is culled.  Seems like okay behavior.

					timeSec_t lastLookTime = dbLookTimeGet(x, y);

					if (curTime - lastLookTime > longTermCullingSeconds)
					{
						// stale

						if (noCullItemList.getElementIndex(tileID) == -1)
						{
							// not on our no-cull list
							clearAllContained(x, y);

							// put proc-genned map value in there
							setMapObject(x, y, wildTile);

							if (wildTile != 0 && getObject(wildTile)->permanent)
							{
								// something nautural occurs here
								// this "breaks" any remaining floor
								// (which may be cull-proof on its own below).
								// this will effectively leave gaps in roads
								// with trees growing through, etc.
								setMapFloor(x, y, 0);
							}
						}
					}
				}
			}
		}
	}

	if (!floorCullingIteratorSet)
	{
		DB_Iterator_init(&floorDB, &floorCullingIterator);
		floorCullingIteratorSet = true;
		numFloorsSeenByIterator = 0;
	}

	for (int i = 0; i < numTilesExaminedPerCullStep; i++)
	{
		int result = DB_Iterator_next(&floorCullingIterator, floorKey, value);

		if (result <= 0)
		{
			// restart the iterator back at the beginning
			DB_Iterator_init(&floorDB, &floorCullingIterator);
			if (numFloorsSeenByIterator != 0)
			{ AppLog::infoF("Map cull iterated through %d floor db entries.", numFloorsSeenByIterator); }
			numFloorsSeenByIterator = 0;
			// end loop now, avoid re-cycling through a short list
			// in same step
			break;
		}
		else
		{
			numFloorsSeenByIterator++;
		}

		int floorID = valueToInt(value);

		if (floorID > 0)
		{
			// next value

			int x = valueToInt(floorKey);
			int y = valueToInt(&(floorKey[4]));

			timeSec_t lastLookTime = dbLookTimeGet(x, y);

			if (curTime - lastLookTime > longTermCullingSeconds)
			{
				// stale

				if (noCullItemList.getElementIndex(floorID) == -1)
				{
					// not on our no-cull list

					setMapFloor(x, y, 0);
				}
			}
		}
	}
}