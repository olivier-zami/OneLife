//
// Created by olivier on 30/11/2021.
//

/**********************************************************************************************************************
void player_update()
{
/**********************************************************************************************************************/
	int numLines;
	char **lines = split( message, "\n", &numLines );

	if( numLines > 0 ) {
// skip fist
		delete [] lines[0];
	}

// for babies that are held, but don't exist yet in
// client because PU creating them hasn't been received yet
// assume these always arrive in the same PU message
	SimpleVector<int> unusedHolderID;
	SimpleVector<int> unusedHeldID;

	for( int i=1; i<numLines; i++ ) {

		LiveObject o;

		o.onScreen = false;
		o.allSpritesLoaded = false;
		o.holdingID = 0;
		o.useWaypoint = false;
		o.pathToDest = NULL;
		o.containedIDs = NULL;
		o.subContainedIDs = NULL;
		o.onFinalPathStep = false;
		o.age = 0;
		o.finalAgeSet = false;
		o.outOfRange = false;
		o.dying = false;
		o.sick = false;
		o.lineageEveID = -1;
		o.name = NULL;
		o.relationName = NULL;
		o.curseLevel = 0;
		o.excessCursePoints = 0;
		o.curseTokenCount = 0;
		o.tempAgeOverrideSet = false;
		o.tempAgeOverride = 0;
		o.foodStore = 0;// don't track these for other players
		o.foodCapacity = 0;
		o.maxFoodStore = 0;
		o.maxFoodCapacity = 0;
		o.currentSpeech = NULL;
		o.speechFade = 1.0;
		o.speechIsSuccessfulCurse = false;
		o.heldByAdultID = -1;
		o.heldByAdultPendingID = -1;
		o.heldByDropOffset.x = 0;
		o.heldByDropOffset.y = 0;
		o.jumpOutOfArmsSentTime = 0;
		o.babyWiggle = false;
		o.ridingOffset.x = 0;
		o.ridingOffset.y = 0;
		o.animationFrameCount = 0;
		o.heldAnimationFrameCount = 0;
		o.lastAnimationFrameCount = 0;
		o.lastHeldAnimationFrameCount = 0;
		o.frozenRotFrameCount = 0;
		o.heldFrozenRotFrameCount = 0;
		o.frozenRotFrameCountUsed = false;
		o.heldFrozenRotFrameCountUsed = false;
		o.clothing = getEmptyClothingSet();
		o.currentMouseOverClothingIndex = -1;

		for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
			o.clothingHighlightFades[c] = 0;
		}


		o.somePendingMessageIsMoreMovement = false;


		o.actionTargetTweakX = 0;
		o.actionTargetTweakY = 0;

		o.currentEmot = NULL;
		o.emotClearETATime = 0;

		o.killMode = false;
		o.killWithID = -1;


		int forced = 0;
		int done_moving = 0;

		char *holdingIDBuffer = new char[500];

		int heldOriginValid, heldOriginX, heldOriginY,
				heldTransitionSourceID;

		char *clothingBuffer = new char[500];

		int justAte = 0;
		int justAteID = 0;

		int facingOverride = 0;
		int actionAttempt = 0;
		int actionTargetX = 0;
		int actionTargetY = 0;

		double invAgeRate = 60.0;

		int responsiblePlayerID = -1;

		int heldYum = 0;

		int numRead = sscanf( lines[i],
				"%d %d "
				"%d "
				"%d "
				"%d %d "
				"%499s %d %d %d %d %f %d %d %d %d "
				"%lf %lf %lf %499s %d %d %d "
				"%d",
				&( o.id ),
				&( o.displayID ),
				&facingOverride,
				&actionAttempt,
				&actionTargetX,
				&actionTargetY,
				holdingIDBuffer,
				&heldOriginValid,
				&heldOriginX,
				&heldOriginY,
				&heldTransitionSourceID,
				&( o.heat ),
				&done_moving,
				&forced,
				&( o.xd ),
				&( o.yd ),
				&( o.age ),
				&invAgeRate,
				&( o.lastSpeed ),
				clothingBuffer,
				&justAte,
				&justAteID,
				&responsiblePlayerID,
				&heldYum);


// heldYum is 24th value, optional
		if( numRead >= 23 ) {

			applyReceiveOffset( &actionTargetX, &actionTargetY );
			applyReceiveOffset( &heldOriginX, &heldOriginY );
			applyReceiveOffset( &( o.xd ), &( o.yd ) );

			printf( "PLAYER_UPDATE with heldOrVal=%d, "
					"heldOrx=%d, heldOry=%d, "
					"pX=%d, pY=%d, heldTransSrcID=%d, "
					"holdingString=%s\n",
					heldOriginValid, heldOriginX, heldOriginY,
					o.xd, o.yd, heldTransitionSourceID,
					holdingIDBuffer );
			if( forced ) {
				printf( "  POSITION FORCED\n" );
			}

			o.lastAgeSetTime = game_getCurrentTime();

			o.ageRate = 1.0 / invAgeRate;

			int numClothes;
			char **clothes = split( clothingBuffer, ";", &numClothes );

			if( numClothes == NUM_CLOTHING_PIECES ) {

				for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {

					int numParts;
					char **parts = split( clothes[c], ",", &numParts );

					if( numParts > 0 ) {
						int id = 0;
						sscanf( parts[0], "%d", &id );

						if( id != 0 ) {
							setClothingByIndex( &( o.clothing ),
									c,
									getObject( id ) );

							if( numParts > 1 ) {
								for( int p=1; p<numParts; p++ ) {

									int cID = 0;
									sscanf( parts[p], "%d", &cID );

									if( cID != 0 ) {

										o.clothingContained[c].
												push_back( cID );
									}
								}
							}
						}
					}
					for( int p=0; p<numParts; p++ ) {
						delete [] parts[p];
					}
					delete [] parts;
				}

			}

			for( int c=0; c<numClothes; c++ ) {
				delete [] clothes[c];
			}
			delete [] clothes;


			if( strstr( holdingIDBuffer, "," ) != NULL ) {
				int numInts;
				char **ints = split( holdingIDBuffer, ",", &numInts );

				o.holdingID = atoi( ints[0] );

				delete [] ints[0];

				o.numContained = numInts - 1;
				o.containedIDs = new int[ o.numContained ];
				o.subContainedIDs =
						new SimpleVector<int>[ o.numContained ];

				for( int c=0; c<o.numContained; c++ ) {
					o.containedIDs[c] = atoi( ints[ c + 1 ] );

					if( strstr( ints[c + 1], ":" ) != NULL ) {
// sub-container items

						int numSubInts;
						char **subInts =
								split( ints[c + 1], ":", &numSubInts );

						delete [] subInts[0];
						int numSubCont = numSubInts - 1;
						for( int s=0; s<numSubCont; s++ ) {
							o.subContainedIDs[c].push_back(
									atoi( subInts[ s + 1 ] ) );
							delete [] subInts[ s + 1 ];
						}

						delete [] subInts;
					}

					delete [] ints[ c + 1 ];
				}
				delete [] ints;
			}
			else {
// a single int
				o.holdingID = atoi( holdingIDBuffer );
				o.numContained = 0;
			}


			o.xServer = o.xd;
			o.yServer = o.yd;

			LiveObject *existing = NULL;

			for( int j=0; j<gameObjects.size(); j++ ) {
				if( gameObjects.getElement(j)->id == o.id ) {
					existing = gameObjects.getElement(j);
					break;
				}
			}

			if( existing != NULL &&
				existing->id == ourID ) {
// got a PU for self

				mYumSlipPosTargetOffset[2] = mYumSlipHideOffset[2];
				mYumSlipPosTargetOffset[3] = mYumSlipHideOffset[3];

				int slipIndexToShow = -1;
				if( heldYum ) {
// YUM
					slipIndexToShow = 2;
				}
				else {
					if( o.holdingID > 0 &&
						getObject( o.holdingID )->foodValue > 0 ) {
// MEH
						slipIndexToShow = 3;
					}
				}

				if( slipIndexToShow >= 0 ) {
					mYumSlipPosTargetOffset[slipIndexToShow].y += 36;
				}

				if( existing->lastActionSendStartTime != 0 ) {

// PU for an action that we sent
// measure round-trip time
					existing->lastResponseTimeDelta =
							game_getCurrentTime() -
							existing->lastActionSendStartTime;

					existing->lastActionSendStartTime = 0;
				}
			}


			if( existing != NULL &&
				existing->destTruncated &&
				( existing->xd != o.xd ||
				  existing->yd != o.yd ) ) {
// edge case:
// our move was truncated due to an obstacle
// but then after that we tried to move to what
// the server saw as our current location
// the server will send a PU for that last move
// to end it immediately, but our xd,yd will still
// be set from the truncated move
// treat this PU as a force
				printf( "Artificially forcing PU position %d,%d "
						"because it mismatches our last truncated "
						"move destination %d,%d\n",
						o.xd, o.yd, existing->xd, existing->yd );
				forced = true;
			}


			char alreadyHeldAsPending = false;

			if( existing != NULL ) {

				if( o.holdingID < 0 ) {
// this held PU talks about held baby

					int heldBabyID = - o.holdingID;

					LiveObject *heldBaby = getLiveObject( heldBabyID );

					if( heldBaby != NULL ) {

// clear up held status on baby if old
// holding adult is out of range
// (we're not getting updates about them
//  anymore).

						if( heldBaby->heldByAdultID > 0 &&
							heldBaby->heldByAdultID != existing->id ) {

							LiveObject *oldHolder =
									getLiveObject(
											heldBaby->heldByAdultID );

							if( oldHolder != NULL &&
								oldHolder->outOfRange ) {
								heldBaby->heldByAdultID = -1;
							}
						}

						if( heldBaby->heldByAdultPendingID > 0 &&
							heldBaby->heldByAdultPendingID !=
							existing->id ) {

							LiveObject *oldPendingHolder =
									getLiveObject(
											heldBaby->heldByAdultPendingID );

							if( oldPendingHolder != NULL &&
								oldPendingHolder->outOfRange ) {
								heldBaby->heldByAdultPendingID = -1;
							}
						}


// now handle changes to held status
						if( heldBaby->heldByAdultID == existing->id ) {
// baby already knows it's held
						}
						else if( heldBaby->heldByAdultID != -1 ) {
// baby thinks it's held by another adult

// stick this pending message on that
// adult's queue instead
							LiveObject *holdingAdult =
									getLiveObject(
											heldBaby->heldByAdultID );

							if( holdingAdult != NULL ) {

								char *pendingMessage =
										autoSprintf( "PU\n%s\n#",
												lines[i] );

								holdingAdult->pendingReceivedMessages.
										push_back( pendingMessage );

								alreadyHeldAsPending = true;
							}
						}
						else if( heldBaby->heldByAdultPendingID ==
								 -1 ){
// mark held pending
							heldBaby->heldByAdultPendingID =
									existing->id;
						}
						else if( heldBaby->heldByAdultPendingID !=
								 existing->id ) {
// already pending held by some other adult

// stick this pending message on that
// adult's queue instead
							LiveObject *holdingAdult =
									getLiveObject(
											heldBaby->heldByAdultPendingID );

							if( holdingAdult != NULL ) {
								char *pendingMessage =
										autoSprintf( "PU\n%s\n#",
												lines[i] );
								holdingAdult->pendingReceivedMessages.
										push_back( pendingMessage );

								alreadyHeldAsPending = true;
							}
						}
					}
				}
			}

			if( alreadyHeldAsPending ) {
			}
			else if( existing != NULL &&
					 existing->id != ourID &&
					 existing->currentSpeed != 0 &&
					 ! forced ) {

// non-forced update about other player
// while we're still playing their last movement

// sometimes this can mean a move truncation
				if( existing->pathToDest != NULL &&
					existing->pathLength > 0 ) {

					GridPos pathEnd =
							existing->pathToDest[
									existing->pathLength - 1 ];

					if( o.xd == pathEnd.x &&
						o.yd == pathEnd.y ) {
// PU destination matches our current path dest
// no move truncation
					}
					else if( done_moving > 0 ) {
// PU should be somewhere along our path
// a truncated move

// truncate our path there
						for( int p=existing->pathLength - 1;
							 p >= 0; p-- ) {

							GridPos thisStep =
									existing->pathToDest[ p ];

							if( thisStep.x == o.xd &&
								thisStep.y == o.yd ) {
// found

// cut off
								existing->pathLength = p + 1;

// navigate there
								if( p > 0 ) {
									existing->currentPathStep = p - 1;
								}
								else {
									existing->currentPathStep = p;
								}

// set new truncated dest
								existing->xd = o.xd;
								existing->yd = o.yd;
								break;
							}
						}

						if( existing->pathLength == 1 ) {
							fixSingleStepPath( existing );
						}
					}
				}

				if( done_moving > 0  ||
					existing->pendingReceivedMessages.size() > 0 ) {

// this PU happens after they are done moving
// or it happens mid-move, but we already
// have messages held, so it may be meant
// to happen in the middle of their next move

// defer it until they're done moving
					printf( "Holding PU message for "
							"%d until later, "
							"%d other messages pending for them\n",
							existing->id,
							existing->pendingReceivedMessages.size() );

					existing->pendingReceivedMessages.push_back(
							autoSprintf( "PU\n%s\n#",
									lines[i] ) );
				}
			}
			else if( existing != NULL &&
					 existing->heldByAdultID != -1 &&
					 getLiveObject( existing->heldByAdultID ) != NULL &&
					 getLiveObject( existing->heldByAdultID )->
							 pendingReceivedMessages.size() > 0 ) {
// we're held by an adult who has pending
// messages to play at the end of their movement
// (server sees that their movement has already ended)
// Thus, an update about us should be played later also
// whenever the adult's messages are played back

				LiveObject *holdingPlayer =
						getLiveObject( existing->heldByAdultID );

				printf( "Holding PU message for baby %d held by %d "
						"until later, "
						"%d other messages pending for them\n",
						existing->id,
						existing->heldByAdultID,
						holdingPlayer->pendingReceivedMessages.size() );

				holdingPlayer->
						pendingReceivedMessages.push_back(
						autoSprintf( "PU\n%s\n#",
								lines[i] ) );
			}
			else if( existing != NULL &&
					 existing->heldByAdultPendingID != -1 &&
					 getLiveObject( existing->heldByAdultPendingID )
					 != NULL &&
					 getLiveObject( existing->heldByAdultPendingID )->
							 pendingReceivedMessages.size() > 0 ) {
// we're pending to be held by an adult who has pending
// messages to play at the end of their movement
// (server sees that their movement has already ended)
// Thus, an update about us should be played later also
// whenever the adult's messages are played back

				LiveObject *holdingPlayer =
						getLiveObject( existing->heldByAdultPendingID );

				printf( "Holding PU message for baby %d with "
						"pending held by %d "
						"until later, "
						"%d other messages pending for them\n",
						existing->id,
						existing->heldByAdultPendingID,
						holdingPlayer->pendingReceivedMessages.size() );

				holdingPlayer->
						pendingReceivedMessages.push_back(
						autoSprintf( "PU\n%s\n#",
								lines[i] ) );
			}
			else if( existing != NULL &&
					 responsiblePlayerID != -1 &&
					 getLiveObject( responsiblePlayerID ) != NULL &&
					 getLiveObject( responsiblePlayerID )->
							 pendingReceivedMessages.size() > 0 ) {
// someone else is responsible for this change
// to us (we're likely a baby) and that person
// is still in the middle of a local walk
// with pending messages that will play
// after the walk.  Defer this message too

				LiveObject *rO =
						getLiveObject( responsiblePlayerID );

				printf( "Holding PU message for %d caused by %d "
						"until later, "
						"%d other messages pending for them\n",
						existing->id,
						responsiblePlayerID,
						rO->pendingReceivedMessages.size() );

				rO->pendingReceivedMessages.push_back(
						autoSprintf( "PU\n%s\n#",
								lines[i] ) );
			}
			else if( existing != NULL ) {
				int oldHeld = existing->holdingID;
				int heldContChanged = false;

				if( oldHeld > 0 &&
					oldHeld == existing->holdingID ) {

					heldContChanged =
							checkIfHeldContChanged( &o, existing );
				}


// receiving a PU means they aren't out of
// range anymore
				if( existing->outOfRange ) {
// was out of range before
// this update is forced
					existing->currentPos.x = o.xd;
					existing->currentPos.y = o.yd;

					existing->currentSpeed = 0;
					existing->currentGridSpeed = 0;

					existing->xd = o.xd;
					existing->yd = o.yd;
					existing->destTruncated = false;

// clear an existing path, since they may no
// longer be on it
					if( existing->pathToDest != NULL ) {
						delete [] existing->pathToDest;
						existing->pathToDest = NULL;
					}

				}
				existing->outOfRange = false;


				existing->lastHoldingID = oldHeld;
				existing->holdingID = o.holdingID;

				if( o.id == ourID &&
					existing->holdingID > 0 &&
					existing->holdingID != oldHeld ) {
// holding something new
// hint about it

					mNextHintObjectID = existing->holdingID;
					mNextHintIndex =
							mHintBookmarks[ mNextHintObjectID ];

					if (minitech::changeHintObjOnTouch) minitech::currentHintObjId = mNextHintObjectID;
				}


				ObjectRecord *newClothing =
						getClothingAdded( &( existing->clothing ),
								&( o.clothing ) );


				if( newClothing == NULL ) {
// has something been removed instead?
					newClothing =
							getClothingAdded( &( o.clothing ),
									&( existing->clothing ) );
				}

				char clothingSoundPlayed = false;

				char clothingChanged = false;

				if( newClothing != NULL ) {
					clothingChanged = true;

					SoundUsage clothingSound =
							newClothing->usingSound;

					if( clothingSound.numSubSounds > 0 ) {
						playSound( clothingSound,
								getVectorFromCamera(
										o.xd,
										o.yd ) );

						clothingSoundPlayed = true;
					}
					else {
// play generic pickup sound
						ObjectRecord *existingObj =
								getObject( existing->displayID );

						if( existingObj->usingSound.numSubSounds > 0 ) {

							playSound( existingObj->usingSound,
									getVectorFromCamera(
											o.xd,
											o.yd ) );
							clothingSoundPlayed = true;
						}
					}
				}

				existing->clothing = o.clothing;


// what we're holding hasn't changed
// maybe action failed
				if( o.id == ourID && existing->holdingID == oldHeld ) {

					LiveObject *ourObj = getOurLiveObject();

					if( ourObj->pendingActionAnimationProgress != 0 &&
						! ourObj->inMotion ) {

						addNewAnimPlayerOnly( existing, ground2 );
					}
				}

				if( o.id != ourID ) {

					if( actionAttempt && ! justAte &&
						nearEndOfMovement( existing ) ) {
						existing->actionTargetX = actionTargetX;
						existing->actionTargetY = actionTargetY;
						existing->pendingActionAnimationProgress =
								0.025 * frameRateFactor;
					}

					if( heldOriginValid ||
						facingOverride != 0 ) {

						if( ( heldOriginValid &&
							  heldOriginX > existing->xd )
							||
							facingOverride == 1 ) {

							existing->holdingFlip = false;
						}
						else if( ( heldOriginValid &&
								   heldOriginX < existing->xd )
								 ||
								 facingOverride == -1 ) {

							existing->holdingFlip = true;
						}
					}
				}
				else if( o.id == ourID &&
						 actionAttempt &&
						 ! justAte &&
						 existing->killMode &&
						 // they were still holding the same weapon
						 // before this update
						 existing->killWithID == oldHeld &&
						 // their weapon changed as a result of this
						 // update
						 existing->holdingID != oldHeld ) {

// show kill "doing" animation and bounce

					playerActionTargetX = actionTargetX;
					playerActionTargetY = actionTargetY;

					addNewAnimPlayerOnly( existing, doing );

					if( existing->pendingActionAnimationProgress
						== 0 ) {
						existing->pendingActionAnimationProgress =
								0.025 * frameRateFactor;
					}

					if( facingOverride == 1 ) {
						existing->holdingFlip = false;
					}
					else if( facingOverride == -1 ) {
						existing->holdingFlip = true;
					}
				}


				char creationSoundPlayed = false;
				char otherSoundPlayed = false;
				char groundSoundPlayed = false;


				if( justAte &&
					o.id != ourID &&
					existing->holdingID == oldHeld ) {
// seems like this PU is about player being
// fed by someone else

// don't interrupt walking
// but still play sound
					if( nearEndOfMovement( existing ) ) {
						addNewAnimPlayerOnly(
								existing, eating );
					}

					ObjectRecord *ateObj =
							getObject( justAteID );

					if( ateObj->eatingSound.numSubSounds > 0 ) {
						playSound(
								ateObj->eatingSound,
								getVectorFromCamera(
										existing->currentPos.x,
										existing->currentPos.y ) );
						otherSoundPlayed = true;
					}
				}


				if( justAte && o.id == ourID ) {
// we just heard from server that we
// finished eating
// play sound now
					ObjectRecord *ateObj =
							getObject( justAteID );
					if( ateObj->eatingSound.numSubSounds > 0 ) {
						playSound(
								ateObj->eatingSound,
								getVectorFromCamera(
										existing->currentPos.x,
										existing->currentPos.y ) );
						otherSoundPlayed = true;
					}
					if( strstr( ateObj->description, "remapStart" )
						!= NULL ) {

						if( mRemapPeak == 0 ) {
// reseed
							setRemapSeed(
									remapRandSource.getRandomBoundedInt(
											0,
											10000000 ) );
							mRemapDelay = 0;
						}

// closer to max peak
// zeno's paradox style
						mRemapPeak += 0.5 * ( 1 - mRemapPeak );

// back toward peak
						mRemapDirection = 1.0;
					}
				}


				if( existing->holdingID == 0 ) {

// don't reset these when dropping something
// leave them in place so that dropped object
// can use them for smooth fade
// existing->animationFrameCount = 0;
// existing->curAnim = ground;

//existing->lastAnim = ground;
//existing->lastAnimFade = 0;
					if( oldHeld != 0 ) {
						if( o.id == ourID ) {
							addNewAnimPlayerOnly( existing, ground );
						}
						else {
							if( justAte ) {
// don't interrupt walking
// but still play sound
								if( nearEndOfMovement( existing ) ) {
									addNewAnimPlayerOnly(
											existing, eating );
								}

								if( oldHeld > 0 ) {
									ObjectRecord *oldHeldObj =
											getObject( oldHeld );

									if( oldHeldObj->
											eatingSound.numSubSounds > 0 ) {

										playSound(
												oldHeldObj->eatingSound,
												getVectorFromCamera(
														existing->currentPos.x,
														existing->currentPos.y ) );
										otherSoundPlayed = true;
									}
								}
							}
							else {
// don't interrupt walking
								if( nearEndOfMovement( existing ) ) {
									addNewAnimPlayerOnly(
											existing, doing );
								}
							}

// don't interrupt walking
							if( nearEndOfMovement( existing ) ) {
								addNewAnimPlayerOnly( existing,
										ground );
							}
						}
					}
				}
				else if( oldHeld != existing->holdingID ||
						 heldContChanged ) {
// holding something new

// what we're holding has gone through
// transition.  Keep old animation going
// for what's held

					if( o.id == ourID ) {
						addNewAnimPlayerOnly( existing, ground2 );
					}
					else {
						if( justAte ) {
// don't interrupt walking
// but still play sound
							if( nearEndOfMovement( existing ) ) {
								addNewAnimPlayerOnly(
										existing, eating );
							}
							if( oldHeld > 0 ) {

								ObjectRecord *oldHeldObj =
										getObject( oldHeld );

								if( oldHeldObj->eatingSound.numSubSounds
									> 0 ) {

									playSound(
											oldHeldObj->eatingSound,
											getVectorFromCamera(
													existing->currentPos.x,
													existing->currentPos.y ) );
									otherSoundPlayed = true;
								}
							}
						}
						else {
// don't interrupt walking
							if( actionAttempt &&
								nearEndOfMovement( existing ) ) {
								addNewAnimPlayerOnly(
										existing, doing );
							}
						}
// don't interrupt walking
						if( nearEndOfMovement( existing ) ) {
							addNewAnimPlayerOnly( existing, ground2 );
						}
					}

					if( heldOriginValid ) {

// use player's using sound for pickup
						ObjectRecord *existingObj =
								getObject( existing->displayID );

						if( existingObj->usingSound.numSubSounds > 0 ) {

							playSound( existingObj->usingSound,
									getVectorFromCamera(
											heldOriginX,
											heldOriginY ) );
							otherSoundPlayed = true;
						}


// transition from last ground animation
// of object, keeping that frame count
// for smooth transition

						if( ! existing->onScreen ) {
// off-screen, instant pickup,
// no transition animation
							existing->heldPosOverride = false;
							existing->heldObjectPos =
									existing->currentPos;
							existing->heldObjectRot = 0;

							existing->lastHeldAnimFade = 0;
							existing->curHeldAnim = held;
						}
						else {
// on-screen, slide into position
// smooth animation transition
							existing->heldPosSlideStepCount = 0;
							existing->heldPosOverride = true;
							existing->heldPosOverrideAlmostOver = false;
							existing->heldObjectPos.x = heldOriginX;
							existing->heldObjectPos.y = heldOriginY;


// check if held origin needs
// tweaking because what we picked up
// had a moving offset
							int mapHeldOriginX =
									heldOriginX - mMapOffsetX + mMapD / 2;
							int mapHeldOriginY =
									heldOriginY- mMapOffsetY + mMapD / 2;

							if( mapHeldOriginX >= 0 &&
								mapHeldOriginX < mMapD
								&&
								mapHeldOriginY >= 0 &&
								mapHeldOriginY < mMapD ) {

								int mapHeldOriginI =
										mapHeldOriginY * mMapD +
										mapHeldOriginX;

								if( mMapMoveSpeeds[ mapHeldOriginI ]
									> 0 &&
									( mMapMoveOffsets[
											  mapHeldOriginI ].x
									  != 0
									  ||
									  mMapMoveOffsets[
											  mapHeldOriginI ].y
									  != 0 ) ) {

// only do this if tweak not already
// set.  Don't want to interrupt
// an existing tweak
									if( existing->actionTargetTweakX ==
										0 &&
										existing->actionTargetTweakY ==
										0 ) {

										existing->actionTargetTweakX =
												lrint(
														mMapMoveOffsets[
																mapHeldOriginI ].x );

										existing->actionTargetTweakY =
												lrint( mMapMoveOffsets[
														mapHeldOriginI ].y );
									}

									existing->heldObjectPos =
											add( existing->heldObjectPos,
													mMapMoveOffsets[
															mapHeldOriginI ] );
								}
							}


							existing->heldObjectRot = 0;


							int mapX =
									heldOriginX - mMapOffsetX + mMapD / 2;
							int mapY =
									heldOriginY - mMapOffsetY + mMapD / 2;

							if( mapX >= 0 && mapX < mMapD
								&&
								mapY >= 0 && mapY < mMapD ) {

								int mapI = mapY * mMapD + mapX;

								existing->heldFrozenRotFrameCount =
										mMapAnimationFrozenRotFrameCount
										[ mapI ];
								existing->heldFrozenRotFrameCountUsed =
										false;

								if( mMapLastAnimFade[ mapI ] == 0 ) {
									existing->lastHeldAnim =
											mMapCurAnimType[ mapI ];
									existing->lastHeldAnimFade = 1;
									existing->curHeldAnim = held;

									existing->
											lastHeldAnimationFrameCount =
											mMapAnimationFrameCount[ mapI ];

									existing->heldAnimationFrameCount =
											mMapAnimationFrameCount[ mapI ];
								}
								else {
// map spot is in the middle of
// an animation fade
									existing->lastHeldAnim =
											mMapLastAnimType[ mapI ];
									existing->
											lastHeldAnimationFrameCount =
											mMapAnimationLastFrameCount[
													mapI ];

									existing->curHeldAnim =
											mMapCurAnimType[ mapI ];

									existing->heldAnimationFrameCount =
											mMapAnimationFrameCount[ mapI ];


									existing->lastHeldAnimFade =
											mMapLastAnimFade[ mapI ];

									existing->futureHeldAnimStack->
											push_back( held );
								}

							}
						}
					}
					else {
						existing->heldObjectPos = existing->currentPos;
						existing->heldObjectRot = 0;

						if( existing->holdingID > 0 ) {
// what player is holding changed
// but they didn't pick anything up
// play change sound

							ObjectRecord *heldObj =
									getObject( existing->holdingID );


							if( oldHeld > 0 &&
								heldTransitionSourceID == -1 ) {
// held object auto-decayed from
// some other object

// play decay sound
								ObjectRecord *obj =
										getObject( oldHeld );
								if( obj->decaySound.numSubSounds
									> 0 ) {

									playSound(
											obj->decaySound,
											getVectorFromCamera(
													existing->currentPos.x,
													existing->currentPos.y ) );
									otherSoundPlayed = true;
								}
							}
							else if( oldHeld > 0 &&
									 heldTransitionSourceID > 0 ) {

								TransRecord *t =
										getTrans( oldHeld,
												heldTransitionSourceID );

								if( t == NULL &&
									oldHeld ==
									heldTransitionSourceID ) {
// see if use-on-bare-ground
// transition exists
									t = getTrans( oldHeld, -1 );
								}

								if( t != NULL &&
									t->target != t->newTarget &&
									t->newTarget > 0 ) {

// something produced on
// ground by this transition

// does it make a sound on
// creation?

									if( getObject( t->newTarget )->
											creationSound.numSubSounds
										> 0 ) {

										int sourceID = t->target;
										if( sourceID == -1 ) {
// use on bare ground
											sourceID = oldHeld;
										}

										if( shouldCreationSoundPlay(
												sourceID,
												t->newTarget ) ) {

// only make one sound
											otherSoundPlayed = true;
										}
									}
								}
							}


							if( ( ! otherSoundPlayed ||
								  heldObj->creationSoundForce )
								&&
								! clothingChanged &&
								heldTransitionSourceID >= 0 &&
								heldObj->creationSound.numSubSounds
								> 0 ) {

								int testAncestor = oldHeld;

								if( oldHeld <= 0 &&
									heldTransitionSourceID > 0 ) {

									testAncestor =
											heldTransitionSourceID;
								}

								if( heldTransitionSourceID > 0 ) {
									TransRecord *groundTrans =
											getTrans(
													oldHeld,
													heldTransitionSourceID );
									if( groundTrans != NULL &&
										groundTrans->newTarget > 0 &&
										groundTrans->newActor ==
										existing->holdingID ) {
										if( shouldCreationSoundPlay(
												groundTrans->target,
												groundTrans->newTarget ) ) {
											groundSoundPlayed = true;
										}
									}
								}


								if( ( ! groundSoundPlayed ||
									  heldObj->creationSoundForce )
									&&
									testAncestor > 0 ) {
// new held object is result
// of a transtion
// (otherwise, it has been
//  removed from a container

// only play creation sound
// if this object is truly new
// (if object is flag for initial
//  creation sounds only)
// Check ancestor chains for
// objects that loop back to their
// initial state.


// also not useDummies that have
// the same parent
									char sameParent = false;

									if( testAncestor > 0 &&
										existing->holdingID > 0 ) {

										int holdID =
												existing->holdingID;

										ObjectRecord *ancObj =
												getObject( testAncestor );


										if( heldObj->isUseDummy &&
											ancObj->isUseDummy &&
											heldObj->useDummyParent ==
											ancObj->useDummyParent ) {
											sameParent = true;
										}
										else if( heldObj->numUses > 1
												 &&
												 ancObj->isUseDummy
												 &&
												 ancObj->useDummyParent
												 == holdID ) {
											sameParent = true;
										}
										else if( ancObj->numUses > 1
												 &&
												 heldObj->isUseDummy
												 &&
												 heldObj->useDummyParent
												 == testAncestor ) {
											sameParent = true;
										}
									}


									if( ! sameParent
										&&
										(! heldObj->
												creationSoundInitialOnly
										 ||
										 ( ! isSpriteSubset(
												 testAncestor,
												 existing->holdingID ) )
										) ) {

										playSound(
												heldObj->creationSound,
												getVectorFromCamera(
														existing->currentPos.x,
														existing->currentPos.y ) );
										creationSoundPlayed = true;

										if( strstr(
												heldObj->description,
												"offScreenSound" )
											!= NULL ) {

											addOffScreenSound(
													existing->currentPos.x *
													CELL_D,
													existing->currentPos.y *
													CELL_D,
													heldObj->description );
										}
									}
								}
							}

							char autoDecay = false;

							if( oldHeld > 0 &&
								existing->holdingID > 0 ) {
								TransRecord *dR =
										getTrans( -1, oldHeld );

								if( dR != NULL &&
									dR->newActor == 0 &&
									dR->newTarget ==
									existing->holdingID ) {

									autoDecay = true;
								}
							}


							if( oldHeld == 0 ||
								heldContChanged ||
								( heldTransitionSourceID == -1 &&
								  !autoDecay &&
								  ! creationSoundPlayed &&
								  ! clothingSoundPlayed ) ) {
// we're holding something new

								existing->lastHeldAnim = held;
								existing->
										lastHeldAnimationFrameCount = 0;
								existing->curHeldAnim = held;
								existing->heldAnimationFrameCount = 0;
								existing->lastHeldAnimFade = 0;


								if( ! otherSoundPlayed &&
									! creationSoundPlayed &&
									! groundSoundPlayed &&
									! clothingSoundPlayed ) {
// play generic pickup sound

									ObjectRecord *existingObj =
											getObject(
													existing->displayID );

// skip this if they are dying
// because they may have picked
// up a wound
									if( !existing->dying &&
										existingObj->
												usingSound.numSubSounds > 0 ) {

										playSound(
												existingObj->usingSound,
												getVectorFromCamera(
														existing->currentPos.x,
														existing->currentPos.y ) );
										otherSoundPlayed = true;
									}
								}
							}
						}
					}

// otherwise, don't touch frame count


					if( existing->holdingID < 0 ) {
// picked up a baby
						int babyID = - existing->holdingID;

// save ALL of these for later, after
// we've processed all PU lines,
// because a held baby may not exist
// locally yet.
						unusedHolderID.push_back( existing->id );
						unusedHeldID.push_back( babyID );
					}
				}



				if( existing->holdingID >= 0 &&
					oldHeld >= 0 &&
					oldHeld != existing->holdingID &&
					! heldOriginValid &&
					heldTransitionSourceID > 0 &&
					! creationSoundPlayed &&
					! clothingSoundPlayed &&
					! groundSoundPlayed &&
					! otherSoundPlayed ) {


// what we're holding changed
// but no sound has been played for it yet

// check for special case where target
// of transition didn't change
// or is a use dummy

					TransRecord *tr =
							getTrans( oldHeld,
									heldTransitionSourceID );

					if( tr != NULL &&
						tr->newActor == existing->holdingID &&
						tr->target == heldTransitionSourceID &&
						( tr->newTarget == tr->target
						  ||
						  ( getObject( tr->target ) != NULL
							&&
							( getObject( tr->target )->isUseDummy
							  ||
							  getObject( tr->target )->numUses > 1 )
						  )
						  ||
						  ( getObject( tr->newTarget ) != NULL
							&&
							( getObject( tr->newTarget )->isUseDummy
							  ||
							  getObject( tr->newTarget )->numUses > 1 )
						  )
						) ) {

// what about "using" sound
// of the target of our transition?

						char creationWillPlay = false;

						if( tr->newTarget > 0 && tr->target > 0 &&
							shouldCreationSoundPlay( tr->target,
									tr->newTarget ) ) {
							creationWillPlay = true;
						}


						if( !creationWillPlay ) {

							ObjectRecord *targetObject =
									getObject( heldTransitionSourceID );

							if( targetObject->usingSound.numSubSounds
								> 0 ) {

								playSound(
										targetObject->usingSound,
										getVectorFromCamera(
												existing->currentPos.x,
												existing->currentPos.y ) );
							}
						}
					}
				}




				existing->displayID = o.displayID;
				existing->age = o.age;
				existing->lastAgeSetTime = o.lastAgeSetTime;

				existing->heat = o.heat;

				if( existing->containedIDs != NULL ) {
					delete [] existing->containedIDs;
				}
				if( existing->subContainedIDs != NULL ) {
					delete [] existing->subContainedIDs;
				}

				existing->containedIDs = o.containedIDs;
				existing->numContained = o.numContained;
				existing->subContainedIDs = o.subContainedIDs;

				existing->xServer = o.xServer;
				existing->yServer = o.yServer;

				existing->lastSpeed = o.lastSpeed;

				char babyDropped = false;

				if( done_moving > 0 && existing->heldByAdultID != -1 ) {
					babyDropped = true;
				}

				if( babyDropped ) {
// got an update for a player that's being held
// this means they've been dropped
					printf( "Baby dropped\n" );

					existing->currentPos.x = o.xd;
					existing->currentPos.y = o.yd;

					existing->currentSpeed = 0;
					existing->currentGridSpeed = 0;
					playPendingReceivedMessages( existing );

					existing->xd = o.xd;
					existing->yd = o.yd;
					existing->destTruncated = false;

// clear an existing path, since they may no
// longer be on it
					if( existing->pathToDest != NULL ) {
						delete [] existing->pathToDest;
						existing->pathToDest = NULL;
					}

					if( existing->lastHeldByRawPosSet ) {
						existing->heldByDropOffset =
								sub( existing->lastHeldByRawPos,
										existing->currentPos );
						if( length( existing->heldByDropOffset ) > 3 ) {
// too far to fly during drop
// snap instead
							existing->heldByDropOffset.x = 0;
							existing->heldByDropOffset.y = 0;
						}
					}
					else {
// held pos not known
// maybe this holding parent has never
// been drawn on the screen
// (can happen if they drop us during map load)
						existing->heldByDropOffset.x = 0;
						existing->heldByDropOffset.y = 0;
					}

					existing->lastHeldByRawPosSet = false;


					LiveObject *adultO =
							getGameObject( existing->heldByAdultID );

					if( adultO != NULL ) {
// fade from held animation
						existing->lastAnim =
								adultO->curHeldAnim;

						existing->animationFrameCount =
								adultO->heldAnimationFrameCount;

						existing->lastAnimFade = 1;
						existing->curAnim = ground;

						adultO->holdingID = 0;
					}

					existing->heldByAdultID = -1;
				}
				else if( done_moving > 0 && forced ) {

// don't ever force-update these for
// our locally-controlled object
// give illusion of it being totally responsive
// to move commands

// nor do we force-update remote players
// don't want glitches at the end of their moves

// UNLESS server tells us to force update
					existing->currentPos.x = o.xd;
					existing->currentPos.y = o.yd;

					existing->currentSpeed = 0;
					existing->currentGridSpeed = 0;

// clear an existing path, since they may no
// longer be on it
					if( existing->pathToDest != NULL ) {
						delete [] existing->pathToDest;
						existing->pathToDest = NULL;
					}

					if( ! existing->somePendingMessageIsMoreMovement ) {
						addNewAnim( existing, ground );
					}
					playPendingReceivedMessages( existing );

					existing->xd = o.xd;
					existing->yd = o.yd;
					existing->destTruncated = false;
				}

				if( existing->id == ourID ) {
// update for us

					if( !existing->inMotion ) {
// this is an update post-action, not post-move

// ready to execute next action
						playerActionPending = false;
						waitingForPong = false;
						playerActionTargetNotAdjacent = false;

						existing->pendingAction = false;
					}

					if( forced ) {
						existing->pendingActionAnimationProgress = 0;
						existing->pendingAction = false;

						playerActionPending = false;
						waitingForPong = false;
						playerActionTargetNotAdjacent = false;

						if( nextActionMessageToSend != NULL ) {
							delete [] nextActionMessageToSend;
							nextActionMessageToSend = NULL;
						}

// immediately send ack message
						char *forceMessage =
								autoSprintf( "FORCE %d %d#",
										existing->xd,
										existing->yd );
						sendToServerSocket( forceMessage );
						delete [] forceMessage;
					}
				}

// in motion until update received, now done
				if( existing->id != ourID ) {
					existing->inMotion = false;
				}
				else {
// only do this if our last requested move
// really ended server-side
					if( done_moving ==
						existing->lastMoveSequenceNumber ) {

						existing->inMotion = false;
					}

				}


				existing->moveTotalTime = 0;

				for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {

					int oldNumCont =
							existing->clothingContained[c].size();

					existing->clothingContained[c].deleteAll();

					int newNumClothingCont =
							o.clothingContained[c].size();
					int *newClothingCont =
							o.clothingContained[c].getElementArray();

					existing->clothingContained[c].appendArray(
							newClothingCont, newNumClothingCont );
					delete [] newClothingCont;

					if( ! clothingSoundPlayed &&
						newNumClothingCont > oldNumCont ) {

// insertion sound

						char soundPlayed = false;

						SoundUsage contSound =
								clothingByIndex(
										existing->clothing, c )->usingSound;

						if( contSound.numSubSounds > 0 ) {
							playSound( contSound,
									getVectorFromCamera(
											existing->xd,
											existing->yd ) );
							soundPlayed = true;
						}

						if( ! soundPlayed ) {
// no container using sound defined
// play player's using sound

							SoundUsage s = getObject(
									existing->displayID )->
									usingSound;

							if( s.numSubSounds > 0 ) {
								playSound(
										s,
										getVectorFromCamera(
												existing->xd,
												existing->yd ) );
							}
						}
					}
				}
			}
			else {
				o.displayChar = lastCharUsed + 1;

				lastCharUsed = o.displayChar;

				o.lastHoldingID = o.holdingID;

				if( o.holdingID < 0 ) {
// picked up a baby
					int babyID = - o.holdingID;

// save ALL of these for later, after
// we've processed all PU lines,
// because a held baby may not exist
// locally yet.
					unusedHolderID.push_back( o.id );
					unusedHeldID.push_back( babyID );
				}

				o.curAnim = ground;
				o.lastAnim = ground;
				o.lastAnimFade = 0;

				o.curHeldAnim = held;
				o.lastHeldAnim = held;
				o.lastHeldAnimFade = 0;

				o.hide = false;

				o.inMotion = false;
				o.lastMoveSequenceNumber = 1;

				o.holdingFlip = false;

				o.lastFlipSendTime = game_getCurrentTime();
				o.lastFlipSent = false;


				o.lastHeldByRawPosSet = false;

				o.pendingAction = false;
				o.pendingActionAnimationProgress = 0;

				o.currentPos.x = o.xd;
				o.currentPos.y = o.yd;

				o.destTruncated = false;

				o.heldPosOverride = false;
				o.heldPosOverrideAlmostOver = false;
				o.heldObjectPos = o.currentPos;
				o.heldObjectRot = 0;

				o.currentSpeed = 0;
				o.numFramesOnCurrentStep = 0;
				o.currentGridSpeed = 0;

				o.moveTotalTime = 0;

				o.futureAnimStack =
						new SimpleVector<AnimType>();
				o.futureHeldAnimStack =
						new SimpleVector<AnimType>();


				ObjectRecord *obj = getObject( o.displayID );

				if( obj->creationSound.numSubSounds > 0 ) {

					playSound( obj->creationSound,
							getVectorFromCamera(
									o.currentPos.x,
									o.currentPos.y ) );
				}

// insert in age order, youngest last
				double newAge = computeCurrentAge( &o );
				char inserted = false;
				for( int e=0; e<gameObjects.size(); e++ ) {
					if( computeCurrentAge( gameObjects.getElement( e ) )< newAge )
					{
						// found first younger, insert in front of it
						gameObjects.push_middle( o, e );
						this->getCasting()->setIndexRecentlyInsertedGameObject(e);
						//recentInsertedGameObjectIndex = e;
						inserted = true;
						break;
					}
				}
				if( ! inserted ) {
// they're all older than us
					gameObjects.push_back( o );
					this->getCasting()->setIndexRecentlyInsertedGameObject(gameObjects.size() - 1);
//recentInsertedGameObjectIndex = gameObjects.size() - 1;
				}
			}
		}
		else if( o.id == ourID &&
				 strstr( lines[i], "X X" ) != NULL  ) {
// we died

			printf( "Got X X death message for our ID %d\n",
					ourID );

// get age after X X
			char *xxPos = strstr( lines[i], "X X" );

			LiveObject *ourLiveObject = getOurLiveObject();

			if( xxPos != NULL ) {
				sscanf( xxPos, "X X %lf", &( ourLiveObject->age ) );
			}
			ourLiveObject->finalAgeSet = true;


			if( mDeathReason != NULL ) {
				delete [] mDeathReason;
			}
			char *reasonPos = strstr( lines[i], "reason" );

			if( reasonPos == NULL ) {
				mDeathReason = stringDuplicate(
						translate( "reasonUnknown" ) );
			}
			else {
				char reasonString[100];
				reasonString[0] = '\0';

				sscanf( reasonPos, "reason_%99s", reasonString );

				if( apocalypseInProgress ) {
					mDeathReason = stringDuplicate(
							translate( "reasonApocalypse" ) );
				}
				else if( strcmp( reasonString, "nursing_hunger" ) ==
						 0 ) {
					mDeathReason = stringDuplicate(
							translate( "reasonNursingHunger" ) );
				}
				else if( strcmp( reasonString, "hunger" ) == 0 ) {
					mDeathReason = stringDuplicate(
							translate( "reasonHunger" ) );
				}
				else if( strcmp( reasonString, "SID" ) == 0 ) {
					mDeathReason = stringDuplicate(
							translate( "reasonSID" ) );
				}
				else if( strcmp( reasonString, "suicide" ) == 0 ) {
					ObjectRecord *holdingO = NULL;

					if( ourLiveObject->holdingID > 0 ) {
						holdingO = getObject( ourLiveObject->holdingID );
					}

					if( holdingO == NULL ) {
						mDeathReason = autoSprintf(
								"%s%s",
								translate( "reasonKilled" ),
								translate( "you" ) );
					}
					else {

						char *stringUpper = stringToUpperCase(
								holdingO->description );

						stripDescriptionComment( stringUpper );


						mDeathReason = autoSprintf(
								"%s%s",
								translate( "reasonKilled" ),
								stringUpper );

						delete [] stringUpper;
					}
				}
				else if( strcmp( reasonString, "age" ) == 0 ) {
					mDeathReason = stringDuplicate(
							translate( "reasonOldAge" ) );
				}
				else if( strcmp( reasonString, "disconnected" ) == 0 ) {
					mDeathReason = stringDuplicate(
							translate( "reasonDisconnected" ) );
				}
				else if( strstr( reasonString, "killed" ) != NULL ) {

					int weaponID = 0;

					sscanf( reasonString, "killed_%d", &weaponID );

					ObjectRecord *weaponO = NULL;

					if( weaponID > 0 ) {
						weaponO = getObject( weaponID );
					}


					if( weaponO == NULL ) {
						mDeathReason = stringDuplicate(
								translate( "reasonKilledUnknown" ) );
					}
					else {

						char *stringUpper = stringToUpperCase(
								weaponO->description );

						stripDescriptionComment( stringUpper );


						mDeathReason = autoSprintf(
								"%s%s",
								translate( "reasonKilled" ),
								stringUpper );

						delete [] stringUpper;
					}
				}
				else if( strstr( reasonString, "succumbed" ) != NULL ) {

					int sicknessID = 0;

					sscanf( reasonString, "succumbed_%d", &sicknessID );

					ObjectRecord *sicknessO = NULL;

					if( sicknessID > 0 ) {
						sicknessO = getObject( sicknessID );
					}


					if( sicknessO == NULL ) {
						mDeathReason = stringDuplicate(
								translate( "reasonSuccumbedUnknown" ) );
					}
					else {

						char *stringUpper = stringToUpperCase(
								sicknessO->description );

						stripDescriptionComment( stringUpper );


						mDeathReason = autoSprintf(
								"%s%s",
								translate( "reasonSuccumbed" ),
								stringUpper );

						delete [] stringUpper;
					}
				}
				else {
					mDeathReason = stringDuplicate(
							translate( "reasonUnknown" ) );
				}
			}

			if( ! anyLiveTriggersLeft() ) {
// if we're in live trigger mode, leave
// server connection open so we can see what happens
// next
				this->socket->close();
				handleOurDeath();
			}
			else {
// just hide our object, but leave it around
				getOurLiveObject()->hide = true;
			}
		}
		else if( strstr( lines[i], "X X" ) != NULL  ) {
// object deleted

			numRead = sscanf( lines[i], "%d %d",
					&( o.id ),
					&( o.holdingID ) );


			for( int i=0; i<gameObjects.size(); i++ ) {

				LiveObject *nextObject =
						gameObjects.getElement( i );

				if( nextObject->id == o.id ) {

					if( nextObject->heldByAdultID > 0 ) {
// baby died while held, tell
// parent to drop them
						LiveObject *parent = getGameObject(
								nextObject->heldByAdultID );

						if( parent != NULL &&
							parent->holdingID == - o.id ) {
							parent->holdingID = 0;
						}
					}

// drop any pending messages about them
// we don't want these to play back later
					for( int j=0; j<gameObjects.size(); j++ ) {
						LiveObject *otherObject =
								gameObjects.getElement( j );

						dropPendingReceivedMessagesRegardingID(
								otherObject, o.id );
					}

// play any pending messages that are
// waiting for them to finish their move
					playPendingReceivedMessagesRegardingOthers(
							nextObject );

					if( nextObject->containedIDs != NULL ) {
						delete [] nextObject->containedIDs;
					}
					if( nextObject->subContainedIDs != NULL ) {
						delete [] nextObject->subContainedIDs;
					}

					if( nextObject->pathToDest != NULL ) {
						delete [] nextObject->pathToDest;
					}

					if( nextObject->currentSpeech != NULL ) {
						delete [] nextObject->currentSpeech;
					}

					if( nextObject->relationName != NULL ) {
						delete [] nextObject->relationName;
					}

					if( nextObject->name != NULL ) {
						delete [] nextObject->name;
					}

					delete nextObject->futureAnimStack;
					delete nextObject->futureHeldAnimStack;

					gameObjects.deleteElement( i );
					break;
				}
			}
		}

		delete [] holdingIDBuffer;
		delete [] clothingBuffer;

		delete [] lines[i];
	}

	for( int i=0; i<unusedHolderID.size(); i++ ) {
		LiveObject *existing =
				getGameObject( unusedHolderID.getElementDirect( i ) );

		LiveObject *babyO =
				getGameObject( unusedHeldID.getElementDirect( i ) );

		if( babyO != NULL && existing != NULL ) {
			babyO->heldByAdultID = existing->id;

			if( babyO->heldByAdultPendingID == existing->id ) {
// pending held finally happened
				babyO->heldByAdultPendingID = -1;
			}

			babyO->jumpOutOfArmsSentTime = 0;

// stop crying when held
			babyO->tempAgeOverrideSet = false;

			if( babyO->pathToDest != NULL ) {
// forget baby's old path
// they are going to be set down elsewhere
// far away from path
				delete [] babyO->pathToDest;
				babyO->pathToDest = NULL;
			}
			babyO->currentSpeed = 0;
			babyO->inMotion = false;

			if( babyO->id == ourID ) {
				if( nextActionMessageToSend != NULL ) {
// forget pending action, we've been interrupted
					delete [] nextActionMessageToSend;
					nextActionMessageToSend = NULL;
				}
				playerActionPending = false;
				waitingForPong = false;
			}


			existing->heldFrozenRotFrameCount =
					babyO->frozenRotFrameCount;

			existing->heldFrozenRotFrameCountUsed =
					false;


			if( babyO->lastAnimFade == 0 ) {

				existing->lastHeldAnim =
						babyO->curAnim;

				existing->heldAnimationFrameCount =
						babyO->animationFrameCount;

				existing->lastHeldAnimationFrameCount =
						babyO->lastAnimationFrameCount;

				existing->lastHeldAnimFade = 1;
				existing->curHeldAnim = held;
			}
			else {
// baby that we're picking up
// in the middle of an existing fade

				existing->lastHeldAnim =
						babyO->lastAnim;
				existing->lastHeldAnimationFrameCount =
						babyO->lastAnimationFrameCount;

				existing->curHeldAnim =
						babyO->curAnim;

				existing->heldAnimationFrameCount =
						babyO->animationFrameCount;


				existing->lastHeldAnimFade =
						babyO->lastAnimFade;

				existing->futureHeldAnimStack->
						push_back( held );
			}
		}
		if( existing != NULL && babyO == NULL ) {
// existing is holding a baby that does not exist
// this can happen when playing back pending PU messages
// when we're holding a baby that dies when we're walking

// holding nothing now instead
			existing->holdingID = 0;
		}

	}

	delete [] lines;
/**********************************************************************************************************************
}
/**********************************************************************************************************************/

