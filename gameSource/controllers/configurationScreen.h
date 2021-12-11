//
// Created by olivier on 09/12/2021.
//

#ifndef ONELIFE_GAME_CONFIGURATION_SCREEN_H
#define ONELIFE_GAME_CONFIGURATION_SCREEN_H

#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"

namespace OneLife::game
{
	class ConfigurationScreen
			: public GamePage
	{
		public:
			ConfigurationScreen();
			~ConfigurationScreen();

			void handle(OneLife::dataType::UiComponent* screen);
			void handleDemoMode(char* demoMode);
			void handleWriteFailed(char* writeFailed);
			void handleMeasureFrameRate(char* measureFrameRate);

			void setPlaybackMode(bool playbackMode);

		private:
			void initDemoMode();
			void checkEnvironment();
			void initUnknownFeature();
			void initUnknownFeature1();
			void initUnknownFeature2();

			struct{
				bool isInitDemoMode;
				bool isEnvironmentChecked;
				bool isPlaybackMode;
			}status;

			struct{
				char* demoMode;
				char* writeFailed;
				char* measureFrameRate;
			}global;

			struct{
			}data;

			OneLife::dataType::uiComponent::LoadingScreen screen;
	};
}

#endif //ONELIFE_GAME_CONFIGURATION_SCREEN_H
