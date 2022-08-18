//
// Created by olivier on 07/05/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_WINDOW_WIDGET_MAPBUILDINGSTREAMINGSCREEN_H
#define ONELIFE_MAPMANAGER_COMPONENT_WINDOW_WIDGET_MAPBUILDINGSTREAMINGSCREEN_H

#include <SDL.h>

namespace OneLife::mapManager::window::widget
{
	class MapBuildingStreamingScreen
	{
		public:
			MapBuildingStreamingScreen(SDL_Renderer* renderer);
			~MapBuildingStreamingScreen();

			void animate();
			void createStreamingFrame();
			void createSurface();
			void createTexture();
			void drawTest();
			void show();

		private:
			SDL_Renderer* renderer;
			SDL_Texture* previewTexture;
			SDL_Surface* previewSurface;
			struct{
				struct{
					struct {
						size_t width;
						size_t height;
					}dimension;
				}image;
			}content;
	};
}


#endif //ONELIFE_MAPMANAGER_COMPONENT_WINDOW_WIDGET_MAPBUILDINGSTREAMINGSCREEN_H
