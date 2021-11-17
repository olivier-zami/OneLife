//
// Created by olivier on 16/11/2021.
//

#ifndef ONELIFE_GRAPHIC_GROUND_H
#define ONELIFE_GRAPHIC_GROUND_H

#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/SoundUsage.h"
#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/GridPos.h"//TODO put in dataType

namespace OneLife::graphic
{
	void drawMapCell(
			int inMapI,
			int inScreenX,
			int inScreenY,
			char inHighlightOnly,
			char inNoTimeEffects,
			int mMapD,
			int *mMap,
			double *mMapAnimationFrameCount,
			double *mMapAnimationLastFrameCount,
			double *mMapAnimationFrozenRotFrameCount,
			char *mMapAnimationFrozenRotFrameCountUsed,
			double *mMapLastAnimFade,
			double *mMapMoveSpeeds,
			AnimType *mMapCurAnimType,
			AnimType *mMapLastAnimType,
			doublePair *mMapDropOffsets,
			doublePair *mMapMoveOffsets,
			SoundUsage *mMapDropSounds,
			SimpleVector<int> *mMapContainedStacks,
			SimpleVector< SimpleVector<int> > *mMapSubContainedStacks,
			char mShowHighlights,
			SimpleVector<float> mPrevMouseOverSpotFades,
			SimpleVector<char> mPrevMouseOverSpotsBehind,
			SimpleVector<GridPos> mPrevMouseOverSpots,
			float mCurMouseOverFade,
			char mCurMouseOverBehind,
			GridPos mCurMouseOverSpot,
			char mCurMouseOverSelf,
			int mCurMouseOverID,
			char *mMapTileFlips,
			double *mMapDropRot,
			int *mMapFloors,
			int mMapOffsetX,
			int mMapOffsetY);
}

#endif //ONELIFE_GRAPHIC_GROUND_H
