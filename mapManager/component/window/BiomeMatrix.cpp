//
// Created by olivier on 09/04/2022.
//

#include "BiomeMatrix.h"

#include <SDL2/SDL_image.h>


OneLife::mapManager::window::BiomeMatrix::BiomeMatrix(SDL_Renderer* renderer)
{
	this->showWindow = true;
	if(renderer) this->renderer = renderer;

	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
	this->mapSurface = SDL_CreateRGBSurface(0, 800, 600, 32, rmask, gmask, bmask, amask);
	//this->mapSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);default map
	this->mapTexture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 800, 600);
	//this->mapTexture = SDL_CreateTextureFromSurface(this->renderer, this->mapSurface);

	this->mapTilesSurface = SDL_LoadBMP("../src/mapManager/data/mini_tiles.bmp");
	if(!this->mapTilesSurface)
	{
		printf("Failed to load \"../src/mapManager/data/mini_tiles.bmp\" : %s", SDL_GetError());
	}

	this->mapTiles.dimension = {this->mapTilesSurface->w, this->mapTilesSurface->h};

	/*
	this->mapTilesTexture = SDL_CreateTextureFromSurface(this->renderer, mapTilesSurface);
	if(!this->mapTilesTexture)
	{
		printf("Failed to read \"../src/mapManager/data/mini_tiles.bmp\" : %s", SDL_GetError());
	}

	/*
	this->mapTiles = SDL_CreateTextureFromSurface(this->renderer, mapTileSurface);
	if(!this->mapTiles)
	{
		printf("Failed to read \"../src/mapManager/data/mini_tiles.bmp\" : %s", SDL_GetError());;
	}
	this->mapTexture = nullptr;
 	*/

}

OneLife::mapManager::window::BiomeMatrix::~BiomeMatrix()
{
	if(this->mapSurface){SDL_FreeSurface(this->mapSurface); this->mapSurface=nullptr;}
	if(this->mapTilesSurface){SDL_FreeSurface(this->mapTilesSurface); this->mapTilesSurface=nullptr;}
	SDL_DestroyTexture(this->mapTexture);
	SDL_DestroyTexture(this->mapTilesTexture);
}

void OneLife::mapManager::window::BiomeMatrix::render()
{
	if(!this->showWindow) return;

	ImVec2 screen_pos = ImGui::GetCursorScreenPos();

	ImGui::Begin("Biome Matrix", &(this->showWindow), ImGuiWindowFlags_MenuBar);

	ImGuiIO& io = ImGui::GetIO();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	//ImVec2        GetWindowPos();

	ImGui::Text("%f, %f", io.MousePos.x, io.MousePos.y);
	ImGui::Text("%f, %f", pos.x, pos.y);
	ImGui::Text("%f, %f", io.MousePos.x - pos.x, io.MousePos.y - pos.y);

	int errCode;

	/*
	failed = SDL_SetRenderTarget(this->renderer, this->hiddenMapTexture);
	if(failed)
	{
		printf("failed to init double hidden buffer : %s", SDL_GetError());
	}
 	*/

	SDL_PixelFormat *fmt;
	//fmt = this->mapSurface->format;
	/*
	if(fmt->BitsPerPixel!=8){
	  fprintf(stderr, "Not an 8-bit surface.\n");
	  return(-1);
	}
	 */
	SDL_Rect biomeTile =  {12, 0, 11, 11};
	SDL_FillRect(this->mapSurface, &biomeTile, SDL_MapRGB(this->mapSurface->format, 0, 255, 0));

	SDL_Rect SrcR = {5, 0, 5, 5};
	SDL_Rect DestR= {0, 0, 5, 5};
	//SDL_RenderCopy(this->renderer, this->mapTilesTexture, &SrcR, &DestR);
	errCode =  SDL_BlitSurface(this->mapTilesSurface, &SrcR, this->mapSurface, &DestR);
	if(errCode)
	{
		printf("failed to blitSurface : %s", SDL_GetError());
	}

	//SDL_SetRenderTarget(this->renderer, nullptr);

	//int SDL_FillRect(SDL_Surface * dst, const SDL_Rect * rect, Uint32 color);

	//if(this->mapTilesTexture)
	{
		void* mPixels;
		int mPitch;
		SDL_LockTexture(this->mapTexture, NULL, &mPixels, &mPitch );
		memcpy(mPixels, this->mapSurface->pixels, this->mapSurface->pitch * this->mapSurface->h);
		SDL_UnlockTexture(this->mapTexture);
	}


	if(!this->mapTexture)
	{
		//void* mPixels;
		//int mPitch;
		//this->mapTexture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 800, 600);
		//this->mapTexture = SDL_CreateTextureFromSurface(this->renderer, image);
		//if(!this->mapTexture)
		//{
		//	printf("failed to create texture : %s", SDL_GetError());
		//}
		//SDL_LockTexture(this->mapTexture, NULL, &mPixels, &mPitch );

		//SDL_Rect SrcR = {0, 5, 5, 5};
		//SDL_Rect DestR= {0,0,0,0};
		//SDL_SetRenderTarget(this->renderer, this->mapTexture);
		//SDL_RenderCopy(this->renderer, this->mapTexture, this->mapTiles, &SrcR, &DestR);

		//memcpy( mPixels, this->mapTiles->pixels, this->mapTiles->pitch * this->mapTiles->h );
		//SDL_UnlockTexture(this->mapTexture);
		//mPixels = NULL;


	}

	ImGui::Image((void*)this->mapTexture, ImVec2(800, 600));
	ImGui::Text("map position : %f, %f", io.MousePos.x, io.MousePos.y);
	/*
	ImGui::Image((void*)this->mapTiles,
			  ImVec2(this->map.dimension.width, this->map.dimension.height),
			  ImVec2(0/this->map.dimension.width,0/this->map.dimension.height),
			  ImVec2(30/this->map.dimension.width,5/this->map.dimension.height));
	ImGui::Image((void*)this->mapTiles,
				 ImVec2(this->map.dimension.width*3, this->map.dimension.height*3),
				 ImVec2(0,0),
				 ImVec2(1,1/2));*/


	// Menu Bar
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Menu"))
		{
			/*
			IMGUI_DEMO_MARKER("Menu/File");
			ShowExampleMenuFile();*/
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Examples"))
		{
			/*IMGUI_DEMO_MARKER("Menu/Examples");
			ImGui::MenuItem("Main menu bar", NULL, &show_app_main_menu_bar);
			ImGui::MenuItem("Console", NULL, &show_app_console);
			ImGui::MenuItem("Log", NULL, &show_app_log);
			ImGui::MenuItem("Simple layout", NULL, &show_app_layout);
			ImGui::MenuItem("Property editor", NULL, &show_app_property_editor);
			ImGui::MenuItem("Long text display", NULL, &show_app_long_text);
			ImGui::MenuItem("Auto-resizing window", NULL, &show_app_auto_resize);
			ImGui::MenuItem("Constrained-resizing window", NULL, &show_app_constrained_resize);
			ImGui::MenuItem("Simple overlay", NULL, &show_app_simple_overlay);
			ImGui::MenuItem("Fullscreen window", NULL, &show_app_fullscreen);
			ImGui::MenuItem("Manipulating window titles", NULL, &show_app_window_titles);
			ImGui::MenuItem("Custom rendering", NULL, &show_app_custom_rendering);
			ImGui::MenuItem("Documents", NULL, &show_app_documents);*/
			ImGui::EndMenu();
		}
		//if (ImGui::MenuItem("MenuItem")) {} // You can also use MenuItem() inside a menu bar!
		if (ImGui::BeginMenu("Tools"))
		{
			/*IMGUI_DEMO_MARKER("Menu/Tools");
#ifndef IMGUI_DISABLE_METRICS_WINDOW
			ImGui::MenuItem("Metrics/Debugger", NULL, &show_app_metrics);
			ImGui::MenuItem("Stack Tool", NULL, &show_app_stack_tool);
#endif
			ImGui::MenuItem("Style Editor", NULL, &show_app_style_editor);
			ImGui::MenuItem("About Dear ImGui", NULL, &show_app_about);*/
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	ImGui::End();
}
