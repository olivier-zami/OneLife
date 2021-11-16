//
// Created by olivier on 16/11/2021.
//

#ifndef ONELIFE_GAME_GRAPHIC_STRING_H
#define ONELIFE_GAME_GRAPHIC_STRING_H

#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"
#include "OneLife/gameSource/dataTypes/game.h"

namespace OneLife::graphic
{
	void drawChalkBackgroundString(
			doublePair inPos,
			const char *inString,
			double inFade,
			double inMaxWidth,
			LiveObject *inSpeaker,
			int inForceMinChalkBlots,
			FloatColor *inForceBlotColor,
			FloatColor *inForceTextColor,
			SpriteHandle mChalkBlotSprite);

	void drawOffScreenSounds(SpriteHandle mChalkBlotSprite);
}

#endif //ONELIFE_GAME_GRAPHIC_STRING_H
