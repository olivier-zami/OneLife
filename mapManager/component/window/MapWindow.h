//
// Created by olivier on 07/04/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_MAPWINDOW_H
#define ONELIFE_MAPMANAGER_COMPONENT_MAPWINDOW_H

#include "../Window.h"

#include <SDL.h>

namespace OneLife::mapManager
{
	class MapWindow
	{
		public:
			MapWindow(SDL_Renderer* renderer=nullptr);
			~MapWindow();

			void render();

		private:
			bool showWindow;
			SDL_Renderer* renderer;
			SDL_Texture* mapTexture;
			struct{
				struct{int width; int height;}dimension;
			}map;
	};
};

#endif //ONELIFE_MAPMANAGER_COMPONENT_MAPWINDOW_H
