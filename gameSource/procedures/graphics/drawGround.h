//
// Created by olivier on 25/10/2021.
//

#ifndef ONELIFE_PROCEDURE_GRAPHIC_DRAWGROUND_H
#define ONELIFE_PROCEDURE_GRAPHIC_DRAWGROUND_H

#include "minorGems/game/doublePair.h"
#include "OneLife/gameSource/groundSprites.h"
#include "minorGems/util/SimpleVector.h"

void drawGround(
	doublePair lastScreenViewCenter,
	int gridCenterX,
	int gridCenterY,
	float gui_fov_scale,
	bool isTrippingEffectOn,
	int mapWidth,
	int mapHeight,
	char *mMapCellDrawnFlags,
	int groundSpritesArraySize,
	int mMapOffsetX,
	int mMapOffsetY,
	int *mMapBiomes,
	GroundSpriteSet **groundSprites,
	int valleyOffset,
	SimpleVector<int> mCulvertStoneSpriteIDs,
	double culvertFractalRoughness,
	double culvertFractalScale,
	double culvertFractalAmp,
	int valleySpacing);


#endif //ONELIFE_PROCEDURE_GRAPHIC_DRAWGROUND_H
