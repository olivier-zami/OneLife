//
// Created by olivier on 23/11/2021.
//

#ifndef ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H
#define ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H

#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"
#include "OneLife/gameSource/dataTypes/game.h" //LiveObject
#include "OneLife/gameSource/scenes1/casting.h"

namespace OneLife::game
{
	class WaitingScreen//TODO rename LoadingLocalMapScreen
			: public GamePage
	{
		public:
			WaitingScreen();
			~WaitingScreen();

			void handle(OneLife::dataType::UiComponent* screen);
			void handle(LiveObject* player);
			void handle(OneLife::game::Casting* casting);

		private:
			void initScreen();
			void initPlayerAgent();
			void updateScreen();
			void update0();
			void update1();
			void update3();

			struct{
				bool isPlayerAgentSet;
			}status;

			static const char* screenName;
			bool isScreenInited;
			LiveObject* player;
			OneLife::game::Casting* casting;
			OneLife::dataType::uiComponent::WaitingScreen screen{};
	};
}

#endif //ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H
