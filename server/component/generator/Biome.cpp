//
// Created by olivier on 01/05/2022.
//

#include "Biome.h"

#include <cstdio>
#include <cfloat>

#include "../../../commonSource/fractalNoise.h"

extern int* biomes;
extern int numBiomes;

float biomeTotalWeight;
float* biomeCumuWeights;
int regularBiomeLimit;
int numSpecialBiomes;
unsigned int biomeRandSeed = 723;

OneLife::server::generator::Biome::Biome() {}
OneLife::server::generator::Biome::~Biome() {}

/**
 *
 * @param inX
 * @param inY
 * @param outSecondPlaceIndex
 * @param outSecondPlaceGap
 * @return
 * @note from server/map.cpp
 * // new code, topographic rings
 */
int computeMapBiomeIndex(int inX, int inY, int *outSecondPlaceIndex, double *outSecondPlaceGap)
{
	// else cache miss
	int pickedBiome = -1;
	int secondPlace = -1;
	double secondPlaceGap = 0;

	setXYRandomSeed(biomeRandSeed);// try topographical altitude mapping
	double randVal = (getXYFractal(inX, inY, 0.55, 0.83332 + 0.08333 * numBiomes));

	// push into range 0..1, based on sampled min/max values
	randVal -= 0.099668;
	randVal *= 1.268963;

	// flatten middle
	// randVal = ( pow( 2*(randVal - 0.5 ), 3 ) + 1 ) / 2;

	// push into range 0..1 with manually tweaked values
	// these values make it pretty even in terms of distribution:
	// randVal -= 0.319;
	// randVal *= 3;

	// these values are more intuitve to make a map that looks good
	// randVal -= 0.23;
	// randVal *= 1.9;

	// apply gamma correction
	// randVal = pow( randVal, 1.5 );
	/*
	randVal += 0.4* sin( inX / 40.0 );
	randVal += 0.4 *sin( inY / 40.0 );

	randVal += 0.8;
	randVal /= 2.6;
	*/

	// slow arc n to s:

	// pow version has flat area in middle
	// randVal += 0.7 * pow( ( inY / 354.0 ), 3 ) ;

	// sin version
	// randVal += 0.3 * sin( 0.5 * M_PI * inY / 354.0 );

	/*
		( sin( M_PI * inY / 708 ) +
		  (1/3.0) * sin( 3 * M_PI * inY / 708 ) );
	*/
	// randVal += 0.5;
	// randVal /= 2.0;

	//OneLife::Debug::write("numBiome: %i", numBiomes);
	float i = randVal * biomeTotalWeight;
	pickedBiome = 0;
	while (pickedBiome < numBiomes && i > biomeCumuWeights[pickedBiome])
	{
		pickedBiome++;
	}
	if (pickedBiome >= numBiomes) { pickedBiome = numBiomes - 1; }

	if (pickedBiome >= regularBiomeLimit && numSpecialBiomes > 0)
	{
		// special case:  on a peak, place a special biome here

		// use patches mode for these
		pickedBiome = -1;

		double maxValue     = -10;
		double secondMaxVal = -10;

		for (int i = regularBiomeLimit; i < numBiomes; i++)
		{
			int biome = biomes[i];

			setXYRandomSeed(biome * 263 + biomeRandSeed + 38475);

			double randVal = getXYFractal(inX, inY, 0.55, 2.4999 + 0.2499 * numSpecialBiomes);

			if (randVal > maxValue)
			{
				if (maxValue != -10) { secondMaxVal = maxValue; }
				maxValue    = randVal;
				pickedBiome = i;
			}
		}

		if (maxValue - secondMaxVal < 0.03)
		{
			// close!  that means we're on a boundary between special biomes

			// stick last regular biome on this boundary, so special
			// biomes never touch
			secondPlace    = pickedBiome;
			secondPlaceGap = 0.1;
			pickedBiome    = regularBiomeLimit - 1;
		}
		else
		{
			secondPlace    = regularBiomeLimit - 1;
			secondPlaceGap = 0.1;
		}
	}
	else
	{
		// second place for regular biome rings

		secondPlace = pickedBiome - 1;
		if (secondPlace < 0) { secondPlace = pickedBiome + 1; }
		secondPlaceGap = 0.1;
	}

	//biomePutCached(inX, inY, pickedBiome, secondPlace, secondPlaceGap);//TODO remove if working (moved outside)

	if (outSecondPlaceIndex != NULL) { *outSecondPlaceIndex = secondPlace; }
	if (outSecondPlaceGap != NULL) { *outSecondPlaceGap = secondPlaceGap; }

	return pickedBiome;
}

/**
 *
 * @param inX
 * @param inY
 * @param outSecondPlaceIndex
 * @param outSecondPlaceGap
 * @return
 * @note from server/map.cpp
 * // old code, separate height fields per biome that compete
// and create a patchwork layout
 */
static int computeMapBiomeIndexOld(int inX, int inY, int *outSecondPlaceIndex = NULL, double *outSecondPlaceGap = NULL)
{
	int secondPlace = -1;
	double secondPlaceGap = 0;
	int pickedBiome = -1;
	double maxValue = -DBL_MAX;

	for (int i = 0; i < numBiomes; i++)
	{
		int biome = biomes[i];
		printf("biome: %d\n", biome);
		setXYRandomSeed(biome * 263 + biomeRandSeed);

		double randVal = getXYFractal(inX, inY, 0.55, 0.83332 + 0.08333 * numBiomes);

		if (randVal > maxValue)
		{
			// a new first place

			// old first moves into second
			secondPlace    = pickedBiome;
			secondPlaceGap = randVal - maxValue;

			maxValue    = randVal;
			pickedBiome = i;
		}
		else if (randVal > maxValue - secondPlaceGap)
		{
			// a better second place
			secondPlace    = i;
			secondPlaceGap = maxValue - randVal;
		}
	}

	//biomePutCached(inX, inY, pickedBiome, secondPlace, secondPlaceGap);//TODO remove if working (moved outside)

	if (outSecondPlaceIndex != NULL) { *outSecondPlaceIndex = secondPlace; }
	if (outSecondPlaceGap != NULL) { *outSecondPlaceGap = secondPlaceGap; }

	return pickedBiome;
}