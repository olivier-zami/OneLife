//
// Created by olivier on 28/04/2022.
//

#include "LookTime.h"

#include "../../../commonSource/Debug.h"
#include "../../dbCommon.h"

DB lookTimeDB;// tracking when a given map cell was last seen
char lookTimeDBOpen = false;
char lookTimeDBEmpty = false;

OneLife::server::database::LookTime::LookTime(
		const char *path,
		int mode,
		unsigned long hash_table_size,
		unsigned long key_size,
		unsigned long value_size
		): OneLife::server::bank::LinearDB(path, mode, hash_table_size, key_size, value_size)
{}

OneLife::server::database::LookTime::~LookTime() {}

void OneLife::server::database::LookTime::setStaleDuration(int staleInSecond)
{
	this->staleInSecond = staleInSecond;
}

/**
 * @note from initMap in map.cpp
 */
void OneLife::server::database::LookTime::clean()
{
	OneLife::server::bank::LinearDB* oldDB = nullptr;
	oldDB = new OneLife::server::database::LookTime(
			this->settings.path,//settings.database.lookTime.url,
			this->settings.mode,//KISSDB_OPEN_MODE_RWCREAT,
			this->settings.hash_table_size,//80000,
			this->settings.key_size,//8, // two 32-bit ints, xy
			this->settings.value_size//8 // one 64-bit double, representing an ETA time in whatever binary format and byte order "double" on the server platform uses
	);

	oldDB->open();
	if(oldDB->isEnable())
	{
		OneLife::Debug::write("\nCleaning stale look times from map...");
		int total    = 0;
		int stale    = 0;
		int nonStale = 0;
		timeSec_t curTime = Time::timeSec();
		int staleLimit = this->staleInSecond;
		std::function<void(RawRecord)> checkInvalidRecord = [&total, &stale, &nonStale, curTime, staleLimit](RawRecord record)->void
		{
			total++;
			timeSec_t t = valueToTime(record.value);
			if (curTime-t >= staleLimit) stale++;// stale cell =>ignore
			else nonStale++;
		};
		oldDB->iterate(checkInvalidRecord);
		if (total == 0) { lookTimeDBEmpty = true; }//TODO remove if not used

		// optimial size for DB of remaining elements
		unsigned int newSize = LINEARDB3_getPerfectTableSize( 0.80, nonStale );//TODO: create DB_GET_PERFECT_TABLE_SIZE ? take 0.80 settings.maxLoad cf LINEARDB3_setMaxLoad(0.80);

		OneLife::Debug::write("Shrinking hash table for lookTimes from %d down to %d",
					  oldDB->getSize(),
					  newSize);

		//!
		const char* suffix = "_temp";
		size_t tmpDbFileNameLength = strlen(this->settings.path)+strlen(suffix)+1;
		char *tmpDbFileName = (char*)malloc(tmpDbFileNameLength*sizeof(char));
		memset(tmpDbFileName, 0, tmpDbFileNameLength);
		char *ptr = tmpDbFileName;
		strncpy(ptr, this->settings.path, strlen(this->settings.path));
		ptr += strlen(this->settings.path);
		strncpy(ptr, suffix, strlen(suffix));

		//!
		OneLife::server::bank::LinearDB* tmpDB = nullptr;
		tmpDB = new OneLife::server::database::LookTime(
				tmpDbFileName,
				KISSDB_OPEN_MODE_RWCREAT,
				80000,
				8,
				8
		);
		free(tmpDbFileName);
		tmpDB->removeDBFile();

		tmpDB->open();
		if(tmpDB->isEnable())
		{
			timeSec_t curTime = Time::timeSec();
			int staleSecLimit = this->staleInSecond;
			std::function<void(RawRecord)> writeIn = [tmpDB, curTime, staleSecLimit](RawRecord rawRecord)->void
			{
				if (curTime - valueToTime(rawRecord.value) < staleSecLimit) tmpDB->insert(rawRecord);
			};
			oldDB->iterate(writeIn);
		}
		else
		{
			int error = 0;
			OneLife::Debug::write("Error %d opening look time temp KissDB", error);
			//return false;
		}

		OneLife::Debug::write("Cleaned %d / %d stale look times", stale, total);

		tmpDB->close();

		this->copy(tmpDB);
		tmpDB->removeDBFile();
	}
	oldDB->close();
}

/**
 *
 * @return
 */
unsigned int OneLife::server::database::LookTime::getRecordNumber()
{
	return 0;//TODO return record number
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