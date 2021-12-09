//
// Created by olivier on 19/11/2021.
//

#ifndef ONELIFE_GAME_INITIALIZATIONSCREEN_H
#define ONELIFE_GAME_INITIALIZATIONSCREEN_H

#include "minorGems/system/Time.h"
#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/controllers/sceneBuilder.h"
#include "OneLife/gameSource/controllers/LivingLifePage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"
#include "OneLife/gameSource/dataTypes/socket.h"

namespace OneLife::game
{
	class initScreen :
			public GamePage
	{
		public:
			initScreen();
			~initScreen();

			void handle(OneLife::dataType::UiComponent* screen);
			void handle(OneLife::game::component::Socket** socket);
			void handle(OneLife::game::SceneBuilder** controller);

			void setServerSocketAddress(OneLife::dataType::socket::Address socket);

			void initSocket();
			void initSceneBuilder();
			bool isTaskComplete();

		private:
			struct{
				bool isConfigurationLoaded;
				bool isInitSocket;
				bool isInitSceneBuilder;
			}status;

			struct{
				OneLife::dataType::socket::Address socket;
			}data;

			struct{
				OneLife::game::SceneBuilder** sceneBuilder;
				LivingLifePage* gameScene;
			}controller;

			OneLife::game::component::Socket** socket;

			bool taskComplete;
			bool minDuration;
			timeSec_t frameStartSec;
			unsigned long frameStartMSec;
			OneLife::dataType::uiComponent::LoadingScreen screen;
	};
}


#endif //ONELIFE_GAME_INITIALIZATIONSCREEN_H
