//
// Created by olivier on 25/10/2021.
//

#include "drawGround.h"

#include "minorGems/game/gameGraphics.h"
#include "OneLife/gameSource/components/engines/gameSceneHandler.h"// setViewCenterPosition()
#include "OneLife/gameSource/groundSprites.h"// CELL_D + GroundSpriteSet()
#include "OneLife/commonSource/fractalNoise.h"
#include "OneLife/gameSource/procedures/animations/special.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "OneLife/gameSource/spriteBank.h"

#define isInBounds(X, Y, Z)   !(X<0 || Y<0 || X>Z-1 || Y>Z-1 )

void drawGround(
	doublePair lastScreenViewCenter,
	int gridCenterX,//from LivingLifePage
	int gridCenterY,//from LivingLifePage
	float gui_fov_scale,//from game.cpp
	bool isTrippingEffectOn,//from game.cpp
	int mapWidth,//mMapD from LivingLifePage
	int mapHeight,//mMapD from LivingLifePage
	char *mMapCellDrawnFlags,//from LivingLifePage
	int groundSpritesArraySize,//#define groundSprite.h
	int mMapOffsetX,//from LivingLifePage::mMapOffsetX
	int mMapOffsetY,//from LivingLifePage::mMapOffsetY
	int *mMapBiomes,// from LivingLifePage::mMapBiomes
	GroundSpriteSet **groundSprites,//from groundSprite.h
	int valleyOffset,//from livingLifePage
	SimpleVector<int> mCulvertStoneSpriteIDs,//from livingLifePage
	double culvertFractalRoughness,
	double culvertFractalScale,
	double culvertFractalAmp,
	int valleySpacing)//from livingLifePage
{
	int mMapD = mapWidth;

	setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

	setDrawColor( 1, 1, 1, 1 );

// don't bound floor start and end here
//
// we want to show unknown biome off edge
// instead, check before using to index mMapBiomes mid-loop

// note that we can't check mMapCellDrawnFlags outside of map boundaries
// which will result in some over-drawing out there (whole sheets with
// tiles drawn on top).  However, given that we're not drawing anything
// else out there, this should be okay from a performance standpoint.


	int yStartFloor = gridCenterY - (int)(ceil(4 * gui_fov_scale + 1));
	int yEndFloor = gridCenterY + (int)(ceil(4 * gui_fov_scale));

	int xStartFloor = gridCenterX - (int)(ceil(6 * gui_fov_scale));
	int xEndFloor = gridCenterX + (int)(ceil(6 * gui_fov_scale) + 1);


	int numCells = mMapD * mMapD;

	memset( mMapCellDrawnFlags, false, numCells );

// draw underlying ground biomes

// two passes
// once for biomes
// second time for overlay shading on y-culvert lines
	for( int pass=0; pass<2; pass++ ) {
		for (int y = yEndFloor; y >= yStartFloor; y--) {

			int screenY = CELL_D * (y + mMapOffsetY - mMapD / 2);

			int tileY = -lrint(screenY / CELL_D);

			int tileWorldY = -tileY;

// slight offset to compensate for tile overlaps and
// make biome tiles more centered on world tiles
			screenY -= 32;

			for (int x = xStartFloor; x <= xEndFloor; x++) {
				int mapI = y * mMapD + x;

				char inBounds = !(x<0 || y<0 || x>mMapD-1 || y>mMapD-1 ); //isInBounds(x, y, mMapD);

				if (pass == 0 && inBounds && mMapCellDrawnFlags[mapI]) {
					continue;
				}

				int screenX = CELL_D * (x + mMapOffsetX - mMapD / 2);

				int tileX = lrint(screenX / CELL_D);

// slight offset to compensate for tile overlaps and
// make biome tiles more centered on world tiles
				screenX += 32;

				int b = -1;

				if (inBounds) {
					b = mMapBiomes[mapI];
				}

				GroundSpriteSet *s = NULL;

				setDrawColor(1, 1, 1, 1);

				if (b >= 0 && b < groundSpritesArraySize) {
					s = groundSprites[b];
				} else if (b == -1) {
// unknown biome image at end
					s = groundSprites[groundSpritesArraySize - 1];
				}

				if (s == NULL) {

// use end image with random color
					s = groundSprites[groundSpritesArraySize - 1];

// random draw color
					setDrawColor(getXYRandom(b, b),
								 getXYRandom(b, b + 100),
								 getXYRandom(b, b + 300), 1);
/*
                // find another
                for( int i=0; i<groundSpritesArraySize && s == NULL; i++ ) {
                    s = groundSprites[ i ];
                    }
                */
				}


				if (s != NULL) {


					doublePair pos = {(double) screenX, (double) screenY};


// wrap around
					int setY = tileY % s->numTilesHigh;
					int setX = tileX % s->numTilesWide;

					if (setY < 0) {
						setY += s->numTilesHigh;
					}
					if (setX < 0) {
						setX += s->numTilesHigh;
					}

					if (pass == 0)
						if (setY == 0 && setX == 0) {

// check if we're on corner of all-same-biome region that
// we can fill with one big sheet

							char allSameBiome = true;

// check borders of would-be sheet too
							for (int nY = y + 1; nY >= y - s->numTilesHigh; nY--) {

								if (nY >= 0 && nY < mMapD) {

									for (int nX = x - 1;
										 nX <= x + s->numTilesWide; nX++) {

										if (nX >= 0 && nX < mMapD) {
											int nI = nY * mMapD + nX;

											int nB = -1;

											if (isInBounds(nX, nY, mMapD)) {
												nB = mMapBiomes[nI];
											}

											if (nB != b) {
												allSameBiome = false;
												break;
											}
										}
									}

								}
								if (!allSameBiome) {
									break;
								}
							}

							if (allSameBiome) {

								doublePair lastCornerPos =
										{pos.x + (s->numTilesWide - 1) * CELL_D,
										 pos.y - (s->numTilesHigh - 1) * CELL_D};

								doublePair sheetPos = mult(add(pos, lastCornerPos),
														   0.5);

								if (!isTrippingEffectOn)
									drawSprite(s->wholeSheet,
											   sheetPos);// All tiles are drawn to change color independently

								if (!isTrippingEffectOn)
// mark all cells under sheet as drawn
									for (int sY = y; sY > y - s->numTilesHigh; sY--) {

										if (sY >= 0 && sY < mMapD) {

											for (int sX = x;
												 sX < x + s->numTilesWide; sX++) {

												if (sX >= 0 && sX < mMapD) {
													int sI = sY * mMapD + sX;

													mMapCellDrawnFlags[sI] = true;
												}
											}
										}
									}
							}
						}

					if (pass == 0)
						if (!inBounds || !mMapCellDrawnFlags[mapI]) {
// not drawn as whole sheet

							int aboveB = -1;
							int leftB = -1;
							int diagB = -1;

							if (isInBounds(x - 1, y, mMapD)) {
								leftB = mMapBiomes[mapI - 1];
							}
							if (isInBounds(x, y + 1, mMapD)) {
								aboveB = mMapBiomes[mapI + mMapD];
							}

							if (isInBounds(x + 1, y + 1, mMapD)) {
								diagB = mMapBiomes[mapI + mMapD + 1];
							}

							if (isTrippingEffectOn) setTrippingColor(pos.x, pos.y);

							if (!isTrippingEffectOn && // All tiles are drawn to change color independently
								leftB == b &&
								aboveB == b &&
								diagB == b) {

// surrounded by same biome above and to left
// AND diagonally to the above-right
// draw square tile here to save pixel fill time
								drawSprite(s->squareTiles[setY][setX], pos);
							} else {
								drawSprite(s->tiles[setY][setX], pos);
							}
							if (inBounds) {
								mMapCellDrawnFlags[mapI] = true;
							}
						}

					if (pass == 1) {
						int yMod = abs(tileWorldY + valleyOffset) % valleySpacing;

// on a culvert fault line?
						if (yMod == 0) {

							setDrawColor(0, 0, 0, 0.625);

							JenkinsRandomSource stonePicker(tileX);

							if (mCulvertStoneSpriteIDs.size() > 0) {

								for (int s = 0; s < 2; s++) {

									int stoneIndex =
											stonePicker.getRandomBoundedInt(
													0,
													mCulvertStoneSpriteIDs.size() - 1);

									int stoneSpriteID =
											mCulvertStoneSpriteIDs.getElementDirect(
													stoneIndex);

									doublePair stoneJigglePos = pos;

									if (s == 1) {
										stoneJigglePos.x += CELL_D / 2;
									}

									stoneJigglePos.y -= 16;

									stoneJigglePos.y +=
											getXYFractal(stoneJigglePos.x,
														 stoneJigglePos.y,
														 culvertFractalRoughness,
														 culvertFractalScale) *
											culvertFractalAmp;

									drawSprite(getSprite(stoneSpriteID),stoneJigglePos);
								}
							}
						}
					}
				}
			}
		}
	}
}