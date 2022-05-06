//
// Created by olivier on 05/05/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_WINDOW_TOOLBOX_H
#define ONELIFE_MAPMANAGER_COMPONENT_WINDOW_TOOLBOX_H

#include "../Window.h"

namespace OneLife::mapManager::window
{
	class ToolBox
	{
		public:
			ToolBox(SDL_Renderer* renderer=nullptr);
			~ToolBox();

			bool* getStatus();
			void render();

		private:
			bool showWindow;
			SDL_Renderer* renderer;

	};
}

#endif //ONELIFE_MAPMANAGER_COMPONENT_WINDOW_TOOLBOX_H
