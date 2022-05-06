//
// Created by olivier on 05/05/2022.
//

#include "Preview.h"

#include <cstdio>
#include <SDL2/SDL.h>
#include <ctime>
#include <cstdlib>


OneLife::mapManager::window::Preview::Preview(SDL_Renderer* renderer)
{
	this->showWindow = true;
	if(renderer) this->renderer = renderer;
	this->previewTexture = nullptr;
	this->previewSurface = nullptr;
	this->content.image.dimension = {1000, 500};
	srand(time(NULL));
}

OneLife::mapManager::window::Preview::~Preview()
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

void OneLife::mapManager::window::Preview::animate()
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

void OneLife::mapManager::window::Preview::createSurface()
{
	this->previewSurface = SDL_CreateRGBSurface(
			0,
			this->content.image.dimension.width,
			this->content.image.dimension.height,
			32, 0, 0, 0, 0);
	if(!previewSurface){printf("\nPreview failed to create Surface (%lu, %lu) : %s", this->content.image.dimension.width, this->content.image.dimension.height, SDL_GetError());}
}

void OneLife::mapManager::window::Preview::createStreamingFrame()
{
	this->previewTexture = SDL_CreateTexture(
			this->renderer,
			SDL_PIXELFORMAT_RGB888,
			SDL_TEXTUREACCESS_STREAMING,
			this->content.image.dimension.width,
			this->content.image.dimension.height);
	if(!this->previewTexture){printf("\nfailed to create texture : %s", SDL_GetError());}
}

void OneLife::mapManager::window::Preview::createTexture()
{
	this->previewTexture = SDL_CreateTexture(
			this->renderer,
			SDL_PIXELFORMAT_RGBA8888,//SDL_PIXELFORMAT_RGB888,
			SDL_TEXTUREACCESS_TARGET,
			this->content.image.dimension.width,
			this->content.image.dimension.height);
	//this->previewTexture = SDL_CreateTextureFromSurface(this->renderer, this->previewSurface);
	if(!this->previewTexture){printf("\nfailed to create texture : %s", SDL_GetError());}

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

void OneLife::mapManager::window::Preview::draw()
{
	int error = SDL_SetRenderTarget(this->renderer, this->previewTexture);
	if(error)
	{
		printf("\nFail to use texture a target : %s", SDL_GetError());
	}

	SDL_PixelFormat* pixFormat;
	SDL_Color red = {255, 0, 0};
	SDL_Color blue = {0, 127, 214, 255};
	SDL_Color orange = {255, 127, 40, 255};
	SDL_Color white = {255, 255, 255, 255};
	SDL_Color light_grey = {235, 235, 235, 255};
	SDL_Rect bigPoint;
	SDL_Rect rect = {100, 100, 100, 100}, dst = {0, 0, 0, 0};

	error = SDL_SetRenderDrawColor(this->renderer, white.r, white.g, white.b, white.a);
	if(error) {printf("Erreur SDL_SetRenderDrawColor : %s", SDL_GetError());}
	SDL_RenderClear(this->renderer);

	error = SDL_SetRenderDrawColor(this->renderer, light_grey.r, light_grey.g, light_grey.b, light_grey.a);
	int step = 20;
	for(int i=step; i<this->content.image.dimension.width; i+=step)
	{
		SDL_RenderDrawLine(this->renderer, i, 0, i, this->content.image.dimension.height);
	}

	for(int i=step; i<this->content.image.dimension.height; i+=step)
	{
		SDL_RenderDrawLine(this->renderer, 0, i, this->content.image.dimension.width, i);
	}


	error = SDL_SetRenderDrawColor(this->renderer, red.r, red.g, red.b, red.a);
	for(int y=step; y<this->content.image.dimension.height; y+=step)
	{
		for(int x=step; x<this->content.image.dimension.width; x+=step)
		{
			bigPoint = {x-1+(rand()%((step/2)-1)) - (step/4), y-1+(rand()%((step/2)-1)) - (step/4), 3, 3};
			SDL_RenderFillRect(this->renderer, &bigPoint);
		}
	}

	error = SDL_SetRenderDrawColor(this->renderer, blue.r, blue.g, blue.b, blue.a);
	if(error) {printf("Erreur SDL_SetRenderDrawColor : %s", SDL_GetError());}
	SDL_RenderFillRect(this->renderer, &rect);

	error = SDL_SetRenderDrawColor(this->renderer, orange.r, orange.g, orange.b, orange.a);
	SDL_RenderDrawPoint(this->renderer, 500, 250);

	//error = SDL_SetRenderDrawColor(this->renderer, white.r, white.g, white.b, white.a);

	error = SDL_SetRenderTarget(this->renderer, NULL);
	if(error){printf("\nFail to use default a target : %s", SDL_GetError());}
}

void OneLife::mapManager::window::Preview::drawTest()
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

void OneLife::mapManager::window::Preview::render()
{
	if(this->showWindow)
	{
		if(!this->previewTexture) this->createTexture();
		this->draw();

		/*
		if(!this->previewSurface) this->createSurface();
		if(!this->previewTexture) this->createStreamingFrame();
		this->animate();
		*/
		//SDL_RenderPresent(this->renderer);//display

		/*
		SDL_SetRenderDrawColor(this->renderer, 255, 0, 0, 255);
		SDL_RenderDrawPoint(this->renderer, 500, 250);
		int errCode =  SDL_BlitSurface(this->mapTilesSurface, &SrcR, this->mapSurface, &DestR);
		if(errCode)
		{
			printf("failed to blitSurface : %s", SDL_GetError());
		}
		*/

		ImGui::Begin("Map Generation", &(this->showWindow));
		ImGui::Image((void*)this->previewTexture, ImVec2(1000, 500));
		ImGui::End();
	}
}