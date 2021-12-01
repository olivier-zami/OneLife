//
// Created by olivier on 23/11/2021.
//

#ifndef ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H
#define ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H

#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"
#include "OneLife/gameSource/dataTypes/game.h" //LiveObject

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

		private:
			void initScreen();
			void updateScreen();
			void update0();
			void update1();
			void update2();
			void update3();

			static const char* screenName;
			bool isScreenInited;
			LiveObject* player;
			OneLife::dataType::uiComponent::WaitingScreen screen{};
	};
}

#endif //ONELIFE_COMPONENT_SCREEN_WAITINGBIRTHSCREEN_H
