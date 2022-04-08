//
// Created by olivier on 07/04/2022.
//

#ifndef ONELIFE_SERVEREDITOR_APPLICATION_H
#define ONELIFE_SERVEREDITOR_APPLICATION_H

#include "component/window/MapWindow.h"

namespace OneLife::mapManager
{
	class Application
	{
		public:
			Application();
			~Application();

			void start();

		private:
			OneLife::mapManager::MapWindow* mapWindow;
	};
}


#endif //ONELIFE_SERVEREDITOR_APPLICATION_H
