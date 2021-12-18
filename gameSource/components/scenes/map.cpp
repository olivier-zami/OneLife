//
// Created by olivier on 18/12/2021.
//

#include "map.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

extern JenkinsRandomSource randSource2;

OneLife::game::Map::Map()
{
	this->mMapD = nullptr;
	this->mMap = nullptr;
	this->mMapBiomes = nullptr;
	this->mMapFloors = nullptr;
	this->mMapAnimationFrameCount = nullptr;
	this->mMapAnimationLastFrameCount = nullptr;
	this->mMapAnimationFrozenRotFrameCount = nullptr;
	this->mMapAnimationFrozenRotFrameCountUsed = nullptr;
	this->mMapFloorAnimationFrameCount = nullptr;
	this->mMapCurAnimType = nullptr;
	this->mMapLastAnimType = nullptr;
	this->mMapLastAnimFade = nullptr;
	this->mMapDropOffsets = nullptr;
	this->mMapDropRot = nullptr;
	this->mMapDropSounds = nullptr;
	this->mMapMoveOffsets = nullptr;
	this->mMapMoveSpeeds = nullptr;
	this->mMapTileFlips = nullptr;
	this->mMapPlayerPlacedFlags = nullptr;
}

OneLife::game::Map::~Map() {}

/**********************************************************************************************************************/

void OneLife::game::Map::handle(
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
		char** mMapPlayerPlacedFlags)
{
	this->mMapD = mMapD;
	this->mMap = mMap;
	this->mMapBiomes = mMapBiomes;
	this->mMapFloors = mMapFloors;
	this->mMapAnimationFrameCount = mMapAnimationFrameCount;
	this->mMapAnimationLastFrameCount = mMapAnimationLastFrameCount;
	this->mMapAnimationFrozenRotFrameCount = mMapAnimationFrozenRotFrameCount;
	this->mMapAnimationFrozenRotFrameCountUsed = mMapAnimationFrozenRotFrameCountUsed;
	this->mMapFloorAnimationFrameCount = mMapFloorAnimationFrameCount;
	this->mMapCurAnimType = mMapCurAnimType;
	this->mMapLastAnimType = mMapLastAnimType;
	this->mMapLastAnimFade = mMapLastAnimFade;
	this->mMapDropOffsets = mMapDropOffsets;
	this->mMapDropRot = mMapDropRot;
	this->mMapDropSounds = mMapDropSounds;
	this->mMapMoveOffsets = mMapMoveOffsets;
	this->mMapMoveSpeeds = mMapMoveSpeeds;
	this->mMapTileFlips = mMapTileFlips;
	this->mMapPlayerPlacedFlags = mMapPlayerPlacedFlags;
}

void OneLife::game::Map::reset()
{
	for(int i=0; i<(*this->mMapD) *(*this->mMapD); i++)
	{
		// -1 represents unknown
		// 0 represents known empty
		(*this->mMap)[i] = -1;
		(*this->mMapBiomes)[i] = -1;
		(*this->mMapFloors)[i] = -1;

		(*this->mMapAnimationFrameCount)[i] = randSource2.getRandomBoundedInt( 0, 10000 );
		(*this->mMapAnimationLastFrameCount)[i] = randSource2.getRandomBoundedInt( 0, 10000 );
		(*this->mMapAnimationFrozenRotFrameCount)[i] = 0;
		(*this->mMapAnimationFrozenRotFrameCountUsed)[i] = false;
		(*this->mMapFloorAnimationFrameCount)[i] = randSource2.getRandomBoundedInt( 0, 10000 );

		(*this->mMapCurAnimType)[i] = ground;
		(*this->mMapLastAnimType)[i] = ground;
		(*this->mMapLastAnimFade)[i] = 0;

		(*this->mMapDropOffsets)[i].x = 0;
		(*this->mMapDropOffsets)[i].y = 0;
		(*this->mMapDropRot)[i] = 0;
		(*this->mMapDropSounds)[i] = blankSoundUsage;

		(*this->mMapMoveOffsets)[i].x = 0;
		(*this->mMapMoveOffsets)[i].y = 0;
		(*this->mMapMoveSpeeds)[i] = 0;

		(*this->mMapTileFlips)[i] = false;

		(*this->mMapPlayerPlacedFlags)[i] = false;
	}
}
