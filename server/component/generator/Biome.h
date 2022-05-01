//
// Created by olivier on 01/05/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_GENERATOR_BIOME_H
#define ONELIFE_SERVER_COMPONENT_GENERATOR_BIOME_H

#include <cstddef>

namespace OneLife::server::generator
{
	class Biome
	{
		public:
			Biome();
			~Biome();
	};
}

int computeMapBiomeIndex(int inX, int inY, int *outSecondPlaceIndex = NULL, double *outSecondPlaceGap = NULL);

#endif //ONELIFE_SERVER_COMPONENT_GENERATOR_BIOME_H
