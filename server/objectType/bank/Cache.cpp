//
// Created by olivier on 01/05/2022.
//

#include "Cache.h"

OneLife::server::bank::Cache::Cache() {}
OneLife::server::bank::Cache::~Cache() {}

/**
 * @note from server/map.cpp
 */
int computeXYCacheHash(int inKeyA, int inKeyB)
{

	int hashKey = (inKeyA * CACHE_PRIME_A + inKeyB * CACHE_PRIME_B) % BIOME_CACHE_SIZE;
	if (hashKey < 0) { hashKey += BIOME_CACHE_SIZE; }
	return hashKey;
}