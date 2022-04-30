//
// Created by olivier on 25/04/2022.
//

#include "Biome.h"

#include "../../../commonSource/Debug.h"
#include "../../dbCommon.h"

DB biomeDB;
char biomeDBOpen = false;
double gapIntScale = 1000000.0;
char anyBiomesInDB = false;
int  maxBiomeXLoc  = -2000000000;
int  maxBiomeYLoc  = -2000000000;
int  minBiomeXLoc  = 2000000000;
int  minBiomeYLoc  = 2000000000;

OneLife::server::database::Biome::Biome(
		const char* path,
		int mode,
		unsigned long hash_table_size,
		unsigned long key_size,
		unsigned long value_size):
		OneLife::server::bank::LinearDB::LinearDB(path, mode, hash_table_size, key_size, value_size)
{
	OneLife::Debug::write("init biomeDB : %s", this->settings.path);
	this->recordNumber = 0;
	this->status.enabled = false;
	this->status.empty = true;
	//this->settings.db = &biomeDB;
}

OneLife::server::database::Biome::~Biome() {}


void OneLife::server::database::Biome::clean() {}

unsigned int OneLife::server::database::Biome::getRecordNumber()
{
	return this->recordNumber;
}

/**********************************************************************************************************************/

/**
 * @note from int_map() in server/map.cpp
 * // see if any biomes are listed in DB
	// if not, we don't even need to check it when generating map
 */
void OneLife::server::database::Biome::enable()
{
	OneLife::Debug::write("Enable biome database");
	int recordNumber = 0;
	int xMin = minBiomeXLoc;
	int xMax = maxBiomeXLoc;
	int yMin = minBiomeYLoc;
	int yMax = maxBiomeYLoc;
	std::function<void(RawRecord)> checkBiomeCell = [&recordNumber, &xMin, &xMax, &yMin, &yMax](RawRecord record) -> void
	{
		recordNumber++;

		int x = valueToInt(record.key);
		int y = valueToInt(&(record.key[4]));

		if (x > xMax) { xMax = x; }
		if (x < xMin) { xMin = x; }
		if (y > yMax) { yMax = y; }
		if (y < yMin) { yMin = y; }

	};
	this->iterate(checkBiomeCell);
	this->recordNumber = recordNumber;
	minBiomeXLoc = xMin;
	maxBiomeXLoc = xMax;
	minBiomeYLoc = yMin;
	maxBiomeYLoc = yMax;

	if(this->recordNumber) anyBiomesInDB = true;
	printf("\n---------------------------------------------------------------\nMin (x,y) of biome in db = (%d,%d), "
		   "Max (x,y) of biome in db = (%d,%d)\nrecordNumber:%lu\n",
		   minBiomeXLoc,
		   minBiomeYLoc,
		   maxBiomeXLoc,
		   maxBiomeYLoc,
		   this->recordNumber);
}

/**
 *
 * @param inX
 * @param inY
 * @param outSecondPlaceBiome
 * @param outSecondPlaceGap
 * @return
 * @note from server/map.cpp
 * // => returns -1 if not found
 * // => don't bother with this call unless biome DB has
		// something in it, and this inX,inY is in the region where biomes
		// exist in the database (tutorial loading, or test maps)
 */
int biomeDBGet(int inX, int inY, int *outSecondPlaceBiome, double *outSecondPlaceGap)
{
	int biome;
	if (!anyBiomesInDB || inX < minBiomeXLoc || inX > maxBiomeXLoc || inY < minBiomeYLoc || inY > maxBiomeYLoc)
	{
		biome = -1;
	}
	else
	{
		unsigned char key[8];
		unsigned char value[12];

		// look for changes to default in database
		intPairToKey(inX, inY, key);

		//int result = DB_get(&biomeDB, key, value);
		int result = LINEARDB3_get(&biomeDB, key, value);

		if (result == 0)
		{
			// found
			biome = valueToInt(&(value[0]));
			if (outSecondPlaceBiome != NULL) { *outSecondPlaceBiome = valueToInt(&(value[4])); }
			if (outSecondPlaceGap != NULL) { *outSecondPlaceGap = valueToInt(&(value[8])) / gapIntScale; }
		}
		else
		{
			biome = -1;
		}
	}
	return biome;
}

/**
 *
 * @param inX
 * @param inY
 * @param inValue
 * @param inSecondPlace
 * @param inSecondPlaceGap
 * @note from server/map.cpp
 */
void biomeDBPut(int inX, int inY, int inValue, int inSecondPlace, double inSecondPlaceGap)
{
	unsigned char key[8];
	unsigned char value[12];

	intPairToKey(inX, inY, key);
	intToValue(inValue, &(value[0]));
	intToValue(inSecondPlace, &(value[4]));
	intToValue(lrint(inSecondPlaceGap * gapIntScale), &(value[8]));

	anyBiomesInDB = true;

	if (inX > maxBiomeXLoc) { maxBiomeXLoc = inX; }
	if (inX < minBiomeXLoc) { minBiomeXLoc = inX; }
	if (inY > maxBiomeYLoc) { maxBiomeYLoc = inY; }
	if (inY < minBiomeYLoc) { minBiomeYLoc = inY; }

	DB_put(&biomeDB, key, value);
}
