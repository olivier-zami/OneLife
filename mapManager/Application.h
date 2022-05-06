//
// Created by olivier on 07/04/2022.
//

#ifndef ONELIFE_SERVEREDITOR_APPLICATION_H
#define ONELIFE_SERVEREDITOR_APPLICATION_H

#include <SDL.h>
#include "../server/component/Map.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_sdl.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer.h"
#include "component/window/BiomeMatrix.h"
#include "component/window/Example.h"
#include "component/window/MapWindow.h"
#include "component/window/ToolBox.h"
#include "component/window/Preview.h"

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
			OneLife::server::Map* worldMap;
			OneLife::mapManager::MapWindow* mapWindow;
			OneLife::mapManager::window::BiomeMatrix* winBiomeMatrix;
			OneLife::mapManager::window::Example* winExample;
			OneLife::mapManager::window::ToolBox* winToolBox;
			OneLife::mapManager::window::Preview* winPreview;
	};
}


#endif //ONELIFE_SERVEREDITOR_APPLICATION_H
