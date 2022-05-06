//
// Created by olivier on 05/05/2022.
//

#ifndef ONELIFE_MAPMANAGER_COMPONENT_WINDOW_PREVIEW_H
#define ONELIFE_MAPMANAGER_COMPONENT_WINDOW_PREVIEW_H

#include "../Window.h"

#include <SDL.h>

namespace OneLife::mapManager::window
{
	class Preview
	{
		public:
			Preview(SDL_Renderer* renderer);
			~Preview();

			void animate();
			void createSurface();
			void createStreamingFrame();
			void createTexture();
			void draw();
			void drawTest();
			void render();

		private:
			bool showWindow;
			SDL_Renderer* renderer;
			SDL_Surface* previewSurface;
			SDL_Texture* previewTexture;
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


#endif //ONELIFE_MAPMANAGER_COMPONENT_WINDOW_PREVIEW_H
