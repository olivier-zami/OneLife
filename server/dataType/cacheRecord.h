//
// Created by olivier on 01/05/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_CACHERECORD_H
#define ONELIFE_SERVER_DATATYPE_CACHERECORD_H

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

#endif //ONELIFE_SERVER_DATATYPE_CACHERECORD_H
