//
// Created by olivier on 07/04/2022.
//

#include "MapWindow.h"

//#include "OneLife/third_party/imgui/imgui.h"
//#include "OneLife/third_party/imgui/backends/imgui_impl_sdl.h"
//#include "OneLife/third_party/imgui/backends/imgui_impl_sdlrenderer.h"
//#include "OneLife/third_party/imgui/backends/imgui_impl_opengl3.h"

#include <stdio.h>

OneLife::mapManager::MapWindow::MapWindow(SDL_Renderer* renderer)
{
	this->showWindow = true;
	if(renderer) this->renderer = renderer;
	this->mapTexture = nullptr;
}

OneLife::mapManager::MapWindow::~MapWindow() {}

void OneLife::mapManager::MapWindow::render()
{
	if(!this->showWindow) return;
	ImGui::Begin("Map Picture", &(this->showWindow));   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
	ImGui::Text("Hello from world map window!");

	//if (ImGui::Button("Close Me")) this->showWindow = false;
	void *tmp;
	Uint32 *pixels;
	SDL_PixelFormat *format;
	int pitch;
	size_t i, j;
	size_t width = 255;
	size_t height = 255;

	if (ImGui::Button("Close Me"))
	{
		int w, h, access;
		Uint32 pixFormat=0;
		SDL_QueryTexture(this->mapTexture, &pixFormat, &access, &w, &h);
		format = SDL_AllocFormat(pixFormat);
		int error = SDL_LockTexture(this->mapTexture, NULL, &tmp, &pitch);
		if(error)
		{
			printf("\nSDL_LockTexture failed : %s", SDL_GetError());
		}
		printf("\ntexture surface (%i, %i) addr: %p", w, h, tmp);

		pixels = (Uint32*)tmp;

		for(i = 0; i < height; i++)
		{
			for(j = 0; j < width; j++) pixels[i * width + j] = SDL_MapRGBA(format, (Uint8)i, 0, 0, 255);
		}


		SDL_UnlockTexture(this->mapTexture);
		SDL_FreeFormat(format);
	}

	if(!this->mapTexture)
	{
		void* mPixels;
		int mPitch;
		SDL_Surface* image = SDL_LoadBMP("../src/mapManager/data/mini_map.bmp");
		if(image)
		{
			this->mapTexture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, image->w, image->h);
			//this->mapTexture = SDL_CreateTextureFromSurface(this->renderer, image);
			if(!this->mapTexture)
			{
				printf("failed to create texture : %s", SDL_GetError());
			}
			SDL_LockTexture(this->mapTexture, NULL, &mPixels, &mPitch );
			memcpy( mPixels, image->pixels, image->pitch * image->h );
			SDL_UnlockTexture(this->mapTexture);
			mPixels = NULL;

			this->map.dimension = {image->w, image->h};
			SDL_FreeSurface(image);
			image = NULL;
		}
		else printf("\nFail to read \"../data/images/mini_map.bmp\"");
	}

	ImGui::Image((void*)this->mapTexture, ImVec2(this->map.dimension.width, this->map.dimension.height));

	/*
	SDL_Rect rct2;
	rct2.x = rct2.y = 200;
	rct2.w = rct2.h = 400;
	SDL_RenderCopy(this->renderer, this->mapTexture, 0, &rct2);
	*/
	ImGui::End();
}
