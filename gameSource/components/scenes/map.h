//
// Created by olivier on 18/12/2021.
//

#ifndef ONELIFE_GAME_COMPONENT_MAP_H
#define ONELIFE_GAME_COMPONENT_MAP_H

#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"
#include "OneLife/gameSource/GridPos.h"
#include "OneLife/gameSource/SoundUsage.h"
#include "OneLife/gameSource/animationBank.h"

namespace OneLife::game
{
	class Map
	{
		public:
			Map();
			~Map();

			void handle(
				int* mMapD,
				int** mMap,
				int** mMapBiomes,
				int** mMapFloors,
				double** mMapAnimationFrameCount,
				double** mMapAnimationLastFrameCount,
				double** mMapAnimationFrozenRotFrameCount,
				char** mMapAnimationFrozenRotFrameCountUsed,
				int** mMapFloorAnimationFrameCount,
				AnimType** mMapCurAnimType,
				AnimType** mMapLastAnimType,
				double** mMapLastAnimFade,
				doublePair** mMapDropOffsets,
				double** mMapDropRot,
				SoundUsage** mMapDropSounds,
				doublePair** mMapMoveOffsets,
				double** mMapMoveSpeeds,
				char** mMapTileFlips,
				char** mMapPlayerPlacedFlags,
				char* mMapGlobalOffsetSet,
				GridPos* mMapGlobalOffset,
				char** mMapCellDrawnFlags,
				SimpleVector<int>** mMapContainedStacks,
				SimpleVector<SimpleVector<int>>** mMapSubContainedStacks);

			void setGlobalOffset(GridPos offset);
			void reset();
			void applyOffset(int* x, int* y);
			bool isGlobalOffsetSet();

		private:
			int* mMapD;
			int** mMap;
			int** mMapBiomes;
			int** mMapFloors;
			double** mMapAnimationFrameCount;
			double** mMapAnimationLastFrameCount;
			double** mMapAnimationFrozenRotFrameCount;
			char** mMapAnimationFrozenRotFrameCountUsed;
			int** mMapFloorAnimationFrameCount;
			AnimType** mMapCurAnimType;
			AnimType** mMapLastAnimType;
			double** mMapLastAnimFade;
			doublePair** mMapDropOffsets;
			double** mMapDropRot;
			SoundUsage** mMapDropSounds;
			doublePair** mMapMoveOffsets;
			double** mMapMoveSpeeds;
			char** mMapTileFlips;
			char** mMapPlayerPlacedFlags;
			char* mMapGlobalOffsetSet;
			GridPos* mMapGlobalOffset;
			char** mMapCellDrawnFlags;
			SimpleVector<int>** mMapContainedStacks;
			SimpleVector<SimpleVector<int>>** mMapSubContainedStacks;
	};
}

#endif //ONELIFE_GAME_COMPONENT_MAP_H
