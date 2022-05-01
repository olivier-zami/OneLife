//
// Created by olivier on 01/05/2022.
//

#include "Biome.h"

#include "../../dataType/cacheRecord.h"

BiomeCacheRecord biomeCache[BIOME_CACHE_SIZE];

OneLife::server::cache::Biome::Biome() {}
OneLife::server::cache::Biome::~Biome() {}

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
 * @note from server/map.cpp
 */
void initBiomeCache()
{
	BiomeCacheRecord blankRecord = {0, 0, -2, 0, 0};
	for (int i = 0; i < BIOME_CACHE_SIZE; i++)
	{
		biomeCache[i] = blankRecord;
	}
}