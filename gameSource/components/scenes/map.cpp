//
// Created by olivier on 18/12/2021.
//

#include "map.h"

#include "minorGems/util/random/JenkinsRandomSource.h"
#include "OneLife/gameSource/debug/console.h"

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
	this->mMapGlobalOffsetSet = nullptr;
	this->mMapGlobalOffset = nullptr;
	this->mMapContainedStacks = nullptr;
	this->mMapSubContainedStacks = nullptr;
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
		char** mMapPlayerPlacedFlags,
		char* mMapGlobalOffsetSet,
		GridPos* mMapGlobalOffset,
		char** mMapCellDrawnFlags,
		SimpleVector<int>** mMapContainedStacks,
		SimpleVector<SimpleVector<int>>** mMapSubContainedStacks)
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
	this->mMapGlobalOffsetSet = mMapGlobalOffsetSet;
	this->mMapGlobalOffset = mMapGlobalOffset;
	this->mMapCellDrawnFlags = mMapCellDrawnFlags;
	this->mMapContainedStacks = mMapContainedStacks;
	this->mMapSubContainedStacks = mMapSubContainedStacks;

	//TODO: put below code in constructor after transfer from LivingLifePage done
	*this->mMap = new int[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapBiomes = new int[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapFloors = new int[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapCellDrawnFlags = new char[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapContainedStacks = new SimpleVector<int>[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapSubContainedStacks = new SimpleVector< SimpleVector<int> >[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapAnimationFrameCount =  new double[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapAnimationLastFrameCount =  new double[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapAnimationFrozenRotFrameCount =  new double[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapAnimationFrozenRotFrameCountUsed =  new char[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapFloorAnimationFrameCount =  new int[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapCurAnimType =  new AnimType[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapLastAnimType =  new AnimType[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapLastAnimFade =  new double[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapDropOffsets = new doublePair[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapDropRot = new double[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapDropSounds = new SoundUsage[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapMoveOffsets = new doublePair[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapMoveSpeeds = new double[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapTileFlips = new char[ (*this->mMapD) * (*this->mMapD) ];
	*this->mMapPlayerPlacedFlags = new char[ (*this->mMapD) * (*this->mMapD) ];
}

void OneLife::game::Map::setGlobalOffset(GridPos offset)
{
// we need 7 fraction bits to represent 128 pixels per tile
	// 32-bit float has 23 significant bits, not counting sign
	// that leaves 16 bits for tile coordinate, or 65,536
	// Give two extra bits of wiggle room
	int maxOK = 16384;

	if( offset.x < maxOK && offset.x > -maxOK && offset.y < maxOK && offset.y > -maxOK )
	{
		OneLife::debug::Console::write("First chunk isn't too far from center, using 0,0 as our global offset\n" );
		(*this->mMapGlobalOffset).x = 0;
		(*this->mMapGlobalOffset).y = 0;
		(*this->mMapGlobalOffsetSet) = true;
	}
	else
	{
		OneLife::debug::Console::write("Using this first chunk center as our global offset:  %d, %d\n", offset.x, offset.y );
		(*this->mMapGlobalOffset).x = offset.x;
		(*this->mMapGlobalOffset).y = offset.y;
		(*this->mMapGlobalOffsetSet) = true;
	}
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

void OneLife::game::Map::applyOffset(int* x, int* y)//TODO: remove applyReceiveOffset from LivingLifePage
{
	if(*this->mMapGlobalOffsetSet)
	{
		*x -= (*this->mMapGlobalOffset).x;
		*y -= (*this->mMapGlobalOffset).y;
	}
}

bool OneLife::game::Map::isGlobalOffsetSet()
{
	return (bool)(*this->mMapGlobalOffsetSet);
}
