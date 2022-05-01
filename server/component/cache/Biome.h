//
// Created by olivier on 01/05/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_CACHE_BIOME_H
#define ONELIFE_SERVER_COMPONENT_CACHE_BIOME_H

#include "../../prototype/bank/Cache.h"

#define BIOME_CACHE_SIZE 131072

namespace OneLife::server::cache
{
	class Biome:
		public OneLife::server::bank::Cache
	{
		public:
			Biome();
			~Biome();
	};
}

int biomeGetCached(int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap);
void biomePutCached(int inX, int inY, int inBiome, int inSecondPlace, double inSecondPlaceGap);

#endif //ONELIFE_SERVER_COMPONENT_CACHE_BIOME_H
