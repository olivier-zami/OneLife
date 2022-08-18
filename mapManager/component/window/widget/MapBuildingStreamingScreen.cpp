//
// Created by olivier on 07/05/2022.
//

#include "MapBuildingStreamingScreen.h"

#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_sdl.h"
#include "../../third_party/imgui/backends/imgui_impl_sdlrenderer.h"

#include <cstdio>
#include <SDL2/SDL.h>
#include <ctime>
#include <cstdlib>

OneLife::mapManager::window::widget::MapBuildingStreamingScreen::MapBuildingStreamingScreen(SDL_Renderer* renderer)
{
	this->renderer = renderer;
	this->previewTexture = nullptr;
	this->previewSurface = nullptr;
	this->content.image.dimension = {1000, 500};
	srand(time(NULL));
	this->createTexture();
}

OneLife::mapManager::window::widget::MapBuildingStreamingScreen::~MapBuildingStreamingScreen()
{
	if(this->previewTexture)
	{
		SDL_DestroyTexture(this->previewTexture);
		this->previewTexture = nullptr;
	}

	if(this->previewSurface)
	{
		SDL_FreeSurface(this->previewSurface);
		this->previewSurface = nullptr;
	}
}

void OneLife::mapManager::window::widget::MapBuildingStreamingScreen::animate()
{
	SDL_PixelFormat* pixFormat;
	pixFormat = this->previewSurface->format;
	struct {
		int x;
		int y;
	}pixel = {500, 250};

	Uint32 color = SDL_MapRGB(pixFormat, 255, 0, 0);
	Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) this->previewSurface->pixels
														+ pixel.y * this->previewSurface->pitch
														+ pixel.x * this->previewSurface->format->BytesPerPixel);
	*target_pixel = color;

	void* mPixels;
	int mPitch;
	SDL_LockTexture(this->previewTexture, NULL, &mPixels, &mPitch );
	memcpy(mPixels, this->previewSurface->pixels, this->previewSurface->pitch * this->previewSurface->h );
	SDL_UnlockTexture(this->previewTexture);
	mPixels = NULL;
}

void OneLife::mapManager::window::widget::MapBuildingStreamingScreen::createStreamingFrame()
{
	this->previewTexture = SDL_CreateTexture(
			this->renderer,
			SDL_PIXELFORMAT_RGB888,
			SDL_TEXTUREACCESS_STREAMING,
			this->content.image.dimension.width,
			this->content.image.dimension.height);
	if(!this->previewTexture){printf("\nfailed to create texture : %s", SDL_GetError());}
}

void OneLife::mapManager::window::widget::MapBuildingStreamingScreen::createSurface()
{
	this->previewSurface = SDL_CreateRGBSurface(
			0,
			this->content.image.dimension.width,
			this->content.image.dimension.height,
			32, 0, 0, 0, 0);
	if(!previewSurface){printf("\nPreview failed to create Surface (%lu, %lu) : %s", this->content.image.dimension.width, this->content.image.dimension.height, SDL_GetError());}
}

void OneLife::mapManager::window::widget::MapBuildingStreamingScreen::createTexture()
{
	/*
	//!now draw on texture
	int error = SDL_SetRenderTarget(this->renderer, this->previewTexture);
	if(error)
	{
		printf("\nfailed to select texture as a target : %s", SDL_GetError());
	}
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderDrawPoint(renderer, 500, 250);
	//SDL_RenderFillRect(this->renderer, &rect);
	SDL_SetRenderTarget(this->renderer, NULL);
	*/
}

void OneLife::mapManager::window::widget::MapBuildingStreamingScreen::drawTest()
{
	/*
	SDL_Point pix = {0, 0};
	SDL_Rect rect = {0, 0, 10, 10};
	SDL_Color color = {255, 0, 0, 255};
	//SDL_Delay(500);//wait for O.5 second
	SDL_Point points[12];
	int SDL_RenderDrawPoints(SDL_Renderer*    renderer,
                         const SDL_Point* points,
                         int              count)

	 h
	 SDL_bool SDL_IntersectRect(const SDL_Rect* A,
                           const SDL_Rect* B,
                           SDL_Rect*       result)

                           SDL_bool SDL_PointInRect(const SDL_Point* p,
                         const SDL_Rect*  r)

 	SDL_Texture* SDL_GetRenderTarget(SDL_Renderer* renderer)//get render target
	 int SDL_RenderCopy(SDL_Renderer*   renderer,
                   SDL_Texture*    texture,
                   const SDL_Rect* srcrect,
                   const SDL_Rect* dstrect)


                   SDL_Rect dst = {0, 0, 0, 0};
SDL_QueryTexture(texture, NULL, NULL, &dst.w, &dst.h);//get texture dimension after resize for rendering
SDL_RenderCopy(renderer, texture, NULL, &dst);

	 */
}

void OneLife::mapManager::window::widget::MapBuildingStreamingScreen::show()
{
	if(!this->previewTexture) this->createTexture();

}