//
// Created by olivier on 07/05/2022.
//

#include "MapBuildingScreen.h"

#include "../../../../third_party/openLife/src/dataType/geometric.h"
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_sdl.h"
#include "../../third_party/imgui/backends/imgui_impl_sdlrenderer.h"

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <SDL2/SDL.h>

OneLife::mapManager::window::widget::MapBuildingScreen::MapBuildingScreen(OneLife::mapManager::window::widget::mapBuildingScreen::Settings settings)
{
	this->renderer = settings.renderer;
	this->textureHandler = new openLife::extension::sdl2::handler::TextureHandler();
	this->content.image.dimension.width = settings.content.map.dimension.width;
	this->content.image.dimension.height = settings.content.map.dimension.height;
	this->sharedData.map.zoneCenter = settings.sharedData.map.zoneCenter;
	this->idScreen = 0;

	this->texture = SDL_CreateTexture(
			this->renderer,
			SDL_PIXELFORMAT_RGBA8888,//SDL_PIXELFORMAT_RGB888,
			SDL_TEXTUREACCESS_TARGET,
			this->content.image.dimension.width,
			this->content.image.dimension.height);
	//this->previewTexture = SDL_CreateTextureFromSurface(this->renderer, this->previewSurface);
	if(!this->texture){printf("\nfailed to create texture : %s", SDL_GetError());}
	printf("\n\n\tsetTexture: %p", this->texture);
	this->textureHandler->handle(this->texture);

	this->tmpLine = new std::vector<TmpLine>();
	this->debugCircle = new std::vector<openLife::dataType::geometric::Circle2D_32>();
	this->debugCircle1 = new std::vector<openLife::dataType::geometric::Circle2D_32>();
	this->debugSingleEdgeEnd = new std::vector<openLife::dataType::geometric::Circle2D_32>();
}

OneLife::mapManager::window::widget::MapBuildingScreen::~MapBuildingScreen()
{
	if(this->texture)
	{
		SDL_DestroyTexture(this->texture);
		this->texture = nullptr;
	}
}

void OneLife::mapManager::window::widget::MapBuildingScreen::drawSmallCircle(Point2D center)
{
	openLife::dataType::geometric::Circle2D_32 circle;
	circle.center.x = center.x;
	circle.center.y = center.y;
	circle.radius = 5;
	this->debugCircle->push_back(circle);
}

void OneLife::mapManager::window::widget::MapBuildingScreen::drawSmallCircle1(Point2D center)
{
	openLife::dataType::geometric::Circle2D_32 circle;
	circle.center.x = center.x;
	circle.center.y = center.y;
	circle.radius = 5;
	this->debugCircle1->push_back(circle);
}

void OneLife::mapManager::window::widget::MapBuildingScreen::drawSingleEdgeEnd(Point2D center)
{
	openLife::dataType::geometric::Circle2D_32 circle;
	circle.center.x = center.x;
	circle.center.y = center.y;
	circle.radius = 5;
	this->debugSingleEdgeEnd->push_back(circle);
}


void OneLife::mapManager::window::widget::MapBuildingScreen::showDotField()
{
	int error = SDL_SetRenderTarget(this->renderer, this->texture);
	if(error)
	{
		printf("\nFail to use texture a target : %s", SDL_GetError());
	}

	SDL_PixelFormat* pixFormat;
	SDL_Color red = {255, 0, 0};
	//SDL_Color blue = {0, 127, 214, 255};//!unused
	SDL_Color orange = {255, 127, 40, 255};
	SDL_Color white = {255, 255, 255, 255};
	SDL_Color light_grey = {245, 245, 245, 255};
	//SDL_Rect dst = {0, 0, 0, 0};//!unused

	error = SDL_SetRenderDrawColor(this->renderer, white.r, white.g, white.b, white.a);
	if(error) {printf("Erreur SDL_SetRenderDrawColor : %s", SDL_GetError());}
	SDL_RenderClear(this->renderer);

	error = SDL_SetRenderDrawColor(this->renderer, light_grey.r, light_grey.g, light_grey.b, light_grey.a);
	int step = 20;
	for(size_t i=step; i<this->content.image.dimension.width; i+=step)
	{
		SDL_RenderDrawLine(this->renderer, i, 0, i, this->content.image.dimension.height);
	}

	for(size_t i=step; i<this->content.image.dimension.height; i+=step)
	{
		SDL_RenderDrawLine(this->renderer, 0, i, this->content.image.dimension.width, i);
	}

	SDL_Rect bigPoint;
	error = SDL_SetRenderDrawColor(this->renderer, red.r, red.g, red.b, red.a);
	for(size_t i=0; i<(this->sharedData.map.zoneCenter)->size(); i++)
	{
		Point2D zoneCenter = this->sharedData.map.zoneCenter->at(i);
		bigPoint = {(int)(zoneCenter.x)-1, (int)(zoneCenter.y)-1, 3, 3};
		SDL_RenderFillRect(this->renderer, &bigPoint);
	}

	/*
	SDL_Rect rect = {100, 100, 100, 100};
	error = SDL_SetRenderDrawColor(this->renderer, blue.r, blue.g, blue.b, blue.a);
	if(error) {printf("Erreur SDL_SetRenderDrawColor : %s", SDL_GetError());}
	SDL_RenderFillRect(this->renderer, &rect);
	*/

	SDL_SetRenderDrawColor(this->renderer, 127, 127, 127, 255);
	for(size_t i=0; i<this->tmpLine->size(); i++)
	{
		SDL_RenderDrawLine(this->renderer, this->tmpLine->at(i).a, this->tmpLine->at(i).b, this->tmpLine->at(i).c, this->tmpLine->at(i).d);
	}

	printf("\n\tsingleEdgeEnd: %i", this->debugSingleEdgeEnd->size());
	SDL_SetRenderDrawColor(this->renderer, 255, 255, 127, 255);
	for(size_t i=0; i<this->debugSingleEdgeEnd->size(); i++)
	{
		this->textureHandler->drawFillCircle(this->debugSingleEdgeEnd->at(i));
	}

	SDL_SetRenderDrawColor(this->renderer, 255, 127, 127, 255);
	for(size_t i=0; i<this->debugCircle->size(); i++)
	{
		this->textureHandler->drawFillCircle(this->debugCircle->at(i));
	}

	SDL_SetRenderDrawColor(this->renderer, 127, 255, 127, 255);
	for(size_t i=0; i<this->debugCircle1->size(); i++)
	{
		this->textureHandler->drawFillCircle(this->debugCircle1->at(i));
	}



	error = SDL_SetRenderDrawColor(this->renderer, orange.r, orange.g, orange.b, orange.a);
	SDL_RenderDrawPoint(this->renderer, 500, 250);

	//error = SDL_SetRenderDrawColor(this->renderer, white.r, white.g, white.b, white.a);

	error = SDL_SetRenderTarget(this->renderer, NULL);
	if(error){printf("\nFail to use default a target : %s", SDL_GetError());}
}

void OneLife::mapManager::window::widget::MapBuildingScreen::tmpDrawLine(int x1, int y1, int x2, int y2)
{
	TmpLine tmpLine1 = {x1, y1, x2, y2};
	this->tmpLine->push_back(tmpLine1);
}

void OneLife::mapManager::window::widget::MapBuildingScreen::render()
{
	ImGui::Image((void*)this->texture, ImVec2(1000, 500));
}

/**********************************************************************************************************************/