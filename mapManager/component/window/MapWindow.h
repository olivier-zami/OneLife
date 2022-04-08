//
// Created by olivier on 07/04/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_MAPWINDOW_H
#define ONELIFE_MAPMANAGER_COMPONENT_MAPWINDOW_H

#include "../Window.h"

namespace OneLife::mapManager
{
	class MapWindow
	{
		public:
			MapWindow();
			~MapWindow();

			void render();

		private:
			bool showWindow;
	};
};

#endif //ONELIFE_MAPMANAGER_COMPONENT_MAPWINDOW_H
