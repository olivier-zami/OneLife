//
// Created by olivier on 01/11/2021.
//

#ifndef ONELIFE_GRAPHICS_SCREENS_H
#define ONELIFE_GRAPHICS_SCREENS_H

#include "OneLife/gameSource/dataTypes/ui.h"

namespace OneLife::game::graphic
{
	void drawInitializingScreen(void* screen = nullptr);
	void drawLoadingScreen(void* screen = nullptr);
	void drawUnimplementedScreen(void* screen = nullptr);
	void drawGameScreen(
			int mFirstServerMessagesReceived,
			char mStartedLoadingFirstObjectSet,
			float mFirstObjectSetLoadingProgress,
			char mDoneLoadingFirstObjectSet);
}



#endif //ONELIFE_GRAPHICS_SCREENS_H
