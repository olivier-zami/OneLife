//
// Created by olivier on 19/11/2021.
//

#ifndef ONELIFE_GAME_INITIALIZATIONSCREEN_H
#define ONELIFE_GAME_INITIALIZATIONSCREEN_H

#include "minorGems/system/Time.h"
#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"

namespace OneLife::game
{
	class InitializationScreen :
			public GamePage
	{
		public:
			InitializationScreen();
			~InitializationScreen();

			void handle(OneLife::dataType::UiComponent* screen);
			bool isTaskComplete();

		private:
			bool taskComplete;
			bool minDuration;
			timeSec_t frameStartSec;
			unsigned long frameStartMSec;
			OneLife::dataType::uiComponent::LoadingScreen screen;
	};
}


#endif //ONELIFE_GAME_INITIALIZATIONSCREEN_H
