//
// Created by olivier on 17/11/2021.
//

#ifndef ONELIFE_GRAPHIC_MISC_H
#define ONELIFE_GRAPHIC_MISC_H

#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"

namespace OneLife::game
{
	void drawHungerMaxFillLine(
			doublePair inAteWordsPos,
			int inMaxFill,
			SpriteHandle *inBarSprites,
			SpriteHandle *inDashSprites,
			char inSkipBar,
			char inSkipDashes);
}

#endif //ONELIFE_GRAPHIC_MISC_H
