//
// Created by olivier on 05/05/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_WINDOW_PREVIEW_H
#define ONELIFE_MAPMANAGER_COMPONENT_WINDOW_PREVIEW_H

#include "../Window.h"

#include <vector>

#include "../../../third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/Types/Point2D.h"
#include "widget/MapBuildingScreen.h"
#include "widget/MapBuildingStreamingScreen.h"

#include <SDL.h>

namespace OneLife::mapManager::window::preview
{
	typedef struct{
		SDL_Renderer* renderer;
		struct{
			size_t width;
			size_t height;
		}dimension;
		struct{
			struct{
				struct{
					size_t width;
					size_t height;
				}dimension;
			}map;
		}content;
	}Settings;
}

namespace OneLife::mapManager::window
{
	class Preview
	{
		public:
			Preview(OneLife::mapManager::window::preview::Settings settings);
			~Preview();

			void render();

		private:
			bool showWindow;
			SDL_Renderer* renderer;
			bool modified;

			struct{
				struct{
					struct{
						size_t width;
						size_t height;
					}dimension;
					std::vector<Point2D>* zoneCenter;
				}map;
			}content;

			OneLife::mapManager::window::widget::MapBuildingScreen* mapBuildingScreen;
	};
}


#endif //ONELIFE_MAPMANAGER_COMPONENT_WINDOW_PREVIEW_H
