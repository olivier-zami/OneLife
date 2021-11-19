//
// Created by olivier on 19/11/2021.
//

#include "screens.h"

#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"

void OneLife::game::graphic::drawInitializingScreen(void* screen)
{
	if(!screen) return;

	doublePair labelPos = { 0, 0 };
	drawMessage( "INITIALIZING", labelPos, false );
}