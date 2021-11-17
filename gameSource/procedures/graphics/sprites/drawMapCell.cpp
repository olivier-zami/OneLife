//
// Created by olivier on 16/11/2021.
//

#include "map.h"

#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/groundSprites.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/components/camera.h"
#include "OneLife/gameSource/components/engines/audioRenderer.h"

extern char mapPullMode;
extern double frameRateFactor;

void OneLife::graphic::drawMapCell(
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
		int mMapOffsetY)
{
	int oID = mMap[ inMapI ];
	int objectHeight = 0;
	if( oID > 0 )
	{
		objectHeight = getObjectHeight( oID );
		double oldFrameCount = mMapAnimationFrameCount[ inMapI ];
		if( !mapPullMode && !inHighlightOnly && !inNoTimeEffects )
		{
			if( mMapCurAnimType[ inMapI ] == moving ) {
				double animSpeed = 1.0;
				ObjectRecord *movingObj = getObject( oID );

				if( movingObj->speedMult < 1.0 ) {
					// only slow anims down don't speed them up
					animSpeed *= movingObj->speedMult;
				}

				mMapAnimationFrameCount[ inMapI ] += animSpeed;
				mMapAnimationLastFrameCount[ inMapI ] += animSpeed;
				mMapAnimationFrozenRotFrameCount[ inMapI ] += animSpeed;
				mMapAnimationFrozenRotFrameCountUsed[ inMapI ] = false;
			}
			else {
				mMapAnimationFrameCount[ inMapI ] ++;
				mMapAnimationLastFrameCount[ inMapI ] ++;
			}


			if( mMapLastAnimFade[ inMapI ] > 0 ) {
				mMapLastAnimFade[ inMapI ] -= 0.05 * frameRateFactor;
				if( mMapLastAnimFade[ inMapI ] < 0 ) {
					mMapLastAnimFade[ inMapI ] = 0;

					AnimType newType;

					if( mMapMoveSpeeds[ inMapI ] == 0 ) {
						newType = ground;
					}
					else if( mMapMoveSpeeds[ inMapI ] > 0 ) {
						// transition to moving now
						newType = moving;
					}

					if( mMapCurAnimType[ inMapI ] != newType ) {

						mMapLastAnimType[ inMapI ] = mMapCurAnimType[ inMapI ];
						mMapCurAnimType[ inMapI ] = newType;
						mMapLastAnimFade[ inMapI ] = 1;

						mMapAnimationLastFrameCount[ inMapI ] =
								mMapAnimationFrameCount[ inMapI ];


						if( newType == moving &&
							mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ) {
							mMapAnimationFrameCount[ inMapI ] =
									mMapAnimationFrozenRotFrameCount[ inMapI ];
						}
					}

				}
			}
		}

		doublePair pos = { (double)inScreenX, (double)inScreenY };
		double rot = 0;

		if( mMapDropOffsets[ inMapI ].x != 0 ||
			mMapDropOffsets[ inMapI ].y != 0 ) {

			doublePair nullOffset = { 0, 0 };


			doublePair delta = sub( nullOffset,
					mMapDropOffsets[ inMapI ] );

			double step = frameRateFactor * 0.0625;
			double rotStep = frameRateFactor * 0.03125;

			if( length( delta ) < step ) {

				mMapDropOffsets[ inMapI ].x = 0;
				mMapDropOffsets[ inMapI ].y = 0;
			}
			else {
				mMapDropOffsets[ inMapI ] =
						add( mMapDropOffsets[ inMapI ],
								mult( normalize( delta ), step ) );
			}

			if( mMapDropOffsets[ inMapI ].x == 0 &&
				mMapDropOffsets[ inMapI ].y == 0 ) {
				// done dropping into place
				if( mMapDropSounds[ inMapI ].numSubSounds > 0 ) {

					playSound( mMapDropSounds[ inMapI ],
							getVectorFromCamera(
									(double)inScreenX / CELL_D,
									(double)inScreenY / CELL_D ) );
					mMapDropSounds[ inMapI ] = blankSoundUsage;
				}

			}



			double rotDelta = 0 - mMapDropRot[ inMapI ];

			if( rotDelta > 0.5 ) {
				rotDelta = rotDelta - 1;
			}
			else if( rotDelta < -0.5 ) {
				rotDelta = 1 + rotDelta;
			}

			if( fabs( rotDelta ) < rotStep ) {
				mMapDropRot[ inMapI ] = 0;
			}
			else {
				double rotSign = 1;
				if( rotDelta < 0 ) {
					rotSign = -1;
				}

				mMapDropRot[ inMapI ] =
						mMapDropRot[ inMapI ] + rotSign * rotStep;
			}



			// step offset BEFORE applying it
			// (so we don't repeat starting position)
			pos = add( pos, mult( mMapDropOffsets[ inMapI ], CELL_D ) );

			rot = mMapDropRot[ inMapI ];
		}

		if( mMapMoveSpeeds[inMapI] > 0 &&
			( mMapMoveOffsets[ inMapI ].x != 0 ||
			  mMapMoveOffsets[ inMapI ].y != 0  ) ) {

			pos = add( pos, mult( mMapMoveOffsets[ inMapI ], CELL_D ) );
		}



		setDrawColor( 1, 1, 1, 1 );

		AnimType curType = ground;
		AnimType fadeTargetType = ground;
		double animFade = 1;

		if( mMapMoveSpeeds[ inMapI ] > 0 ) {
			curType = moving;
			fadeTargetType = moving;
			animFade = 1;
		}



		double timeVal = frameRateFactor *
						 mMapAnimationFrameCount[ inMapI ] / 60.0;

		double frozenRotTimeVal = frameRateFactor *
								  mMapAnimationFrozenRotFrameCount[ inMapI ] / 60.0;

		double targetTimeVal = timeVal;

		if( mMapLastAnimFade[ inMapI ] != 0 ) {
			animFade = mMapLastAnimFade[ inMapI ];
			curType = mMapLastAnimType[ inMapI ];
			fadeTargetType = mMapCurAnimType[ inMapI ];

			timeVal = frameRateFactor *
					  mMapAnimationLastFrameCount[ inMapI ] / 60.0;
		}



		char flip = mMapTileFlips[ inMapI ];

		ObjectRecord *obj = getObject( oID );
		if( obj->noFlip ||
			( obj->permanent &&
			  ( obj->blocksWalking || obj->drawBehindPlayer ||
				obj->anySpritesBehindPlayer) ) ) {
			// permanent, blocking objects (e.g., walls)
			// or permanent behind-player objects (e.g., roads)
			// are never drawn flipped
			flip = false;
			// remember that this tile is NOT flipped, so that it
			// won't flip back strangely if it changes to something
			// that doesn't have a noFlip status
			mMapTileFlips[ inMapI ] = false;
		}

		char highlight = false;
		float highlightFade = 1.0f;

		if( mCurMouseOverID > 0 &&
			! mCurMouseOverSelf &&
			mCurMouseOverSpot.y * mMapD + mCurMouseOverSpot.x == inMapI ) {

			if( mCurMouseOverBehind ) {
				highlight = inHighlightOnly;
			}
			else {
				highlight = true;
			}

			highlightFade = mCurMouseOverFade;
		}
		else {
			for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
				GridPos prev = mPrevMouseOverSpots.getElementDirect( i );

				if( prev.y * mMapD + prev.x == inMapI ) {
					if( mPrevMouseOverSpotsBehind.getElementDirect( i ) ) {
						highlight = inHighlightOnly;
					}
					else {
						highlight = true;
					}
					highlightFade = mPrevMouseOverSpotFades.getElementDirect(i);
				}
			}
		}

		if( ! mShowHighlights ) {
			if( inHighlightOnly ) {
				return;
			}
			highlight = false;
		}


		if( !mapPullMode && !inHighlightOnly && !inNoTimeEffects ) //play agent sound !!!
		{
			/********************************************************************************************************* TODO: uncomment
			handleAnimSound( oID, 0, mMapCurAnimType[ inMapI ], oldFrameCount,
					mMapAnimationFrameCount[ inMapI ],
					pos.x / CELL_D,
					pos.y / CELL_D );
			****/
			OneLife::game::handleAnimSound(
					oID, //int inObjectID,
					0, //double inAge,
					mMapCurAnimType[ inMapI ], //AnimType inType,
					oldFrameCount, //int inOldFrameCount,
					mMapAnimationFrameCount[ inMapI ], //int inNewFrameCount,
					pos.x / CELL_D, //double inPosX,
					pos.y / CELL_D, //double inPosY,
					mMapFloors, //int *
					mMapD, //int
					mMapOffsetX, //int
					mMapOffsetY); //int
		}


		if( highlight && obj->noHighlight ) {
			if( inHighlightOnly ) {
				return;
			}
			highlight = false;
		}


		int numPasses = 1;
		int startPass = 0;

		if( highlight ) {

			// first pass normal draw
			// then three stencil passes (second and third with a subtraction)
			numPasses = 6;

			if( highlightFade != 1.0f ) {
				//fadeHandle = addGlobalFade( highlightFade );
			}

			if( inHighlightOnly ) {
				startPass = 1;
			}
		}

		for( int i=startPass; i<numPasses; i++ )
		{
			doublePair passPos = pos;

			if( highlight ) {

				switch( i ) {
					case 0:
						// normal color draw
						break;
					case 1:
						// opaque portion
						startAddingToStencil( false, true, .99f );
						break;
					case 2:
						// first fringe
						startAddingToStencil( false, true, .07f );
						break;
					case 3:
						// subtract opaque from fringe to get just first fringe
						startAddingToStencil( false, false, .99f );
						break;
					case 4:
						// second fringe
						// ignore totally transparent stuff
						// like invisible animation layers
						startAddingToStencil( false, true, 0.01f );
						break;
					case 5:
						// subtract first fringe from fringe to get
						// just secon fringe
						startAddingToStencil( false, false, .07f );
						break;
					default:
						break;
				}

			}

			if( mMapContainedStacks[ inMapI ].size() > 0 ) {
				int *stackArray =
						mMapContainedStacks[ inMapI ].getElementArray();
				SimpleVector<int> *subStackArray =
						mMapSubContainedStacks[ inMapI ].getElementArray();

				drawObjectAnim( oID,
						curType, timeVal,
						animFade,
						fadeTargetType,
						targetTimeVal,
						frozenRotTimeVal,
						&( mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ),
						endAnimType,
						endAnimType,
						passPos, rot, false, flip,
						-1,
						false, false, false,
						getEmptyClothingSet(),
						NULL,
						mMapContainedStacks[ inMapI ].size(),
						stackArray, subStackArray );
				delete [] stackArray;
				delete [] subStackArray;
			}
			else {
				drawObjectAnim( oID, 2,
						curType, timeVal,
						animFade,
						fadeTargetType,
						targetTimeVal,
						frozenRotTimeVal,
						&( mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ),
						endAnimType,
						endAnimType,
						passPos, rot,
						false,
						flip, -1,
						false, false, false,
						getEmptyClothingSet(), NULL );
			}


			if( highlight ) {


				float mainFade = .35f;

				toggleAdditiveBlend( true );

				doublePair squarePos = passPos;

				if( objectHeight > 1.5 * CELL_D ) {
					squarePos.y += 192;
				}

				int squareRad = 306;

				switch( i )
				{
					case 0:
						// normal color draw
						break;
					case 1:
						// opaque portion
						startDrawingThroughStencil( false );

						setDrawColor( 1, 1, 1, highlightFade * mainFade );

						drawSquare( squarePos, squareRad );

						stopStencil();
						break;
					case 2:
						// first fringe
						// wait until next pass to isolate fringe
						break;
					case 3:
						// now first fringe is isolated in stencil
						startDrawingThroughStencil( false );

						setDrawColor( 1, 1, 1, highlightFade * mainFade * .5 );

						drawSquare( squarePos, squareRad );

						stopStencil();
						break;
					case 4:
						// second fringe
						// wait until next pass to isolate fringe
						break;
					case 5:
						// now second fringe is isolated in stencil
						startDrawingThroughStencil( false );

						setDrawColor( 1, 1, 1, highlightFade * mainFade *.25 );

						drawSquare( squarePos, squareRad );

						stopStencil();
						break;
					default:
						break;
				}
				toggleAdditiveBlend( false );
			}
		}
	}
	else if( oID == -1 )
	{
		// unknown
		doublePair pos = { (double)inScreenX, (double)inScreenY };

		setDrawColor( 0, 0, 0, 0.5 );
		drawSquare( pos, 14 );
	}
}

