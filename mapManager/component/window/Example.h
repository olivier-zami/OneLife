//
// Created by olivier on 05/05/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_WINDOW_EXAMPLE_H
#define ONELIFE_MAPMANAGER_COMPONENT_WINDOW_EXAMPLE_H

#include "../Window.h"
#include "ToolBox.h"

namespace OneLife::mapManager::window
{
	class Example
	{
		public:
			Example(SDL_Renderer* renderer=nullptr);
			~Example();

			void control(OneLife::mapManager::window::ToolBox* toolBox);
			void handle(ImVec4* clear_color);
			void render();

		private:
			bool showWindow;
			bool* remoteShowToolBox;
			ImVec4* clear_color;
			SDL_Renderer* renderer;
	};
}


#endif //ONELIFE_MAPMANAGER_COMPONENT_WINDOW_EXAMPLE_H
