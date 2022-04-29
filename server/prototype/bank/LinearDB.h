//
// Created by olivier on 14/04/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_DATABASE_LINEARDB_H
#define ONELIFE_SERVER_DATATYPE_DATABASE_LINEARDB_H

#include "linearDB/lineardb3.h"

#include <functional>

#include "../../../third_party/minorGems/io/file/File.h"
#include "../../../third_party/minorGems/system/Time.h"
#include "../../../gameSource/GridPos.h"
#include "../../component/feature/apocalypse.h"


#define DB LINEARDB3
#define DB_open LINEARDB3_open
#define DB_close LINEARDB3_close
#define DB_get LINEARDB3_get
#define DB_put LINEARDB3_put
#define DB_put_new LINEARDB3_put// no distinction between put and put_new in lineardb3
#define DB_Iterator LINEARDB3_Iterator
#define DB_Iterator_init LINEARDB3_Iterator_init
#define DB_Iterator_next LINEARDB3_Iterator_next
#define DB_maxStack db.maxOverflowDepth
#define DB_getShrinkSize LINEARDB3_getShrinkSize
#define DB_getCurrentSize LINEARDB3_getCurrentSize
#define DB_getNumRecords LINEARDB3_getNumRecords

#define NUM_RECENT_PLACEMENTS 100
#define BASE_MAP_CACHE_SIZE 256 // should be a power of 2 cache will contain squared number of records
#define BIOME_CACHE_SIZE 131072 // optimization:// cache biomeIndex results in RAM // 3.1 MB of RAM for this.
#define DB_CACHE_SIZE 131072// optimization:// cache dbGet results in RAM// 2.6 MB of RAM for this.
#define NUM_CONT_SLOT 2
#define FIRST_CONT_SLOT 3

#define KISSDB_OPEN_MODE_RWCREAT 3

#define CACHE_PRIME_A 776509273
#define CACHE_PRIME_B 904124281
#define CACHE_PRIME_C 528383237
#define CACHE_PRIME_D 148497157

typedef struct DBTimeCacheRecord
{
	int       x, y, slot, subCont;
	timeSec_t timeVal;
} DBTimeCacheRecord;

typedef struct BiomeCacheRecord
{
	int    x, y;
	int    biome, secondPlace;
	double secondPlaceGap;
} BiomeCacheRecord;

typedef struct BlockingCacheRecord
{
	int x, y;
	// -1 if not present
	char blocking;
} BlockingCacheRecord;

typedef struct DBCacheRecord
{
	int x, y, slot, subCont;
	int value;
} DBCacheRecord;

typedef struct RecentPlacement
{
	GridPos pos;
	int depth;// depth of object in tech tree
} RecentPlacement;

typedef struct RawRecord
{
	unsigned char* key;
	unsigned char* value;
}RawRecord;

namespace OneLife::server::bank::linearDB
{
	typedef struct
	{
		char* path;
		int mode;
		unsigned long hash_table_size;
		unsigned long key_size;
		unsigned long value_size;
	}Settings;
}

namespace OneLife::server::bank
{
	class LinearDB
	{
		public:
			LinearDB(
				 const char* path,
				 int mode,
				 unsigned long hash_table_size,
				 unsigned long key_size,
				 unsigned long value_size);
			~LinearDB();

			//!open close
			void open(DB* db=nullptr, char* ptrDbOpenStatus=nullptr, char* ptrEmptyStatus=nullptr);
			void close();

			//!

			//!status
			OneLife::server::bank::linearDB::Settings getSettings();
			uint32_t getSize();
			bool isEmpty();
			bool isEnable();

			//!public operation (record management)
			void copy(OneLife::server::bank::LinearDB* srcDB);
			void insert(RawRecord rawRecord);
			void iterate(std::function<void(RawRecord)> processRawRecord);

			//!public operation (db management)
			virtual void clean() =0;
			virtual unsigned int getRecordNumber() =0;
			void removeDBFile();

		protected:
			DB* db;
			char* dbEmpty;
			char* dbOpen;
			DB_Iterator* dbi;
			OneLife::server::bank::linearDB::Settings settings;
			RawRecord rawRecord;
			File* dbFile;
			unsigned long int recordNumber;
			struct{
				bool enabled;
				bool empty;
			}status;

	};
}

int dbGet(int inX, int inY, int inSlot, int inSubCont = 0);
void dbPutCached(int inX, int inY, int inSlot, int inSubCont, int inValue);
int dbGetCached(int inX, int inY, int inSlot, int inSubCont);
int computeDBCacheHash(int inKeyA, int inKeyB, int inKeyC, int inKeyD);
void blockingClearCached(int inX, int inY);
int computeXYCacheHash(int inKeyA, int inKeyB);
int eveDBGet(const char *inEmail, int *outX, int *outY, int *outRadius);

#endif //ONELIFE_SERVER_DATATYPE_DATABASE_LINEARDB_H
