//
// Created by olivier on 23/11/2021.
//

#ifndef ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H
#define ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H

#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/components/socket.h"
#include "OneLife/gameSource/controllers/LivingLifePage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"
#include "OneLife/gameSource/dataTypes/game.h" //LiveObject

namespace OneLife::game
{
	class SceneBuilder//TODO rename LoadingLocalMapScreen
			: public GamePage
	{
		public:
			SceneBuilder();
			~SceneBuilder();

			void handle(OneLife::dataType::UiComponent* screen);
			void handle(LiveObject** player);
			void handle(LivingLifePage** gameScene);
			void handle(OneLife::game::component::Socket* socket);

		private:
			void connect();
			void initScreen();
			void downloadObjects();
			void initPlayerAgent();
			void updateScreen();
			void update0();
			void update1();
			void update3();

			struct{
				bool isConnected;
				bool isPlayerAgentSet;
				bool isObjectsDownloaded;
			}status;

			static const char* screenName;
			bool isScreenInited;
			LiveObject** player;
			LivingLifePage** gameScene;
			OneLife::dataType::uiComponent::SceneBuilder screen{};
			OneLife::game::component::Socket* socket;
	};
}

#endif //ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H
