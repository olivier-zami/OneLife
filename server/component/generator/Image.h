//
// Created by olivier on 03/05/2022.
//

#ifndef ONELIFE_SERVER_COMPONENT_GENERATOR_WORLDMAP_H
#define ONELIFE_SERVER_COMPONENT_GENERATOR_WORLDMAP_H

namespace OneLife::server::generator
{
	class Image
	{
		public:
			Image();
			~Image();
	};
}

void outputMapImage();
void printBiomeSamples();
void outputBiomeFractals();

#endif //ONELIFE_SERVER_COMPONENT_GENERATOR_WORLDMAP_H
