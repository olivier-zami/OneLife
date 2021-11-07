//
// Created by olivier on 01/11/2021.
//

#include "screenRenderer.h"

#include "minorGems/game/doublePair.h"
#include "OneLife/gameSource/components/GamePage.h"

extern GamePage *currentGamePage;
extern double viewWidth;
extern doublePair lastScreenViewCenter;

OneLife::game::ScreenRenderer::ScreenRenderer() {}
OneLife::game::ScreenRenderer::~ScreenRenderer() {}

void OneLife::game::ScreenRenderer::render()
{
	if(currentGamePage!= nullptr)
	{
		//printf("\n===>draw Screen ...");
		currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
	}
}