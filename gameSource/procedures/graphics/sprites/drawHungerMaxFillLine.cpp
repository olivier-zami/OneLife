//
// Created by olivier on 17/11/2021.
//

#include "misc.h"

#include "minorGems/util/random/JenkinsRandomSource.h"
#include "OneLife/gameSource/procedures/maths/misc.h"
#include "OneLife/gameSource/components/pages/menu/playerStatus.h"

extern doublePair lastScreenViewCenter;
extern float gui_fov_scale;
extern float gui_fov_scale_hud;

void OneLife::game::drawHungerMaxFillLine(
		doublePair inAteWordsPos,
		int inMaxFill,
		SpriteHandle *inBarSprites,
		SpriteHandle *inDashSprites,
		char inSkipBar,
		char inSkipDashes)
{
	//FOV
	doublePair barPos = { lastScreenViewCenter.x - ( recalcOffsetX( 590 ) * gui_fov_scale ),
						  lastScreenViewCenter.y - ( recalcOffsetY( 334 ) * gui_fov_scale )};

	barPos.x -= 12 * gui_fov_scale_hud;
	barPos.y -= 10 * gui_fov_scale_hud;

	barPos.x += ( 30 * gui_fov_scale_hud ) * inMaxFill;

	if( ! inSkipBar )
	{
		drawSprite( inBarSprites[ inMaxFill % NUM_HUNGER_DASHES ], barPos, gui_fov_scale_hud );
	}

	if( inSkipDashes ) { return; }

	doublePair dashPos = inAteWordsPos;
	dashPos.y -= 6 * gui_fov_scale_hud;
	dashPos.x -= 5 * gui_fov_scale_hud;
	int numDashes = 0;
	JenkinsRandomSource dashRandSource( 0 );

	while( dashPos.x > barPos.x + 9 ) {

		doublePair drawPos = dashPos;

		//drawPos.x += dashRandSource.getRandomBoundedInt( -2, 2 );
		//drawPos.y += dashRandSource.getRandomBoundedInt( -1, 1 );

		drawSprite( inDashSprites[ numDashes % NUM_HUNGER_DASHES ], drawPos, gui_fov_scale_hud );
		dashPos.x -= 15 * gui_fov_scale_hud;
		//numDashes += dashRandSource.getRandomBoundedInt( 1, 10 );
		numDashes += 1;

		// correct shortness of last one
		if( numDashes % NUM_HUNGER_DASHES == 0 ) {dashPos.x += 3 * gui_fov_scale_hud;}
	}

	// draw one more to connect to bar
	dashPos.x = barPos.x + ( 6 * gui_fov_scale_hud );
	drawSprite( inDashSprites[ numDashes % NUM_HUNGER_DASHES ], dashPos, gui_fov_scale_hud );
}

