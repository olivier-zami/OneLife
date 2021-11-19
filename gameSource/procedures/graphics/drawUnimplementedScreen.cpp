//
// Created by olivier on 19/11/2021.
//

#include "screens.h"

#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"

void OneLife::game::graphic::drawUnimplementedScreen(void* screen)
{
	doublePair labelPos = { 0, 0 };
	drawMessage( "UNIMPLEMENTED SCREEN ...", labelPos, false );
}