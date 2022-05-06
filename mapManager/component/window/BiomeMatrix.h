//
// Created by olivier on 09/04/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_WORLDMAP_H
#define ONELIFE_MAPMANAGER_COMPONENT_WORLDMAP_H

#include "../Window.h"

#include <SDL.h>

namespace OneLife::mapManager::window
{
	class BiomeMatrix
	{
		public:
			BiomeMatrix(SDL_Renderer* renderer=nullptr);
			~BiomeMatrix();

			void render();

		private:
			bool showWindow;
			SDL_Renderer* renderer;
			SDL_Surface* mapSurface;
			SDL_Texture* mapTexture;
			SDL_Surface* mapTilesSurface;
			SDL_Texture* mapTilesTexture;
			struct{
				struct{int width; int height;}dimension;
			}mapTiles;

	};
}



#endif //ONELIFE_MAPMANAGER_COMPONENT_WORLDMAP_H
