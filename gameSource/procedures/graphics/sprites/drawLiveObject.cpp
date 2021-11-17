//
// Created by olivier on 17/11/2021.
//

#include "agents.h"

#include <cmath>
#include "minorGems/game/gameGraphics.h"
#include "OneLife/gameSource/groundSprites.h"// CELL_D + GroundSpriteSet()
#include "OneLife/gameSource/components/camera.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/dataTypes/const.h"
#include "OneLife/gameSource/scenes1/agent.h"
#include "OneLife/gameSource/ageControl.h"

extern double frameRateFactor;
extern SimpleVector<doublePair> trail;
extern SimpleVector<FloatColor> trailColors;
extern int playerActionTargetX;
extern int playerActionTargetY;
extern int ourID;
extern SimpleVector<LiveObject> gameObjects;
extern FloatColor trailColor;

ObjectAnimPack OneLife::game::drawLiveObject(
		LiveObject *inObj,
		SimpleVector<LiveObject *> *inSpeakers,
		SimpleVector<doublePair> *inSpeakersPos )
{
	ObjectAnimPack returnPack;
	returnPack.inObjectID = -1;
	if( inObj->hide || inObj->outOfRange ) {return returnPack;}

	inObj->onScreen = true;



	if( ! inObj->allSpritesLoaded ) {
		// skip drawing until fully loaded
		return returnPack;
	}

	// current pos

	doublePair pos = mult( inObj->currentPos, CELL_D );


	if( inObj->heldByDropOffset.x != 0 ||
		inObj->heldByDropOffset.y != 0 ) {

		doublePair nullOffset = { 0, 0 };


		doublePair delta = sub( nullOffset,
				inObj->heldByDropOffset );

		double step = frameRateFactor * 0.0625;

		if( length( delta ) < step ) {

			inObj->heldByDropOffset.x = 0;
			inObj->heldByDropOffset.y = 0;

			ObjectRecord *displayObj =
					getObject( inObj->displayID );

			if( displayObj->usingSound.numSubSounds > 0 ) {
				// play baby's using sound as they are put down
				// we no longer have access to parent's using sound
				playSound( displayObj->usingSound,
						getVectorFromCamera(
								inObj->currentPos.x, inObj->currentPos.y ) );
			}
		}
		else {
			inObj->heldByDropOffset =
					add( inObj->heldByDropOffset,
							mult( normalize( delta ), step ) );
		}

		// step offset BEFORE applying it
		// (so we don't repeat starting position)
		pos = add( pos, mult( inObj->heldByDropOffset, CELL_D ) );
	}


	doublePair actionOffset = { 0, 0 };

	if( false )
		if( inObj->curAnim == moving ) {
			trail.push_back( pos );
			trailColors.push_back( trailColor );

			while( trail.size() > 1000 ) {
				trail.deleteElement( 0 );
				trailColors.deleteElement( 0 );
			}
		}


	int targetX = playerActionTargetX;
	int targetY = playerActionTargetY;

	if( inObj->id != ourID ) {
		targetX = inObj->actionTargetX;
		targetY = inObj->actionTargetY;
	}
	else {
		setClothingHighlightFades( inObj->clothingHighlightFades );
	}


	if( inObj->curAnim != eating &&
		inObj->lastAnim != eating &&
		inObj->pendingActionAnimationProgress != 0 ) {

		// wiggle toward target


		int trueTargetX = targetX + inObj->actionTargetTweakX;
		int trueTargetY = targetY + inObj->actionTargetTweakY;


		float xDir = 0;
		float yDir = 0;

		doublePair dir = { trueTargetX - inObj->currentPos.x,
						   trueTargetY - inObj->currentPos.y };

		if( dir.x != 0 || dir.y != 0 ) {
			dir = normalize( dir );
		}

		xDir = dir.x;
		yDir = dir.y;

		if( inObj->currentPos.x < trueTargetX ) {
			if( inObj->currentSpeed == 0 ) {
				inObj->holdingFlip = false;
			}
		}
		if( inObj->currentPos.x > trueTargetX ) {
			if( inObj->currentSpeed == 0 ) {
				inObj->holdingFlip = true;
			}
		}

		double wiggleMax = CELL_D *.5 *.90;

		double halfWiggleMax = wiggleMax * 0.5;

		if( xDir == 0 && yDir == 0 ) {
			// target where we're standing
			// wiggle tiny bit down
			yDir = -1;

			halfWiggleMax *= 0.25;
		}
		else if( xDir == 0 && yDir == -1 ) {
			// moving down causes feet to cross object in our same tile
			// move less
			halfWiggleMax *= 0.5;
		}



		double offset =
				halfWiggleMax -
				halfWiggleMax *
				cos( 2 * M_PI * inObj->pendingActionAnimationProgress );


		actionOffset.x += xDir * offset;
		actionOffset.y += yDir * offset;
	}


	// bare hands action OR holding something
	// character wiggle
	if( inObj->pendingActionAnimationProgress != 0 ) {

		pos = add( pos, actionOffset );
	}


	AnimType curType = inObj->curAnim;
	AnimType fadeTargetType = inObj->curAnim;

	double animFade = 1.0;

	double timeVal = frameRateFactor *
					 inObj->animationFrameCount / 60.0;

	double targetTimeVal = timeVal;

	double frozenRotTimeVal = frameRateFactor *
							  inObj->frozenRotFrameCount / 60.0;

	if( inObj->lastAnimFade > 0 ) {
		curType = inObj->lastAnim;
		fadeTargetType = inObj->curAnim;
		animFade = inObj->lastAnimFade;

		timeVal = frameRateFactor *
				  inObj->lastAnimationFrameCount / 60.0;
	}


	setDrawColor( 1, 1, 1, 1 );
	//setDrawColor( red, 0, blue, 1 );
	//mainFont->drawString( string,
	//                      pos, alignCenter );

	double age = computeCurrentAge( inObj );


	ObjectRecord *heldObject = NULL;

	int hideClosestArm = 0;
	char hideAllLimbs = false;


	if( inObj->holdingID != 0 ) {
		if( inObj->holdingID > 0 ) {
			heldObject = getObject( inObj->holdingID );
		}
		else if( inObj->holdingID < 0 ) {
			// held baby
			LiveObject *babyO = getGameObject( - inObj->holdingID );

			if( babyO != NULL ) {
				heldObject = getObject( babyO->displayID );
			}
		}
	}


	getArmHoldingParameters( heldObject, &hideClosestArm, &hideAllLimbs );


	// override animation types for people who are riding in something
	// animationBank will freeze the time on their arms whenever they are
	// in a moving animation.
	AnimType frozenArmType = endAnimType;
	AnimType frozenArmFadeTargetType = endAnimType;

	if( hideClosestArm == -2
		||
		( inObj->holdingID > 0 && getObject( inObj->holdingID )->rideable )
		||
		( inObj->lastHoldingID > 0 &&
		  getObject( inObj->lastHoldingID )->rideable ) ) {

		if( curType == ground2 || curType == moving ) {
			frozenArmType = moving;
		}
		if( fadeTargetType == ground2 || fadeTargetType == moving ) {
			frozenArmFadeTargetType = moving;
		}
	}

	char alreadyDrawnPerson = false;

	HoldingPos holdingPos;
	holdingPos.valid = false;


	if( inObj->holdingID > 0 &&
		heldObject->rideable ) {
		// don't draw now,
		// wait until we know rideable's offset
	}
	else {
		alreadyDrawnPerson = true;
		doublePair personPos = pos;

		// decay away from riding offset, if any
		if( inObj->ridingOffset.x != 0 ||
			inObj->ridingOffset.y != 0 ) {

			doublePair nullOffset = { 0, 0 };


			doublePair delta = sub( nullOffset, inObj->ridingOffset );

			double step = frameRateFactor * 8;

			if( length( delta ) < step ) {

				inObj->ridingOffset.x = 0;
				inObj->ridingOffset.y = 0;
			}
			else {
				inObj->ridingOffset =
						add( inObj->ridingOffset,
								mult( normalize( delta ), step ) );
			}

			// step offset BEFORE applying it
			// (so we don't repeat starting position)
			personPos = add( personPos, inObj->ridingOffset );
		}


		setAnimationEmotion( inObj->currentEmot );
		addExtraAnimationEmotions( &( inObj->permanentEmots ) );

		holdingPos =
				drawObjectAnim( inObj->displayID, 2, curType,
						timeVal,
						animFade,
						fadeTargetType,
						targetTimeVal,
						frozenRotTimeVal,
						&( inObj->frozenRotFrameCountUsed ),
						frozenArmType,
						frozenArmFadeTargetType,
						personPos,
						0,
						false,
						inObj->holdingFlip,
						age,
						// don't actually hide body parts until
						// held object is done sliding into place
						hideClosestArm,
						hideAllLimbs,
						inObj->heldPosOverride &&
						! inObj->heldPosOverrideAlmostOver,
						inObj->clothing,
						inObj->clothingContained );

		setAnimationEmotion( NULL );
	}


	if( inObj->holdingID != 0 ) {
		doublePair holdPos;

		double holdRot = 0;

		computeHeldDrawPos( holdingPos, pos,
				heldObject,
				inObj->holdingFlip,
				&holdPos, &holdRot );


		doublePair heldObjectDrawPos = holdPos;

		if( heldObject != NULL && heldObject->rideable ) {
			heldObjectDrawPos = pos;
		}


		heldObjectDrawPos = mult( heldObjectDrawPos, 1.0 / CELL_D );

		if( inObj->currentSpeed == 0 &&
			inObj->heldPosOverride &&
			! inObj->heldPosOverrideAlmostOver &&
			! equal( heldObjectDrawPos, inObj->heldObjectPos ) ) {

			doublePair delta = sub( heldObjectDrawPos, inObj->heldObjectPos );
			double rotDelta = holdRot - inObj->heldObjectRot;

			if( rotDelta > 0.5 ) {
				rotDelta = rotDelta - 1;
			}
			else if( rotDelta < -0.5 ) {
				rotDelta = 1 + rotDelta;
			}

			// as slide gets longer, we speed up
			double longSlideModifier = 1;

			double slideTime = inObj->heldPosSlideStepCount * frameRateFactor;

			if( slideTime > 30 ) {
				// more than a half second
				longSlideModifier = pow( slideTime / 30, 2 );
			}

			double step = frameRateFactor * 0.0625 * longSlideModifier;
			double rotStep = frameRateFactor * 0.03125;

			if( length( delta ) < step ) {
				inObj->heldObjectPos = heldObjectDrawPos;
				inObj->heldPosOverrideAlmostOver = true;
			}
			else {
				inObj->heldObjectPos =
						add( inObj->heldObjectPos,
								mult( normalize( delta ),
										step ) );

				heldObjectDrawPos = inObj->heldObjectPos;
			}

			if( fabs( rotDelta ) < rotStep ) {
				inObj->heldObjectRot = holdRot;
			}
			else {

				double rotDir = 1;
				if( rotDelta < 0 ) {
					rotDir = -1;
				}

				inObj->heldObjectRot =
						inObj->heldObjectRot + rotDir * rotStep;

				holdRot = inObj->heldObjectRot;
			}

			inObj->heldPosSlideStepCount ++;
		}
		else {
			inObj->heldPosOverride = false;
			inObj->heldPosOverrideAlmostOver = false;
			// track it every frame so we have a good
			// base point for smooth move when the object
			// is dropped
			inObj->heldObjectPos = heldObjectDrawPos;
			inObj->heldObjectRot = holdRot;
		}

		doublePair worldHoldPos = heldObjectDrawPos;

		heldObjectDrawPos = mult( heldObjectDrawPos, CELL_D );

		if( heldObject == NULL ||
			! heldObject->rideable ) {

			holdPos = heldObjectDrawPos;
		}

		setDrawColor( 1, 1, 1, 1 );


		AnimType curHeldType = inObj->curHeldAnim;
		AnimType fadeTargetHeldType = inObj->curHeldAnim;

		double heldAnimFade = 1.0;

		double heldTimeVal = frameRateFactor *
							 inObj->heldAnimationFrameCount / 60.0;

		double targetHeldTimeVal = heldTimeVal;

		double frozenRotHeldTimeVal = frameRateFactor *
									  inObj->heldFrozenRotFrameCount / 60.0;


		char heldFlip = inObj->holdingFlip;

		if( heldObject != NULL &&
			heldObject->noFlip ) {
			heldFlip = false;
		}


		if( !alreadyDrawnPerson ) {
			doublePair personPos = pos;

			doublePair targetRidingOffset = sub( personPos, holdPos );


			ObjectRecord *personObj = getObject( inObj->displayID );

			targetRidingOffset =
					sub( targetRidingOffset,
							getAgeBodyOffset(
									age,
									personObj->spritePos[
											getBodyIndex( personObj, age ) ] ) );

			// step toward target to smooth
			doublePair delta = sub( targetRidingOffset,
					inObj->ridingOffset );

			double step = frameRateFactor * 8;

			if( length( delta ) < step ) {
				inObj->ridingOffset = targetRidingOffset;
			}
			else {
				inObj->ridingOffset =
						add( inObj->ridingOffset,
								mult( normalize( delta ), step ) );
			}

			personPos = add( personPos, inObj->ridingOffset );

			setAnimationEmotion( inObj->currentEmot );
			addExtraAnimationEmotions( &( inObj->permanentEmots ) );

			if( heldObject->anySpritesBehindPlayer ) {
				// draw part that is behind player
				prepareToSkipSprites( heldObject, true );

				if( inObj->numContained == 0 ) {
					drawObjectAnim(
							inObj->holdingID, curHeldType,
							heldTimeVal,
							heldAnimFade,
							fadeTargetHeldType,
							targetHeldTimeVal,
							frozenRotHeldTimeVal,
							&( inObj->heldFrozenRotFrameCountUsed ),
							endAnimType,
							endAnimType,
							heldObjectDrawPos,
							holdRot,
							false,
							heldFlip, -1, false, false, false,
							getEmptyClothingSet(), NULL,
							0, NULL, NULL );
				}
				else {
					drawObjectAnim(
							inObj->holdingID, curHeldType,
							heldTimeVal,
							heldAnimFade,
							fadeTargetHeldType,
							targetHeldTimeVal,
							frozenRotHeldTimeVal,
							&( inObj->heldFrozenRotFrameCountUsed ),
							endAnimType,
							endAnimType,
							heldObjectDrawPos,
							holdRot,
							false,
							heldFlip,
							-1, false, false, false,
							getEmptyClothingSet(),
							NULL,
							inObj->numContained,
							inObj->containedIDs,
							inObj->subContainedIDs );
				}

				restoreSkipDrawing( heldObject );
			}


			// rideable object
			holdingPos =
					drawObjectAnim( inObj->displayID, 2, curType,
							timeVal,
							animFade,
							fadeTargetType,
							targetTimeVal,
							frozenRotTimeVal,
							&( inObj->frozenRotFrameCountUsed ),
							frozenArmType,
							frozenArmFadeTargetType,
							personPos,
							0,
							false,
							inObj->holdingFlip,
							age,
							// don't actually hide body parts until
							// held object is done sliding into place
							hideClosestArm,
							hideAllLimbs,
							inObj->heldPosOverride &&
							! inObj->heldPosOverrideAlmostOver,
							inObj->clothing,
							inObj->clothingContained );

			setAnimationEmotion( NULL );
		}



		// animate baby with held anim just like any other held object
		if( inObj->lastHeldAnimFade > 0 ) {
			curHeldType = inObj->lastHeldAnim;
			fadeTargetHeldType = inObj->curHeldAnim;
			heldAnimFade = inObj->lastHeldAnimFade;

			heldTimeVal = frameRateFactor *
						  inObj->lastHeldAnimationFrameCount / 60.0;
		}


		if( inObj->holdingID < 0 ) {
			// draw baby here
			int babyID = - inObj->holdingID;

			LiveObject *babyO = getGameObject( babyID );

			if( babyO != NULL ) {

				// save flip so that it sticks when baby set down
				babyO->holdingFlip = inObj->holdingFlip;

				// save world hold pos for smooth set-down of baby
				babyO->lastHeldByRawPosSet = true;
				babyO->lastHeldByRawPos = worldHoldPos;

				int hideClosestArmBaby = 0;
				char hideAllLimbsBaby = false;

				if( babyO->holdingID > 0 ) {
					ObjectRecord *babyHoldingObj =
							getObject( babyO->holdingID );

					getArmHoldingParameters( babyHoldingObj,
							&hideClosestArmBaby,
							&hideAllLimbsBaby );
				}


				setAnimationEmotion( babyO->currentEmot );
				addExtraAnimationEmotions( &( babyO->permanentEmots ) );

				doublePair babyHeldPos = holdPos;

				if( babyO->babyWiggle ) {

					babyO->babyWiggleProgress += 0.04 * frameRateFactor;

					if( babyO->babyWiggleProgress > 1 ) {
						babyO->babyWiggle = false;
					}
					else {

						// cosine from pi to 3 pi has smooth start and finish
						int wiggleDir = 1;
						if( heldFlip ) {
							wiggleDir = -1;
						}
						babyHeldPos.x += wiggleDir * 8 *
										 ( cos( babyO->babyWiggleProgress * 2 * M_PI +
												M_PI ) * 0.5 + 0.5 );
					}
				}

				returnPack =
						drawObjectAnimPacked(
								babyO->displayID, curHeldType,
								heldTimeVal,
								heldAnimFade,
								fadeTargetHeldType,
								targetHeldTimeVal,
								frozenRotHeldTimeVal,
								&( inObj->heldFrozenRotFrameCountUsed ),
								endAnimType,
								endAnimType,
								babyHeldPos,
								// never apply held rot to baby
								0,
								false,
								heldFlip,
								computeCurrentAge( babyO ),
								hideClosestArmBaby,
								hideAllLimbsBaby,
								false,
								babyO->clothing,
								babyO->clothingContained,
								0, NULL, NULL );

				setAnimationEmotion( NULL );

				if( babyO->currentSpeech != NULL ) {

					inSpeakers->push_back( babyO );
					inSpeakersPos->push_back( holdPos );
				}
			}
		}
		else if( inObj->numContained == 0 ) {

			returnPack =
					drawObjectAnimPacked(
							inObj->holdingID, curHeldType,
							heldTimeVal,
							heldAnimFade,
							fadeTargetHeldType,
							targetHeldTimeVal,
							frozenRotHeldTimeVal,
							&( inObj->heldFrozenRotFrameCountUsed ),
							endAnimType,
							endAnimType,
							heldObjectDrawPos,
							holdRot,
							false,
							heldFlip, -1, false, false, false,
							getEmptyClothingSet(), NULL,
							0, NULL, NULL );
		}
		else {
			returnPack =
					drawObjectAnimPacked(
							inObj->holdingID, curHeldType,
							heldTimeVal,
							heldAnimFade,
							fadeTargetHeldType,
							targetHeldTimeVal,
							frozenRotHeldTimeVal,
							&( inObj->heldFrozenRotFrameCountUsed ),
							endAnimType,
							endAnimType,
							heldObjectDrawPos,
							holdRot,
							false,
							heldFlip,
							-1, false, false, false,
							getEmptyClothingSet(),
							NULL,
							inObj->numContained,
							inObj->containedIDs,
							inObj->subContainedIDs );
		}
	}

	if( inObj->currentSpeech != NULL ) {

		inSpeakers->push_back( inObj );
		inSpeakersPos->push_back( pos );
	}

	if( inObj->id == ourID ) {
		setClothingHighlightFades( NULL );
	}

	return returnPack;
}

