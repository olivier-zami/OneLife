//
// Created by olivier on 07/05/2022.
//

#ifndef INC_2HOL_MAPBUILDINGSCREEN_H
#define INC_2HOL_MAPBUILDINGSCREEN_H

#include <SDL.h>
#include <vector>

#include "../../../../third_party/openLife/src/extension/sdl2/handler/TextureHandler.h"
#include "../../../../third_party/openLife/src/procedure/diagram/voronoi/fortuneAlgorithm/Types/Point2D.h"

namespace OneLife::mapManager::window::widget::mapBuildingScreen
{
	typedef struct{
		SDL_Renderer* renderer;
		struct {
			struct{
				struct{
					size_t width;
					size_t height;
				}dimension;
			}map;
		}content;
		struct{
			struct{
				std::vector<Point2D>* zoneCenter;
			}map;
		}sharedData;
	}Settings;
}

typedef struct{
	int a;
	int b;
	int c;
	int d;
}TmpLine;

namespace OneLife::mapManager::window::widget
{
	class MapBuildingScreen
	{
		public:
			MapBuildingScreen(OneLife::mapManager::window::widget::mapBuildingScreen::Settings settings);
			~MapBuildingScreen();

			void drawSmallCircle(Point2D center);
			void drawSmallCircle1(Point2D center);
			void drawSingleEdgeEnd(Point2D center);
			void showDotField();
			void tmpDrawLine(int x1, int y1, int x2, int y2);
			void tmpDrawLine1(int x1, int y1, int x2, int y2);
			void render();

		private:
			SDL_Renderer* renderer;
			SDL_Texture* texture;
			openLife::extension::sdl2::handler::TextureHandler* textureHandler;
			int idScreen;
			struct{
				struct{
					struct {
						size_t width;
						size_t height;
					}dimension;
				}image;
			}content;
			struct{
				struct{
					std::vector<Point2D>* zoneCenter;
				}map;
			}sharedData;

			std::vector<TmpLine>* tmpLine;
			std::vector<openLife::dataType::geometric::Circle2D_32>* debugCircle;
			std::vector<openLife::dataType::geometric::Circle2D_32>* debugCircle1;
			std::vector<openLife::dataType::geometric::Circle2D_32>* debugSingleEdgeEnd;
	};
}


#endif //INC_2HOL_MAPBUILDINGSCREEN_H
