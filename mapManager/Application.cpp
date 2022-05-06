//
// Created by olivier on 07/04/2022.
//

// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important to understand: SDL_Renderer is an _optional_ component of SDL. We do not recommend you use SDL_Renderer
// because it provide a rather limited API to the end-user. We provide this backend for the sake of completeness.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "Application.h"

#include <stdio.h>

#include "../gameSource/GridPos.h"
#include "../server/component/cache/Biome.h"
#include "../server/component/Log.h"
#include "../server/component/Map.h"
#include "../server/dataType/LiveObject.h"
#include "../server/HashTable.h"
#include "../third_party/minorGems/system/Time.h"
#include "../third_party/minorGems/util/log/AppLog.h"
#include "../third_party/minorGems/util/MinPriorityQueue.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

extern int numBiomes;
extern int* biomes;
extern float* biomeWeights;
extern int numSpecialBiomes;
extern int* specialBiomes;
extern float* biomeCumuWeights;
extern float biomeTotalWeight;
extern int regularBiomeLimit;
extern float* specialBiomeCumuWeights;
extern float specialBiomeTotalWeight;

//!Should not user Server class because it may share data with a running one so declare same shared var here
int apocalypsePossible = 0;
GridPos apocalypseLocation = { 0, 0 };
char apocalypseTriggered = false;
int evePrimaryLocSpacing  = 0;
int evePrimaryLocObjectID = -1;
SimpleVector<GridPos> *speechPipesIn;
SimpleVector<GridPos> *speechPipesOut;
double mapChangeLogTimeStart = -1;
FILE *mapChangeLogFile = NULL;
HashTable<timeSec_t> liveDecayRecordLastLookTimeHashTable(1024);// times in seconds that a tracked live decay map cell or slot// was last looked at
HashTable<double> liveMovementEtaTimes(1024, 0);// clock time in fractional seconds of destination ETA indexed as x, y, 0
MinPriorityQueue<MovementRecord> liveMovements;
SimpleVector<LiveObject> players;

extern OneLife::server::cache::Biome* cachedBiome;

OneLife::mapManager::Application::Application()
{
	//!
	this->worldMap = new OneLife::server::Map();

	OneLife::server::settings::WorldMap settings = {
			0,
			0,
			0.80,
			0,
			nullptr,
			nullptr
	};
	settings.database.lookTime.url = (char*)malloc(64);
	memset(settings.database.lookTime.url, 0, 64);
	strcpy(settings.database.lookTime.url, "../src/server/lookTime.db");

	settings.database.biome.url = (char*)malloc(64);
	memset(settings.database.biome.url, 0, 64);
	strcpy(settings.database.biome.url, "../src/server/biome.db");

	cachedBiome = new OneLife::server::cache::Biome();

	//!---------------------------------------------
	numBiomes = 9;
	biomes = (int*)malloc(numBiomes*sizeof(int));
	biomeWeights = (float*)malloc(numBiomes*sizeof(float));
	numSpecialBiomes = 3;
	specialBiomes = (int*)malloc(numSpecialBiomes*sizeof(int));
	//printf("\n=====>numBiomes:%i", numBiomes);
	//printf("\n=====>numSpecialBiomes:%i", numSpecialBiomes);

	biomes[0] = 7;biomeWeights[0] = 0.18;specialBiomes[0] = 6;
	biomes[1] = 8;biomeWeights[1] = 0.04;specialBiomes[1] = 5;
	biomes[2] = 0;biomeWeights[2] = 0.18;specialBiomes[2] = 4;
	biomes[3] = 1;biomeWeights[3] = 0.13;
	biomes[4] = 2;biomeWeights[4] = 0.08;
	biomes[5] = 3;biomeWeights[5] = 0.09;
	biomes[6] = 6;biomeWeights[6] = 0.09;
	biomes[7] = 5;biomeWeights[7] = 0.11;
	biomes[8] = 4;biomeWeights[8] = 0.12;

	//printf("\n=====>biomes list:");for(int i=0; i<numBiomes; i++)printf("%i ", biomes[i]);
	//printf("\n=====>biomeWeights list:");for(int i=0; i<numBiomes; i++)printf("%f ", biomeWeights[i]);

	biomeCumuWeights = new float[numBiomes];
	biomeTotalWeight = 0;
	for (int i = 0; i < numBiomes; i++)
	{
		biomeTotalWeight += biomeWeights[i];
		biomeCumuWeights[i] = biomeTotalWeight;
	}
	//printf("\n=====>biomeCumuWeights list:");for(int i=0; i<numBiomes; i++)printf("%f ", biomeCumuWeights[i]);
	//printf("\n=====>biomeTotalWeight:%f", biomeTotalWeight);

	regularBiomeLimit = numBiomes - numSpecialBiomes;
	specialBiomeCumuWeights = new float[numSpecialBiomes];
	specialBiomeTotalWeight = 0;
	for (int i = regularBiomeLimit; i < numBiomes; i++)
	{
		specialBiomeTotalWeight += biomeWeights[i];
		specialBiomeCumuWeights[i - regularBiomeLimit] = specialBiomeTotalWeight;
	}
	//printf("\n=====>regularBiomeLimit:%i", regularBiomeLimit);
	//printf("\n=====>specialBiomeTotalWeight:%f", specialBiomeTotalWeight);
	//printf("\n=====>specialBiomeCumuWeights list:");for(int i=0; i<numSpecialBiomes; i++)printf("%f ", specialBiomeCumuWeights[i]);

	//!---------------------------------------------


	this->worldMap->init(settings);//TODO: use flag to disable unused feature

	free(settings.database.lookTime.url);


	OneLife::server::dataType::map::BiomeRegion biomeRegion;
	biomeRegion.coord = {-10, -10, 10, 10};

	this->worldMap->updateBiomeRegion(&biomeRegion);

	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return;// -1;
	}

	// Setup window
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	this->window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

	// Setup SDL_Renderer instance
	this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if (this->renderer == NULL)
	{
		SDL_Log("Error creating SDL_Renderer!");
		return ;//false;
	}

	//!
	this->mapWindow = new OneLife::mapManager::MapWindow(this->renderer);
	this->winBiomeMatrix = new OneLife::mapManager::window::BiomeMatrix(this->renderer);
	this->winExample = new OneLife::mapManager::window::Example(this->renderer);
	this->winToolBox = new OneLife::mapManager::window::ToolBox(this->renderer);
	this->winPreview = new OneLife::mapManager::window::Preview(this->renderer);

	//!
	this->winToolBox;
}

OneLife::mapManager::Application::~Application()
{
	free(biomes);
	free(biomeWeights);
	free(specialBiomes);
	delete[] biomeCumuWeights;
	delete[] specialBiomeCumuWeights;

	SDL_DestroyRenderer(this->renderer);
	SDL_DestroyWindow(this->window);
	SDL_Quit();
}

/**********************************************************************************************************************/

void OneLife::mapManager::Application::start()
{
	//SDL_RendererInfo info;
	//SDL_GetRendererInfo(this->renderer, &info);
	//SDL_Log("Current SDL_Renderer: %s", info.name);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(this->window, this->renderer);
	ImGui_ImplSDLRenderer_Init(this->renderer);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	this->winExample->handle(&clear_color);
	this->winExample->control(this->winToolBox);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle events (inputs, this->window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(this->window))
				done = true;
			if (event.type == SDL_KEYDOWN)
				printf("\nkey pressed ...");
		}

		// Start the Dear ImGui frame
		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame(this->window);
		ImGui::NewFrame();

		//!
		this->mapWindow->render();
		this->winBiomeMatrix->render();
		this->winExample->render();
		this->winToolBox->render();
		this->winPreview->render();

		// Rendering
		ImGui::Render();
		SDL_SetRenderDrawColor(this->renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
		SDL_RenderClear(this->renderer);
		ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(this->renderer);
	}

	// Cleanup
	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}
