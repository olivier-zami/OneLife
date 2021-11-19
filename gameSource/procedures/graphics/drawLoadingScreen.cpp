//
// Created by olivier on 01/11/2021.
//

#include "screens.h"

#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/procedures/graphics/screens.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"//TODO: put in procedure/graphic directory
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"

void OneLife::game::graphic::drawLoadingScreen(void* screen)
{
	if(!screen) return;
	OneLife::dataType::uiComponent::LoadingScreen* loadingScreen;
	loadingScreen = (OneLife::dataType::uiComponent::LoadingScreen*) screen;

	doublePair labelPos = { 0, 0 };
	drawMessage( "LOADING", labelPos, false );
	labelPos.y = -100;
	drawMessage( loadingScreen->taskName, labelPos, false );
	if( loadingScreen->status.showProgressBar )
	{
		setDrawColor( 1, 1, 1, 1 );// border
		drawRect( -100, -220, 100, -200 );

		setDrawColor( 0, 0, 0, 1 );// inner black
		drawRect(-98, -218, 98, -202);

		setDrawColor( .8, .8, .8, 1 ); // progress
		drawRect( -98, -218, -98 + loadingScreen->progressBar * ( 98 * 2 ), -202 );
	}
}