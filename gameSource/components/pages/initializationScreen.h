//
// Created by olivier on 19/11/2021.
//

#ifndef ONELIFE_GAME_INITIALIZATIONSCREEN_H
#define ONELIFE_GAME_INITIALIZATIONSCREEN_H

#include "minorGems/system/Time.h"
#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/components/pages/initOutdoorSceneScreen.h"
#include "OneLife/gameSource/components/pages/LivingLifePage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"
#include "OneLife/gameSource/dataTypes/socket.h"

namespace OneLife::game
{
	class InitializationScreen :
			public GamePage
	{
		public:
			InitializationScreen();
			~InitializationScreen();

			void handle(OneLife::dataType::UiComponent* screen);
			void handle(OneLife::game::component::Socket* socket);
			void handle(OneLife::game::WaitingScreen* controller);
			void handle(LivingLifePage* controller);

			void setServerSocketAddress(OneLife::dataType::socket::Address socket);

			void initSocket();
			void initSceneGeneratorController();
			void initGameSceneController();
			bool isTaskComplete();

		private:
			struct{
				bool isConfigurationLoaded;
				bool isSocketSet;
				bool isInitSceneGeneratorController;
				bool isInitGameSceneController;
			}status;

			struct{
				OneLife::dataType::socket::Address socket;
			}data;

			struct{
				OneLife::game::WaitingScreen* sceneGeneratorController;
				LivingLifePage* gameSceneController;
			}controller;

			OneLife::game::component::Socket* socket;

			bool taskComplete;
			bool minDuration;
			timeSec_t frameStartSec;
			unsigned long frameStartMSec;
			OneLife::dataType::uiComponent::LoadingScreen screen;
	};
}


#endif //ONELIFE_GAME_INITIALIZATIONSCREEN_H
