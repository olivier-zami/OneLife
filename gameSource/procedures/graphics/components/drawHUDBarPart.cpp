//
// Created by olivier on 20/11/2021.
//

#include "../components.h"

#include <cmath>
#include "minorGems/game/gameGraphics.h"

extern SpriteHandle guiPanelTileSprite;
extern float gui_fov_scale_hud;

//FOV
void drawHUDBarPart( double x, double y, double width, double height ) {
	doublePair barPos[4] = {
			{ x, y + height },
			{ x + width, y + height },
			{ x + width, y },
			{ x, y }
	};
	double gapLength = abs( barPos[0].x - barPos[1].x ) / ( 256. * gui_fov_scale_hud );
	doublePair barTexCoords[4] = {
			{ 0.f, 0.f },
			{ gapLength, 0.f },
			{ gapLength, 1.f },
			{ 0.f , 1.f },
	};
	drawSprite( guiPanelTileSprite, barPos, barTexCoords );
}
