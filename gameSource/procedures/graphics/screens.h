//
// Created by olivier on 01/11/2021.
//

#ifndef ONELIFE_GRAPHICS_SCREENS_H
#define ONELIFE_GRAPHICS_SCREENS_H

#include "minorGems/game/doublePair.h"
#include "OneLife/gameSource/dataTypes/ui.h"
#include "OneLife/gameSource/components/socket.h"
#include "OneLife/gameSource/components/pages/LivingLifePage.h"

namespace OneLife::game::graphic
{
	void drawInitializingScreen(void* screen = nullptr);
	void drawLoadingScreen(void* screen = nullptr);
	void drawUnimplementedScreen(void* screen = nullptr);
	void drawOutdoorSceneScreen(void* screen = nullptr);

	void drawGameScreen(
			char mStartedLoadingFirstObjectSet,
			float mFirstObjectSetLoadingProgress,
			char mDoneLoadingFirstObjectSet);

	void drawWaitingBirthScreen(void* screen = nullptr);
	void drawWaitingBirthScreen(
			void* screen = nullptr,
			OneLife::game::component::Socket* socket = nullptr,
			doublePair lastScreenViewCenter = {0,0},
			float connectionMessageFade = 0,
			double frameRateFactor = 0,
			float mFirstObjectSetLoadingProgress = 0,
			char usingCustomServer = 0,
			char* serverIP = nullptr,
			int serverPort = 0,
			char userReconnect = false,
			char mPlayerInFlight = false,
			char *userTwinCode = nullptr,
			int userTwinCount = 0,
			char mStartedLoadingFirstObjectSet = false);
}



#endif //ONELIFE_GRAPHICS_SCREENS_H
