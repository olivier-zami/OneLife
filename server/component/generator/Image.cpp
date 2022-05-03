//
// Created by olivier on 03/05/2022.
//

#include "Image.h"

#include <cstdio>

#include "../../../commonSource/fractalNoise.h"
#include "../../../gameSource/objectBank.h"
#include "../../../third_party/minorGems/graphics/Color.h"
#include "../../../third_party/minorGems/graphics/converters/TGAImageConverter.h"
#include "../../../third_party/minorGems/util/random/JenkinsRandomSource.h"
#include "../../../third_party/minorGems/util/SimpleVector.h"
#include "../../component/cache/Biome.h"
#include "../../dataType/cacheRecord.h"
#include "../Map.h"
#include "Biome.h"


extern SimpleVector<int> allNaturalMapIDs;
extern int numBiomes;
extern int* biomes;
extern OneLife::server::cache::Biome* cachedBiome;

/**
 * @note from server/map.cpp
 */
void outputMapImage()
{

	// output a chunk of the map as an image

	int w = 708;
	int h = 708;

	Image objIm(w, h, 3, true);
	Image biomeIm(w, h, 3, true);

	SimpleVector<Color> objColors;
	SimpleVector<Color> biomeColors;
	SimpleVector<int>   objCounts;

	int totalCounts = 0;

	for (int i = 0; i < allNaturalMapIDs.size(); i++)
	{
		Color *c = Color::makeColorFromHSV((float)i / allNaturalMapIDs.size(), 1, 1);
		objColors.push_back(*c);

		objCounts.push_back(0);

		delete c;
	}

	SimpleVector<int> biomeCounts;
	int               totalBiomeCount = 0;

	for (int j = 0; j < numBiomes; j++)
	{
		biomeCounts.push_back(0);

		Color *c;

		int biomeNumber = biomes[j];

		switch (biomeNumber)
		{
			case 0: c = new Color(0, 0.8, .1); break;
			case 1: c = new Color(0.4, 0.2, 0.7); break;
			case 2: c = new Color(1, .8, 0); break;
			case 3: c = new Color(0.6, 0.6, 0.6); break;
			case 4: c = new Color(1, 1, 1); break;
			case 5: c = new Color(0.7, 0.6, 0.0); break;
			case 6: c = new Color(0.0, 0.5, 0.0); break;
			case 7: c = new Color(0.0, 0.3, 1); break;
			case 8: c = new Color(0.2, 0.8, .1); break;
			default: c = Color::makeColorFromHSV((float)j / numBiomes, 1, 1);
		}

		biomeColors.push_back(*c);
		delete c;
	}

	/*
	double startTime = Time::getCurrentTime();
	for( int y = 0; y<h; y++ ) {

		for( int x = 0; x<w; x++ ) {
			// discard id output
			// just invoking this to time it
			getBaseMap( x, y );
			}
		}

	printf( "Generating %d map spots took %f sec\n",
			w * h, Time::getCurrentTime() - startTime );
	//exit(0);

	*/

	struct origin{
		int x;
		int y;
	}origin;
	//origin = {4781,-3886};
	origin = {0,0};

	for (int y = origin.y; y < h; y++)
	{

		for (int x = origin.x; x < w; x++)
		{

			/*
			// raw rand output and correlation test
			uint32_t xHit = xxTweakedHash2D( x, y );
			uint32_t yHit = xxTweakedHash2D( x+1, y+1 );
			//uint32_t xHit = getXYRandom_test( x, y );
			//uint32_t yHit = getXYRandom_test( x+1, y );

			xHit = xHit % w;
			yHit = yHit % h;

			double val = xxTweakedHash2D( x, y ) * oneOverIntMax;

			Color c = im.getColor( yHit * w + xHit );
			c.r += 0.1;
			c.g += 0.1;
			c.b += 0.1;

			im.setColor( yHit * w + xHit, c );

			c.r = val;
			c.g = val;
			c.b = val;

			//im.setColor( y * w + x, c );
			*/

			int id = getBaseMap(x - h / 2, -(y - h / 2));

			int biomeInd = getMapBiomeIndex(x - h / 2, -(y - h / 2));

			if (id > 0)
			{
				for (int i = 0; i < allNaturalMapIDs.size(); i++)
				{
					if (allNaturalMapIDs.getElementDirect(i) == id)
					{
						objIm.setColor(y * w + x, objColors.getElementDirect(i));

						(*objCounts.getElement(i))++;
						totalCounts++;
						break;
					}
				}
			}


			Color* tmpColor;
			if((x - h / 2)==0 ||  -(y - h / 2)==0)
			{
				tmpColor = new Color(0, 0, 0);
			}
			else
			{
				*tmpColor = biomeColors.getElementDirect(biomeInd);
			}
			biomeIm.setColor(y * w + x, *tmpColor);
			delete tmpColor; tmpColor = nullptr;
			(*(biomeCounts.getElement(biomeInd)))++;
			totalBiomeCount++;
		}
	}

	const char *biomeNames[] = {
			"Grass ", "Swamp ", "Yellow", "Gray  ", "Snow  ", "Desert", "Jungle", "Ocean ", "Flower"};

	for (int j = 0; j < numBiomes; j++)
	{
		const char *name = "unknwn";

		if (biomes[j] < 7) { name = biomeNames[biomes[j]]; }
		int c = biomeCounts.getElementDirect(j);

		printf("Biome %d (%s) \tcount = %d\t%.1f%%\n", biomes[j], name, c, 100 * (float)c / totalBiomeCount);
	}

	for (int i = 0; i < allNaturalMapIDs.size(); i++)
	{
		ObjectRecord *obj = getObject(allNaturalMapIDs.getElementDirect(i));

		int count = objCounts.getElementDirect(i);

		printf("%d\t%-30s  (actual=%f, expected=%f\n",
			   objCounts.getElementDirect(i),
			   obj->description,
			   count / (double)totalCounts,
			   obj->mapChance / allNaturalMapIDs.size());
	}

	// rough legend in corner
	for (int i = 0; i < allNaturalMapIDs.size(); i++)
	{
		if (i < h) { objIm.setColor(i * w + 0, objColors.getElementDirect(i)); }
	}

	File             tgaFile(NULL, "mapOut.tga");
	FileOutputStream tgaStream(&tgaFile);

	TGAImageConverter converter;

	converter.formatImage(&objIm, &tgaStream);

	File             tgaBiomeFile(NULL, "mapBiomeOut.tga");
	FileOutputStream tgaBiomeStream(&tgaBiomeFile);

	converter.formatImage(&biomeIm, &tgaBiomeStream);

	exit(0);
}

/**
 * @note from server/map.cpp
 */
void printBiomeSamples()
{
	int *biomeSamples = new int[numBiomes];

	for (int i = 0; i < numBiomes; i++)
	{
		biomeSamples[i] = 0;
	}

	JenkinsRandomSource sampleRandSource;

	int numSamples = 10000;

	int range = 2000;

	for (int i = 0; i < numSamples; i++)
	{
		int x = sampleRandSource.getRandomBoundedInt(-range, range);
		int y = sampleRandSource.getRandomBoundedInt(-range, range);

		int idxBiome;
		BiomeCacheRecord biomeCacheRecord = cachedBiome->getRecord(x, y);
		if(biomeCacheRecord.biome != -2)//TODO: (refacto) might be useless check it latter
		{
			idxBiome = biomeCacheRecord.biome;
		}
		else
		{
			idxBiome = computeMapBiomeIndex(x, y);
			//TODO: (refacto) store idxBiome in cache if required
		}
		biomeSamples[idxBiome]++;
	}

	for (int i = 0; i < numBiomes; i++)
	{
		printf("Biome %d:  %d (%.2f)\n", biomes[i], biomeSamples[i], biomeSamples[i] / (double)numSamples);
	}
}

/**
 * @note from server/map.cpp
 */
void outputBiomeFractals()
{
	for (int scale = 1; scale <= 4; scale *= 2)
	{

		for (int b = 0; b < numBiomes; b++)
		{
			int biome = biomes[b];

			setXYRandomSeed(biome * 263 + 723);

			int r = 100 * scale;

			Image outIm(r * 2, r * 2, 4);

			for (int y = -r; y < r; y++)
			{
				for (int x = -r; x < r; x++)
				{

					double v = getXYFractal(x, y, 0.55, scale);
					Color  c(v, v, v, 1);

					int imX = x + r;
					int imY = y + r;

					outIm.setColor(imY * 2 * r + imX, c);
				}
			}

			char *name = autoSprintf("fractal_b%d_s%d.tga", biome, scale);

			File tgaFile(NULL, name);

			FileOutputStream tgaStream(&tgaFile);

			TGAImageConverter converter;

			converter.formatImage(&outIm, &tgaStream);
			printf("Outputting file %s\n", name);

			delete[] name;
		}
	}
}