//
// Created by olivier on 29/04/2022.
//

#ifndef ONELIFE_SERVER_DATATYPE_MAP_H
#define ONELIFE_SERVER_DATATYPE_MAP_H

#include "../../third_party/minorGems/util/SimpleVector.h"
#include "record.h"

namespace OneLife::server::dataType::map
{
	typedef struct Region
	{
		int xMin;
		int yMin;
		int xMax;
		int yMax;
	}Region;

	typedef struct BiomeRegion
	{
		Region coord;
		SimpleVector<OneLife::server::dataType::record::Biome> tile;
	}BiomeRegion;
}

#endif //ONELIFE_SERVER_DATATYPE_MAP_H
