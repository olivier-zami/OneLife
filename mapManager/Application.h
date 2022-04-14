//
// Created by olivier on 07/04/2022.
//

#ifndef ONELIFE_SERVEREDITOR_APPLICATION_H
#define ONELIFE_SERVEREDITOR_APPLICATION_H

#include <SDL.h>
#include "component/window/MapWindow.h"
#include "component/window/WorldMap.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_sdl.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer.h"

namespace OneLife::mapManager
{
	class Application
	{
		public:
			Application();
			~Application();

			void start();

		private:
			SDL_Window* window;
			SDL_Renderer* renderer;
			OneLife::mapManager::MapWindow* mapWindow;
			OneLife::mapManager::WorldMap* worldMap;
	};
}


#endif //ONELIFE_SERVEREDITOR_APPLICATION_H
