//
// Created by olivier on 01/05/2022.
//

#include "Blocking.h"


BlockingCacheRecord blockingCache[DB_CACHE_SIZE];

OneLife::server::cache::Blocking::Blocking() {}
OneLife::server::cache::Blocking::~Blocking() {}

/**
 *
 * @param inX
 * @param inY
 * @return
 * @note from server/map.cpp
 * // returns -1 on miss
 */
char blockingGetCached(int inX, int inY)
{
	BlockingCacheRecord r = blockingCache[computeXYCacheHash(inX, inY)];

	if (r.x == inX && r.y == inY && r.blocking != -1) { return r.blocking; }
	else
	{
		return -1;
	}
}

/**
 *
 * @param inX
 * @param inY
 * @param inBlocking
 * @note from server/map.cpp
 */
void blockingPutCached(int inX, int inY, char inBlocking)
{
	BlockingCacheRecord r = {inX, inY, inBlocking};

	blockingCache[computeXYCacheHash(inX, inY)] = r;
}

/**
 * @note from server/map.cpp
 */
void blockingClearCached(int inX, int inY)
{

	BlockingCacheRecord *r = &(blockingCache[computeXYCacheHash(inX, inY)]);

	if (r->x == inX && r->y == inY) { r->blocking = -1; }
}