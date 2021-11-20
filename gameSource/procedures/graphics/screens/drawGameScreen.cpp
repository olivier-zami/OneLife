//
// Created by olivier on 17/11/2021.
//

#include "../screens.h"

#include <cstdio>
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h" //TODO put drawMessage in procedure/graphic

struct {
	bool intro;
}debug_var = {0};

void OneLife::game::graphic::drawGameScreen(
		int mFirstServerMessagesReceived,
		char mStartedLoadingFirstObjectSet,
		float mFirstObjectSetLoadingProgress,
		char mDoneLoadingFirstObjectSet
		)
{
	if(!debug_var.intro){printf("\n======>drawGameScreen");debug_var.intro=1;}

	char stillWaitingBirth = false;
	if( mFirstServerMessagesReceived != 3 )
	{
		stillWaitingBirth = true; // haven't gotten first messages from server yet
		printf("\n=====>waiting for birth");
	}
	else if( mFirstServerMessagesReceived == 3 )
	{
		if( !mDoneLoadingFirstObjectSet )
		{
			stillWaitingBirth = true;
		}
	}

	/*
	  if( stillWaitingBirth )
	  {
  #include "OneLife/gameSource/components/pages/waitingBirthPage.cpp"
	  }


	  int gridCenterX = lrintf( lastScreenViewCenter.x / CELL_D ) - mMapOffsetX + mMapD/2;
	  int gridCenterY = lrintf( lastScreenViewCenter.y / CELL_D ) - mMapOffsetY + mMapD/2;

  //FOV
  // SIDE NOTE:  These 4 variables control how far items should be rendered, separately from biome drawing

  // more on left and right of screen to avoid wide object tops popping in
  // SIDE NOTE:  x is scaled directly.  value * scale
	  int xStart = gridCenterX - (int)(ceil(7 * gui_fov_scale));
	  int xEnd = gridCenterX + (int)(ceil(7 * gui_fov_scale));

  // more on bottom of screen so that tall objects don't pop in
  // SIDE NOTE:  y is scaled with offset.
	  int yStart = gridCenterY - (int)(ceil(5 * gui_fov_scale) + 1);   // Default: 6  (5 * scale + 1)
	  int yEnd = gridCenterY + (int)(ceil(5 * gui_fov_scale) - 1);     // Default: 4   (5 * scale - 1)

	  if( xStart < 0 ) {
		  xStart = 0;
	  }
	  if( xStart >= mMapD ) {
		  xStart = mMapD - 1;
	  }

	  if( yStart < 0 ) {
		  yStart = 0;
	  }
	  if( yStart >= mMapD ) {
		  yStart = mMapD - 1;
	  }

	  if( xEnd < 0 ) {
		  xEnd = 0;
	  }
	  if( xEnd >= mMapD ) {
		  xEnd = mMapD - 1;
	  }

	  if( yEnd < 0 ) {
		  yEnd = 0;
	  }
	  if( yEnd >= mMapD ) {
		  yEnd = mMapD - 1;
	  }

  // For tripping color effect
	  isTrippingEffectOn = isTripping();

	  drawGround(
			  lastScreenViewCenter,
			  gridCenterX,//from LivingLifePage
			  gridCenterY,//from LivingLifePage
			  gui_fov_scale,//from game.cpp
			  isTrippingEffectOn,//from game.cpp
			  mMapD, //int mapWidth,//mMapD from LivingLifePage
			  mMapD, //int mapHeight,//mMapD from LivingLifePage
			  mMapCellDrawnFlags,//from LivingLifePage
			  groundSpritesArraySize,//#define groundSprite.h
			  mMapOffsetX,//from LivingLifePage::mMapOffsetX
			  mMapOffsetY,//from LivingLifePage::mMapOffsetY
			  mMapBiomes,// from LivingLifePage::mMapBiomes
			  groundSprites,//from groundSprite.h
			  valleyOffset,//from livingLifePage
			  mCulvertStoneSpriteIDs,//from livingLifePage
			  culvertFractalRoughness,
			  culvertFractalScale,
			  culvertFractalAmp,
			  valleySpacing);

  //!drawFloor
	  double hugR = CELL_D * 0.6;

  // draw floors on top of biome
	  for( int y=yEnd; y>=yStart; y-- ) {

		  int worldY = y + mMapOffsetY - mMapD / 2;

		  int screenY = CELL_D * worldY;

		  for( int x=xStart; x<=xEnd; x++ ) {

			  int worldX = x + mMapOffsetX - mMapD / 2;


			  int mapI = y * mMapD + x;

			  int oID = mMapFloors[mapI];


			  int screenX = CELL_D * worldX;

			  doublePair pos = { (double)screenX, (double)screenY };


			  char drawHuggingFloor = false;

  // for main floor, and left and right hugging floor
  // 0 to skip a pass
			  int passIDs[3] = { 0, 0, 0 };

			  if( oID > 0 ) {
				  passIDs[0] = oID;
			  }



			  if( oID <= 0) {


				  int cellOID = mMap[mapI];

				  if( cellOID > 0 && getObject( cellOID )->floorHugging ) {

					  if( x > 0 && mMapFloors[ mapI - 1 ] > 0 ) {
  // floor to our left
						  passIDs[1] = mMapFloors[ mapI - 1 ];
						  drawHuggingFloor = true;
					  }

					  if( x < mMapD - 1 && mMapFloors[ mapI + 1 ] > 0 ) {
  // floor to our right
						  passIDs[2] = mMapFloors[ mapI + 1 ];
						  drawHuggingFloor = true;

					  }
				  }


				  if( ! drawHuggingFloor ) {
					  continue;
				  }
			  }



			  int oldFrameCount =
					  mMapFloorAnimationFrameCount[ mapI ];

			  if( ! mapPullMode ) {
				  mMapFloorAnimationFrameCount[ mapI ] ++;
			  }



			  for( int p=0; p<3; p++ ) {
				  if( passIDs[p] == 0 ) {
					  continue;
				  }

				  oID = passIDs[p];

				  if( p > 0 ) {
					  setDrawColor( 1, 1, 1, 1 );
					  startAddingToStencil( false, true );
				  }

				  if( p == 1 ) {
					  drawRect( pos.x - hugR, pos.y + hugR,
							  pos.x, pos.y - hugR );
				  }
				  else if( p == 2 ) {

					  drawRect( pos.x, pos.y + hugR,
							  pos.x + hugR, pos.y - hugR );
				  }

				  if( p > 0 ) {
					  startDrawingThroughStencil();
				  }



				  if( !mapPullMode ) {
					  handleAnimSound( oID, 0, ground, oldFrameCount,
							  mMapFloorAnimationFrameCount[ mapI ],
							  (double)screenX / CELL_D,
							  (double)screenY / CELL_D );
				  }

				  double timeVal = frameRateFactor *
								   mMapFloorAnimationFrameCount[ mapI ] / 60.0;


				  if( p > 0 ) {
  // floor hugging pass

					  int numLayers = getObject( oID )->numSprites;

					  if( numLayers > 1 ) {
  // draw all but top layer of floor
						  setAnimLayerCutoff( numLayers - 1 );
					  }
				  }


				  char used;
				  drawObjectAnim( oID, 2,
						  ground, timeVal,
						  0,
						  ground,
						  timeVal,
						  timeVal,
						  &used,
						  ground,
						  ground,
						  pos, 0,
						  false,
						  false, -1,
						  false, false, false,
						  getEmptyClothingSet(), NULL );

				  if( p > 0 ) {
					  stopStencil();
				  }
			  }

			  if( passIDs[1] != passIDs[2] ) {
				  setDrawColor( 1, 1, 1, 1 );
				  pos.y += 10;
				  drawSprite( mFloorSplitSprite, pos );
			  }
		  }
	  }


  // draw overlay evenly over all floors and biomes
	  doublePair groundCenterPos;

	  int groundWTile = getSpriteWidth( mGroundOverlaySprite[0] );
	  int groundHTile = getSpriteHeight( mGroundOverlaySprite[0] );

	  int groundW = groundWTile * 2;
	  int groundH = groundHTile * 2;

	  groundCenterPos.x = lrint( lastScreenViewCenter.x / groundW ) * groundW;
	  groundCenterPos.y = lrint( lastScreenViewCenter.y / groundH ) * groundH;

	  toggleMultiplicativeBlend( true );

  // use this to lighten ground overlay
	  toggleAdditiveTextureColoring( true );
	  setDrawColor( multAmount, multAmount, multAmount, 1 );

	  for( int y=-1; y<=1; y++ ) {

		  doublePair pos = groundCenterPos;

		  pos.y = groundCenterPos.y + y * groundH;

		  for( int x=-1; x<=1; x++ ) {

			  pos.x = groundCenterPos.x + x * groundW;

			  if( drawMult ) {
				  for( int t=0; t<4; t++ ) {

					  doublePair posTile = pos;
					  if( t % 2 == 0 ) {
						  posTile.x -= groundWTile / 2;
					  }
					  else {
						  posTile.x += groundWTile / 2;
					  }

					  if( t < 2 ) {
						  posTile.y += groundHTile / 2;
					  }
					  else {
						  posTile.y -= groundHTile / 2;
					  }

					  drawSprite( mGroundOverlaySprite[t], posTile );
				  }
			  }
		  }
	  }
	  toggleAdditiveTextureColoring( false );
	  toggleMultiplicativeBlend( false );


	  toggleAdditiveBlend( true );

  // use this to lighten ground overlay
  //toggleAdditiveTextureColoring( true );
	  setDrawColor( 1, 1, 1, addAmount );

	  for( int y=-1; y<=1; y++ ) {

		  doublePair pos = groundCenterPos;

		  pos.x += 512;
		  pos.y += 512;

		  pos.y = groundCenterPos.y + y * groundH;

		  for( int x=-1; x<=1; x++ ) {

			  pos.x = groundCenterPos.x + x * groundW;

			  if( drawAdd )  {
				  for( int t=0; t<4; t++ ) {

					  doublePair posTile = pos;
					  if( t % 2 == 0 ) {
						  posTile.x -= groundWTile / 2;
					  }
					  else {
						  posTile.x += groundWTile / 2;
					  }

					  if( t < 2 ) {
						  posTile.y += groundHTile / 2;
					  }
					  else {
						  posTile.y -= groundHTile / 2;
					  }

					  drawSprite( mGroundOverlaySprite[t], posTile );
				  }
			  }
		  }
	  }
  //toggleAdditiveTextureColoring( false );
	  toggleAdditiveBlend( false );


	  float maxFullCellFade = 0.5;
	  float maxEmptyCellFade = 0.75;


	  if( mShowHighlights
		  &&
		  mCurMouseOverCellFade > 0
		  &&
		  mCurMouseOverCell.x >= 0 && mCurMouseOverCell.x < mMapD
		  &&
		  mCurMouseOverCell.y >= 0 && mCurMouseOverCell.y < mMapD ) {

		  int worldY = mCurMouseOverCell.y + mMapOffsetY - mMapD / 2;

		  int screenY = CELL_D * worldY;

		  int screenX =
				  CELL_D * ( mCurMouseOverCell.x + mMapOffsetX - mMapD / 2 );

		  int mapI = mCurMouseOverCell.y * mMapD + mCurMouseOverCell.x;

		  int id = mMap[mapI];

		  float fill = 0;
		  float border = 1;
		  char drawFill = false;
		  float maxFade = maxEmptyCellFade;

		  if( id > 0 ) {
			  fill = 1;
			  border = 0;
			  drawFill = true;
			  maxFade = maxFullCellFade;
		  }


		  doublePair cellPos = { (double)screenX, (double)screenY };

		  cellPos.x += 2;

		  if( drawFill ) {
			  setDrawColor( fill, fill, fill, maxFade * mCurMouseOverCellFade );
			  drawSprite( mCellFillSprite, cellPos );
		  }


		  setDrawColor( border, border, border,
				  0.75  * maxFade * mCurMouseOverCellFade );
		  drawSprite( mCellBorderSprite, cellPos );
	  }


	  if( mShowHighlights )
		  for( int i=0; i<mPrevMouseOverCells.size(); i++ ) {
			  float fade = mPrevMouseOverCellFades.getElementDirect( i );

			  if( fade <= 0 ) {
				  continue;
			  }

			  GridPos prev = mPrevMouseOverCells.getElementDirect( i );

			  if( prev.x < 0 || prev.x >= mMapD
				  ||
				  prev.y < 0 || prev.y >= mMapD ) {

				  continue;
			  }


			  int worldY = prev.y + mMapOffsetY - mMapD / 2;

			  int screenY = CELL_D * worldY;

			  int screenX =
					  CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );


			  int mapI = prev.y * mMapD + prev.x;

			  int id = mMap[mapI];

			  float fill = 0;
			  float border = 1;
			  char drawFill = false;
			  float maxFade = maxEmptyCellFade;

			  if( id > 0 ) {
				  fill = 1;
				  border = 0;
				  drawFill = true;
				  maxFade = maxFullCellFade;
			  }


			  doublePair cellPos = { (double)screenX, (double)screenY };

			  cellPos.x += 2;

			  if( drawFill ) {
				  setDrawColor( fill, fill, fill, maxFade * fade );
				  drawSprite( mCellFillSprite, cellPos );
			  }

			  setDrawColor( border, border, border, 0.75 * maxFade * fade );
			  drawSprite( mCellBorderSprite, cellPos );
		  }




	  if( mShowHighlights )
		  for( int i=0; i<mPrevMouseClickCells.size(); i++ ) {
			  float fade = mPrevMouseClickCellFades.getElementDirect( i );

			  if( fade <= 0 ) {
				  continue;
			  }

			  GridPos prev = mPrevMouseClickCells.getElementDirect( i );

			  if( prev.x < 0 || prev.x >= mMapD
				  ||
				  prev.y < 0 || prev.y >= mMapD ) {

				  continue;
			  }


			  int worldY = prev.y + mMapOffsetY - mMapD / 2;

			  int screenY = CELL_D * worldY;

			  int screenX =
					  CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );

			  float border = 1;
			  float maxFade = maxEmptyCellFade;


			  doublePair cellPos = { (double)screenX, (double)screenY };

			  cellPos.x += 2;

			  setDrawColor( border, border, border, 0.75 * maxFade * fade );
			  drawSprite( mCellBorderSprite, cellPos );
		  }


  //int worldXStart = xStart + mMapOffsetX - mMapD / 2;
  //int worldXEnd = xEnd + mMapOffsetX - mMapD / 2;

  //int worldYStart = xStart + mMapOffsetY - mMapD / 2;
  //int worldYEnd = xEnd + mMapOffsetY - mMapD / 2;


  // capture pointers to objects that are speaking and visible on
  // screen

  // draw speech on top of everything at end

	  SimpleVector<LiveObject *> speakers;
	  SimpleVector<doublePair> speakersPos;


  // draw long path for our character
	  LiveObject *ourLiveObject = getOurLiveObject();

	  if( ourLiveObject != NULL ) {

		  if( ourLiveObject->currentPos.x != ourLiveObject->xd
			  || ourLiveObject->currentPos.y != ourLiveObject->yd ) {

			  if( ourLiveObject->pathToDest != NULL &&
				  ourLiveObject->shouldDrawPathMarks &&
				  mShowHighlights ) {
  // highlight path

				  JenkinsRandomSource pathRand( 340930281 );

				  GridPos pathSpot = ourLiveObject->pathToDest[ 0 ];


				  GridPos endGrid =
						  ourLiveObject->pathToDest[ ourLiveObject->pathLength - 1 ];

				  doublePair endPos;
				  endPos.x = endGrid.x * CELL_D;
				  endPos.y = endGrid.y * CELL_D;


				  doublePair playerPos = mult( ourLiveObject->currentPos,
						  CELL_D );

				  double distFromEnd = distance( playerPos, endPos );

				  float endFade = 1.0f;


				  if( distFromEnd < 2 * CELL_D ) {
					  endFade = distFromEnd / ( 2 * CELL_D );
				  }


				  doublePair curPos;

				  curPos.x = pathSpot.x * CELL_D;
				  curPos.y = pathSpot.y * CELL_D;


				  GridPos pathSpotB = ourLiveObject->pathToDest[ 1 ];


				  doublePair nextPosB;

				  nextPosB.x = pathSpotB.x * CELL_D;
				  nextPosB.y = pathSpotB.y * CELL_D;


				  doublePair curDir = normalize( sub( nextPosB, curPos ) );


				  double turnFactor = .25;

				  int numStepsSinceDrawn = 0;
				  int drawOnStep = 6;


				  for( int p=1; p< ourLiveObject->pathLength; p++ ) {


					  GridPos pathSpotB = ourLiveObject->pathToDest[ p ];


					  doublePair nextPos;

					  nextPos.x = pathSpotB.x * CELL_D;
					  nextPos.y = pathSpotB.y * CELL_D;

					  int closeDist = 60;

					  if( p == ourLiveObject->pathLength - 1 ) {
						  closeDist = 20;
					  }


					  while( distance( curPos, nextPos ) > closeDist  ) {

						  doublePair dir = normalize( sub( nextPos, curPos ) );


						  if( dot( dir, curDir ) >= 0 ) {
							  curDir =
									  normalize(
											  add( curDir,
													  mult( dir, turnFactor ) ) );

						  }
						  else {
  // path doubles back on itself
  // smooth turning never resolves here
  // just make a sharp point in path instead
							  curDir = dir;
						  }

						  curPos = add( curPos,
								  mult( curDir, 6 ) );

						  setDrawColor( 0, 0, 0,
								  ourLiveObject->pathMarkFade * endFade );


						  doublePair drawPos = curPos;

						  if( numStepsSinceDrawn == 0 ) {

							  drawSprite( mPathMarkSprite, drawPos, 1.0,
									  -angle( curDir ) / ( 2 * M_PI ) + .25  );
						  }

						  numStepsSinceDrawn ++;
						  if( numStepsSinceDrawn == drawOnStep ) {
							  numStepsSinceDrawn = 0;
						  }
					  }
				  }

				  if( ourLiveObject->pathMarkFade < 1 ) {
					  ourLiveObject->pathMarkFade += 0.1 * frameRateFactor;

					  if( ourLiveObject->pathMarkFade > 1 ) {
						  ourLiveObject->pathMarkFade = 1;
					  }
				  }
			  }
		  }
		  else {
			  ourLiveObject->pathMarkFade = 0;
		  }
	  }

  //!drawPath_testVersion()
  // FIXME:  skip these that are off screen
  // of course, we may not end up drawing paths on screen anyway
  // (probably only our own destination), so don't fix this for now

  // draw paths and destinations under everything

  // debug overlay
	  if( false )
		  for( int i=0; i<gameObjects.size(); i++ ) {

			  LiveObject *o = gameObjects.getElement( i );


			  if( o->currentPos.x != o->xd || o->currentPos.y != o->yd ) {
  // destination

				  char *string = autoSprintf( "[%c]", o->displayChar );

				  doublePair pos;
				  pos.x = o->xd * CELL_D;
				  pos.y = o->yd * CELL_D;

				  setDrawColor( 1, 0, 0, 1 );
				  mainFont->drawString( string,
						  pos, alignCenter );
				  delete [] string;


				  if( o->pathToDest != NULL ) {
  // highlight path

					  for( int p=0; p< o->pathLength; p++ ) {
						  GridPos pathSpot = o->pathToDest[ p ];

						  pos.x = pathSpot.x * CELL_D;
						  pos.y = pathSpot.y * CELL_D;

						  setDrawColor( 1, 1, 0, 1 );
						  mainFont->drawString( "P",
								  pos, alignCenter );
					  }

  // draw waypoint

					  setDrawColor( 0, 1, 1, 1 );
					  pos.x = o->waypointX * CELL_D;
					  pos.y = o->waypointY * CELL_D;
					  mainFont->drawString( "W",
							  pos, alignCenter );

				  }
				  else {
					  pos.x = o->closestDestIfPathFailedX * CELL_D;
					  pos.y = o->closestDestIfPathFailedY * CELL_D;

					  setDrawColor( 1, 0, 1, 1 );
					  mainFont->drawString( "P",
							  pos, alignCenter );
				  }


			  }
		  }



  // reset flags so that only drawn objects get flagged
	  for( int i=0; i<gameObjects.size(); i++ ) {
		  gameObjects.getElement( i )->onScreen = false;
	  }

	  int numMoving = 0;
	  int movingObjectsIndices[ MAP_NUM_CELLS ];

  // prevent double-drawing when cell goes from moving to non-moving
  // mid-draw
	  char cellDrawn[ MAP_NUM_CELLS ];

	  memset( cellDrawn, false, MAP_NUM_CELLS );

	  for( int y=0; y<mMapD; y++ ) {
		  for( int x=0; x<mMapD; x++ ) {
			  int mapI = y * mMapD + x;

			  if( mMap[ mapI ] > 0 &&
				  mMapMoveSpeeds[ mapI ] > 0 ) {

				  movingObjectsIndices[ numMoving ] = mapI;

				  numMoving++;
			  }
		  }
	  }


	  PtrDrawMapCell ptrDrawMapCell = &LivingLifePage::drawMapCell;
	  PtrPutInMap ptrPutInMap = &LivingLifePage::putInMap;
	  PtrCopyFromMap ptrCopyFromMap = &LivingLifePage::copyFromMap;
	  PtrGetLiveObject ptrGetLiveObject = &LivingLifePage::getLiveObject;
	  PtrDrawLiveObject ptrDrawLiveObject = &LivingLifePage::drawLiveObject;

	  drawAgent(
			  this,
			  ptrDrawMapCell,
			  ptrPutInMap,
			  ptrCopyFromMap,
			  ptrGetLiveObject,
			  ptrDrawLiveObject,
			  xStart,
			  xEnd,
			  yStart,
			  yEnd,
			  mMapOffsetX,
			  mMapOffsetY,
			  mMapD,
			  mMapD,
			  cellDrawn,
			  mMap,
			  mMapMoveSpeeds,
			  //cellPos,
			  gameObjects,
			  numMoving,
			  movingObjectsIndices,
			  mMapMoveOffsets,
			  mMapExtraMovingObjects,
			  &speakers,
			  &speakersPos,
			  mainFont,
			  mMapExtraMovingObjectsDestWorldPos);



  // finally, draw any highlighted our-placements
	  if( mCurMouseOverID > 0 && ! mCurMouseOverSelf && mCurMouseOverBehind ) {
		  int worldY = mCurMouseOverSpot.y + mMapOffsetY - mMapD / 2;

		  int screenY = CELL_D * worldY;

		  int mapI = mCurMouseOverSpot.y * mMapD + mCurMouseOverSpot.x;
		  int screenX =
				  CELL_D * ( mCurMouseOverSpot.x + mMapOffsetX - mMapD / 2 );

  // highlights only
		  drawMapCell( mapI, screenX, screenY, true );
	  }

	  for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
		  if( mPrevMouseOverSpotsBehind.getElementDirect( i ) ) {

			  GridPos prev = mPrevMouseOverSpots.getElementDirect( i );

			  int worldY = prev.y + mMapOffsetY - mMapD / 2;

			  int screenY = CELL_D * worldY;

			  int mapI = prev.y * mMapD + prev.x;
			  int screenX =
					  CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );

  // highlights only
			  drawMapCell( mapI, screenX, screenY, true );
		  }
	  }


	  if( ! takingPhoto )
		  for( int i=0; i<speakers.size(); i++ ) {
			  LiveObject *o = speakers.getElementDirect( i );

			  doublePair pos = speakersPos.getElementDirect( i );


			  doublePair speechPos = pos;

			  speechPos.y += 84;

			  ObjectRecord *displayObj = getObject( o->displayID );


			  double age = computeCurrentAge( o );

			  doublePair headPos =
					  displayObj->spritePos[ getHeadIndex( displayObj, age ) ];

			  doublePair bodyPos =
					  displayObj->spritePos[ getBodyIndex( displayObj, age ) ];

			  doublePair frontFootPos =
					  displayObj->spritePos[ getFrontFootIndex( displayObj, age ) ];

			  headPos = add( headPos,
					  getAgeHeadOffset( age, headPos,
							  bodyPos, frontFootPos ) );
			  headPos = add( headPos,
					  getAgeBodyOffset( age, bodyPos ) );

			  speechPos.y += headPos.y;

			  int width = 250 * gui_fov_scale_hud;
			  int widthLimit = 250 * gui_fov_scale_hud;

			  double fullWidth =
					  handwritingFont->measureString( o->currentSpeech );

			  if( fullWidth < width ) {
				  width = (int)fullWidth;
			  }


			  speechPos.x -= width / 2;


			  drawChalkBackgroundString( speechPos, o->currentSpeech,
					  o->speechFade, widthLimit,
					  o );
		  }



	  for( int i=0; i<locationSpeech.size(); i++ ) {
		  LocationSpeech *ls = locationSpeech.getElement( i );

		  doublePair pos = ls->pos;


		  doublePair speechPos = pos;


		  speechPos.y += 84;

		  int width = 250 * gui_fov_scale_hud;
		  int widthLimit = 250 * gui_fov_scale_hud;

		  double fullWidth =
				  handwritingFont->measureString( ls->speech );

		  if( fullWidth < width ) {
			  width = (int)fullWidth;
		  }

		  speechPos.x -= width / 2;

		  drawChalkBackgroundString( speechPos, ls->speech,
				  ls->fade, widthLimit );
	  }


	  drawOffScreenSounds();



  /-*
		// for debugging home location

	  ClickRecord *home = getHomeLocation();

	  if( home != NULL ) {
		  setDrawColor( 1, 0, 0, 0.5 );

		  int screenY = CELL_D * home->y;

		  int screenX = CELL_D * home->x;

		  doublePair pos = { (double)screenX, (double)screenY };

		  drawSquare( pos, CELL_D * .45 );

		  int startX = CELL_D * ourLiveObject->currentPos.x;
		  int startY = CELL_D * ourLiveObject->currentPos.y;

		  doublePair start = { (double)startX, (double)startY };

		  doublePair delta = sub( pos, start );

		  int numSteps = length( delta ) / CELL_D;

		  delta = mult( delta, 1.0 / numSteps );

		  for( int i=0; i<numSteps; i++ ) {

			  pos = add( start, mult( delta, i ) );

			  drawSquare( pos, CELL_D * .25 );
			  }
		  }
	  *-/

  /-*
	  // for debugging click counts
	  for( int i=0; i<NUM_CLICK_RECORDS; i++ ) {
		  if( clicks[i].count > 0 ) {
			  setDrawColor( 1, 0, 0, 1 );

			  doublePair pos;
			  pos.x = clicks[i].x * CELL_D;
			  pos.y = clicks[i].y * CELL_D;

			  char *string = autoSprintf( "%d", clicks[i].count );

			  mainFont->drawString( string, pos, alignCenter );

			  delete [] string;
			  }
		  }
	  *-/


  //doublePair lastChunkCenter = { (double)( CELL_D * mMapOffsetX ),
  //                               (double)( CELL_D * mMapOffsetY ) };

	  setDrawColor( 0, 1, 0, 1 );

  // debug overlay
  //mainFont->drawString( "X",
  //                      lastChunkCenter, alignCenter );







	  if( false )
		  for( int i=0; i<trail.size(); i++ ) {
			  doublePair *p = trail.getElement( i );

			  setDrawColor( trailColors.getElementDirect( i ) );

			  mainFont->drawString( ".",
					  *p, alignCenter );
		  }


	  setDrawColor( 0, 0, 0, 0.125 );

	  int screenGridOffsetX = lrint( lastScreenViewCenter.x / CELL_D );
	  int screenGridOffsetY = lrint( lastScreenViewCenter.y / CELL_D );

  // debug overlay
	  if( false )
		  for( int y=-5; y<=5; y++ ) {
			  for( int x=-8; x<=8; x++ ) {

				  doublePair pos;
				  pos.x = ( x + screenGridOffsetX ) * CELL_D;
				  pos.y = ( y + screenGridOffsetY ) * CELL_D;

				  drawSquare( pos, CELL_D * .45 );
			  }
		  }

	  if( mapPullMode ) {

		  float progress;

		  if( ! mapPullCurrentSaved &&
			  isLiveObjectSetFullyLoaded( &progress ) ) {

			  int screenWidth, screenHeight;
			  getScreenDimensions( &screenWidth, &screenHeight );

			  Image *screen =
					  getScreenRegionRaw( 0, 0, screenWidth, screenHeight );

			  int startX = lastScreenViewCenter.x - screenW / 2;
			  int startY = lastScreenViewCenter.y + screenH / 2;
			  startY = -startY;
			  double scale = (double)screenWidth / (double)screenW;
			  startX = lrint( startX * scale );
			  startY = lrint( startY * scale );

			  int totalW = mapPullTotalImage->getWidth();
			  int totalH = mapPullTotalImage->getHeight();

			  int totalImStartX = startX + totalW / 2;
			  int totalImStartY = startY + totalH / 2;

			  double gridCenterOffsetX = ( mapPullEndX + mapPullStartX ) / 2.0;
			  double gridCenterOffsetY = ( mapPullEndY + mapPullStartY ) / 2.0;

			  totalImStartX -= lrint( gridCenterOffsetX * CELL_D  * scale );
			  totalImStartY += lrint( gridCenterOffsetY * CELL_D * scale );

  //totalImStartY =  totalH - totalImStartY;

			  if( totalImStartX >= 0 && totalImStartX < totalW &&
				  totalImStartY >= 0 && totalImStartY < totalH ) {

				  mapPullTotalImage->setSubImage( totalImStartX,
						  totalImStartY,
						  screenWidth, screenHeight,
						  screen );
				  numScreensWritten++;
			  }


			  delete screen;

			  mapPullCurrentSaved = true;

			  if( mapPullModeFinalImage ) {
				  mapPullMode = false;

				  writeTGAFile( "mapOut.tga", mapPullTotalImage );
				  delete mapPullTotalImage;


  // pull over

  // auto-quit
				  printf( "Map pull done, auto-quitting game\n" );
				  quitGame();
			  }
		  }

  // skip gui
		  return;
	  }


  // special mode for teaser video
	  if( teaserVideo ) {
  //setDrawColor( 1, 1, 1, 1 );
  //drawRect( lastScreenViewCenter, 640, 360 );

  // two passes
  // first for arrows, second for labels
  // make sure arrows don't cross labels
		  for( int pass=0; pass<2; pass++ )
			  for( int y=yEnd; y>=yStart; y-- ) {

				  int worldY = y + mMapOffsetY - mMapD / 2;
  //printf( "World y = %d\n", worldY );

				  if( worldY == 1 || worldY == 0 || worldY == -10 ) {

					  int xLimit = 0;

					  if( worldY == -10 ) {
						  xLimit = -20;
					  }

					  int screenY = CELL_D * worldY;


					  for( int x=xStart; x<=xEnd; x++ ) {

						  int worldX = x + mMapOffsetX - mMapD / 2;

						  if( worldX >= xLimit ) {

							  int mapI = y * mMapD + x;

							  int screenX = CELL_D * worldX;

							  int arrowTipX = screenX;
							  int arrowTipY = screenY;

							  const char *baseDes = NULL;

							  char numbered = false;

							  if( mMap[ mapI ] > 0 ) {
								  ObjectRecord *o = getObject( mMap[mapI] );

								  baseDes = o->description;

  // don't number natural objects
  // or objects that occur before the
  // start of our line
  // or use dummies (berry bush is picked from
  // at end of teaser video).
								  numbered = ( o->mapChance == 0 && worldX > 10
											   &&
											   ! o->isUseDummy );
							  }
							  else if( worldX == -20 && worldY == -10 &&
									   ( gameObjects.size() == 1 ||
										 gameObjects.size() == 2 ) ) {
								  baseDes = "PLAYER ONE";
								  LiveObject *o =  gameObjects.getElement( 0 );
								  arrowTipX = o->currentPos.x * CELL_D;
								  arrowTipY += 64;
							  }
							  else if( worldX == -10 && worldY == -10 &&
									   gameObjects.size() == 3 ) {
								  baseDes = "PLAYER TWO";
  // point to parent location
  // they pick up baby and keep walking
								  LiveObject *o =  gameObjects.getElement( 0 );
								  arrowTipX = o->currentPos.x * CELL_D;
								  arrowTipY += 64;
							  }


							  if( baseDes != NULL ) {

								  doublePair labelPos;


  // speeds are center weights picked from
  // 7/8, 8/8, or 9/8
  // 8/8 means the label sticks to the x-center
  // of the screen.
  // 7/8 means it lags behind the center a bit
  // 9/8 means it moves faster than the center a bit
								  double centerWeight = 8.0/8.0;


  // pick a new speed order every 6 tiles
								  double randOrder =
										  getXYRandom( worldX/6, worldY + 300 );

								  int orderOffsets[3] = { 0, 2, 4 };

  // even distribution among 0,1,2
								  int orderIndex =
										  lrint( ( randOrder * 2.98 ) - 0.49 );

								  int orderOffset = orderOffsets[ orderIndex ];

								  int worldXOffset = worldX - 11 + orderOffset;

								  if( ( worldX - 11 ) % 2 == 0 ) {

									  if( worldXOffset % 6 == 0 ) {
										  centerWeight = 8.0 / 8.0;
									  }
									  else if( worldXOffset % 6 == 2 ) {
										  centerWeight = 7.0/8.0;
									  }
									  else if( worldXOffset % 6 == 4 ) {
										  centerWeight = 9.0/8.0;
									  }
								  }
								  else {

									  if( worldXOffset % 6 == 1 ) {
										  centerWeight = 7.0 / 8.0;
									  }
									  else if( worldXOffset % 6 == 3 ) {
										  centerWeight = 8.0/8.0;
									  }
									  else if( worldXOffset % 6 == 5 ) {
										  centerWeight = 6.0/8.0;
									  }
								  }


								  labelPos.x =
										  (1 - centerWeight ) * screenX +
										  centerWeight * lastScreenViewCenter.x;

								  char labelAbove = false;

								  double xWiggle =
										  getXYRandom( worldX, 924 + worldY );

								  labelPos.x += lrint( (xWiggle - 0.5) *  512 );


								  labelPos.y = lastScreenViewCenter.y;

								  double yWiggle =
										  getXYRandom( worldX, 29 + worldY );

								  labelPos.y += lrint( (yWiggle - 0.5) * 32 );

								  SpriteHandle arrowSprite = mTeaserArrowMedSprite;

								  if( worldY != 0 ||
									  ( worldX > 0 && worldX < 7 ) ) {

									  if( ( worldX - 11 ) % 2 == 0 ) {
										  labelAbove = true;
										  labelPos.y += 214;

										  if( ( worldX - 11 ) % 6 == 2 ) {
											  labelPos.y -= 106;
											  arrowSprite =
													  mTeaserArrowVeryShortSprite;
										  }
										  else if( ( worldX - 11 ) % 6 == 4 ) {
											  labelPos.y += 86;
											  arrowSprite = mTeaserArrowLongSprite;

  // prevent top row from
  // going off top of screen
											  if( labelPos.y -
												  lastScreenViewCenter.y
												  > 224 + 86 ) {
												  labelPos.y = lastScreenViewCenter.y
															   + 224 + 86;
											  }
										  }
									  }
									  else {
										  labelPos.y -= 202;
										  arrowTipY -= 48;

										  if( ( worldX - 11 ) % 6 == 3 ) {
											  labelPos.y -= 106;
											  arrowSprite = mTeaserArrowLongSprite;
										  }
										  else if( ( worldX - 11 ) % 6 == 5 ) {
											  labelPos.y += 86;
											  arrowSprite = mTeaserArrowShortSprite;
										  }
									  }
								  }
								  else {
									  if( worldX % 2 == 0 ) {
										  labelPos.y = lastScreenViewCenter.y + 224;
										  labelAbove = true;
									  }
									  else {
										  labelPos.y = lastScreenViewCenter.y - 224;
										  labelAbove = false;
									  }
								  }

  // special case loincloth for baby
								  if( worldX == -1 && worldY == -10 ) {
									  labelPos.y = lastScreenViewCenter.y - 224;
									  labelAbove = false;
									  arrowTipY -= 48;
								  }
  // phonograph sleve
								  if( worldX == 3 && worldY == 0 ) {
									  labelPos.y = lastScreenViewCenter.y - 224;
									  labelAbove = false;
									  arrowTipY -= 48;
									  arrowSprite = mTeaserArrowVeryShortSprite;
								  }
  // phonograph
								  if( worldX == 4 && worldY == 0 ) {
									  labelPos.y = lastScreenViewCenter.y + 224;
									  labelAbove = true;
									  labelPos.x += 256;
									  arrowTipY += 48;
								  }

								  double fade = 0;

								  if( fabs( lastScreenViewCenter.x - screenX )
									  < 400 ) {

									  if( fabs( lastScreenViewCenter.x - screenX )
										  < 300 ) {
										  fade = 1;
									  }
									  else {
										  fade =
												  ( 400 - fabs( lastScreenViewCenter.x -
																screenX ) ) / 100.0;
									  }
								  }

								  if( fade > 0  ) {
									  if( fabs( lastScreenViewCenter.y - screenY )
										  > 200 ) {
										  fade = 0;
									  }
									  else if(
											  fabs( lastScreenViewCenter.y - screenY )
											  > 100 ) {
										  fade = ( 200 -
												   fabs( lastScreenViewCenter.y -
														 screenY ) ) / 100.0;
									  }
								  }





								  if( pass == 0 ) {
									  double arrowStart = 27;

									  if( labelAbove ) {
										  arrowStart = -21;
									  }

									  double w = 11;

									  double deltaX = arrowTipX - labelPos.x;
									  double deltaY = arrowTipY -
													  ( labelPos.y + arrowStart );

									  double a = atan( deltaX / deltaY );

									  a = 0.85 * a;

									  w = w / cos( a );

									  doublePair spriteVerts[4] =
											  { { labelPos.x - w,
														labelPos.y + arrowStart },
												{ labelPos.x + w,
														labelPos.y + arrowStart },
												{ (double)arrowTipX + w,
														(double)arrowTipY },
												{ (double)arrowTipX - w,
														(double)arrowTipY } };

									  FloatColor spriteColors[4] =
											  { { 1, 1, 1, (float)fade },
												{ 1, 1, 1, (float)fade },
												{ 1, 1, 1, (float)fade * .25f },
												{ 1, 1, 1, (float)fade * .25f } };

									  drawSprite( arrowSprite,
											  spriteVerts, spriteColors );
								  }
								  else {
									  char *des = stringDuplicate( baseDes );

									  char *poundPos = strstr( des, "#" );

									  if( poundPos != NULL ) {
  // terminate at pound
										  poundPos[0] = '\0';
									  }

									  SimpleVector<char*> *words =
											  tokenizeString( des );

									  delete [] des;

									  SimpleVector<char*> finalWords;
									  for( int i=0; i< words->size(); i++ ) {
										  char *word = words->getElementDirect( i );

										  if( word[0] >= 65 &&
											  word[0] <= 90 ) {

											  char *u = stringToUpperCase( word );

											  delete [] word;
											  finalWords.push_back( u );
										  }
										  else {
											  finalWords.push_back( word );
										  }
									  }
									  delete words;

									  char **wordArray = finalWords.getElementArray();

									  char *newDes = join( wordArray,
											  finalWords.size(),
											  " " );
									  finalWords.deallocateStringElements();
									  delete [] wordArray;


									  char *finalDes;

									  if( worldY == 1 && numbered ) {
										  finalDes = autoSprintf(
												  "%d. %s",
												  worldX - 10,
												  newDes );
									  }
									  else {
										  finalDes = stringDuplicate( newDes );
									  }

									  delete [] newDes;

									  double w =
											  mainFontReview->measureString( finalDes );

									  doublePair rectPos = labelPos;
									  rectPos.y += 3;




									  if( blackBorder ) {

  // block hole in border
										  startAddingToStencil( false, true );
										  setDrawColor( 1, 1, 1, 1 );
										  drawRect(
												  rectPos,
												  w/2 +
												  mainFontReview->getFontHeight() / 2,
												  24 );

										  startDrawingThroughStencil( true );

										  setDrawColor( 0, 0, 0, fade );
										  drawRect(
												  rectPos,
												  w/2 +
												  mainFontReview->getFontHeight() / 2 +
												  2,
												  24 + 2 );

										  stopStencil();
									  }

									  double rectW =
											  w/2 + mainFontReview->getFontHeight() / 2;

									  double rectH = 24;

									  setDrawColor( 1, 1, 1, fade );
									  drawRect( rectPos, rectW, rectH );


									  if( whiteBorder ) {

										  FloatColor lineColor =
												  { 1, 1, 1, (float)fade };

										  doublePair corners[4];

										  double rOut = 20;
										  double rIn = 4;



										  corners[0].x = rectPos.x - rectW -
														 getBoundedRandom( worldX, 34,
																 rOut, rIn );
										  corners[0].y = rectPos.y - rectH -
														 getBoundedRandom( worldX, 87,
																 rOut, rIn );


										  corners[1].x = rectPos.x + rectW +
														 getBoundedRandom( worldX, 94,
																 rOut, rIn );
										  corners[1].y = rectPos.y - rectH -
														 getBoundedRandom( worldX, 103,
																 rOut, rIn );


										  corners[2].x = rectPos.x + rectW +
														 getBoundedRandom( worldX, 99,
																 rOut, rIn );
										  corners[2].y = rectPos.y + rectH +
														 getBoundedRandom( worldX, 113,
																 rOut, rIn );


										  corners[3].x = rectPos.x - rectW -
														 getBoundedRandom( worldX, 123,
																 rOut, rIn );
										  corners[3].y = rectPos.y + rectH +
														 getBoundedRandom( worldX, 135,
																 rOut, rIn );


										  for( int i=0; i<4; i++ ) {

											  drawLine( mLineSegmentSprite,
													  corners[i],
													  corners[ ( i + 1 ) % 4 ],
													  lineColor );
										  }
									  }


									  setDrawColor( 0, 0, 0, fade );
									  mainFontReview->drawString(
											  finalDes,
											  labelPos,
											  alignCenter );

									  delete [] finalDes;
								  }
							  }
						  }
					  }
				  }
			  }
	  }


	  if( apocalypseInProgress ) {
		  toggleAdditiveBlend( true );

		  setDrawColor( 1, 1, 1, apocalypseDisplayProgress );

		  drawRect( lastScreenViewCenter, 640 * gui_fov_scale, 360 * gui_fov_scale );

		  toggleAdditiveBlend( false );
	  }


	  if( takingPhoto ) {

		  if( photoSequenceNumber == -1 ) {
			  photoSequenceNumber = getNextPhotoSequenceNumber();
		  }
		  else if( photoSig == NULL && ! waitingForPhotoSig ) {
			  char *message =
					  autoSprintf( "PHOTO %d %d %d#",
							  takingPhotoGlobalPos.x, takingPhotoGlobalPos.y,
							  photoSequenceNumber );
			  sendToServerSocket( message );
			  waitingForPhotoSig = true;
			  delete [] message;
		  }
		  else if( photoSig != NULL ) {
			  float currentFOV = gui_fov_scale;
			  changeFOV( 1.0 );

			  doublePair pos;

			  pos.x = takingPhotoGlobalPos.x;
			  pos.y = takingPhotoGlobalPos.y;


			  pos = mult( pos, CELL_D );
			  pos = sub( pos, lastScreenViewCenter );

			  int screenWidth, screenHeight;
			  getScreenDimensions( &screenWidth, &screenHeight );

			  pos.x += screenWidth / 2;
			  pos.y += screenHeight / 2;

			  char *ourName;

			  if( ourLiveObject->name != NULL ) {
				  ourName = ourLiveObject->name;
			  }
			  else {
				  ourName = (char*)translate( "namelessPerson" );
			  }

			  SimpleVector<int> subjectIDs;
			  SimpleVector<char*> subjectNames;

			  int xStart = takingPhotoGlobalPos.x + 1;

			  int xEnd;

			  if( takingPhotoFlip ) {
				  xStart = takingPhotoGlobalPos.x - 3;
				  xEnd = takingPhotoGlobalPos.x - 1;
			  }
			  else {
				  xEnd = xStart + 3;
			  }

			  int yStart = takingPhotoGlobalPos.y - 1;
			  int yEnd = yStart + 2;


			  for( int i=0; i<gameObjects.size(); i++ ) {

				  LiveObject *o = gameObjects.getElement( i );

				  if( o != ourLiveObject ) {
					  doublePair p = o->currentPos;

					  if( p.x >= xStart && p.x <= xEnd &&
						  p.y >= yStart && p.y <= yEnd ) {
						  subjectIDs.push_back( o->id );

						  if( o->name != NULL ) {
							  subjectNames.push_back( o->name );
						  }
					  }
				  }
			  }


			  takePhoto( pos, takingPhotoFlip ? -1 : 1,
					  photoSequenceNumber,
					  photoSig,
					  ourID,
					  ourName,
					  &subjectIDs,
					  &subjectNames );

			  takingPhoto = false;
			  delete [] photoSig;
			  photoSig = NULL;
			  photoSequenceNumber = -1;
			  waitingForPhotoSig = false;

			  changeFOV( currentFOV );
		  }
	  }



	  if( hideGuiPanel ) {
  // skip gui
		  return;
	  }

	  if( showFPS ) {

		  doublePair pos = lastScreenViewCenter;
		  pos.x -= 600;
		  pos.y += 300;

		  if( mTutorialNumber > 0 || mGlobalMessageShowing ) {
			  pos.y -= 50;
		  }

		  if( frameBatchMeasureStartTime == -1 ) {
			  frameBatchMeasureStartTime = game_getCurrentTime();
		  }
		  else {
			  framesInBatch ++;


			  if( framesInBatch == 30 ) {
				  double delta = game_getCurrentTime() -
								 frameBatchMeasureStartTime;

				  fpsToDraw = framesInBatch / delta;

				  addToGraph( &fpsHistoryGraph, fpsToDraw );

  // new batch
				  frameBatchMeasureStartTime = game_getCurrentTime();
				  framesInBatch = 0;
			  }
		  }
		  if( fpsToDraw != -1 ) {

			  char *fpsString =
					  autoSprintf( "%.1f %s", fpsToDraw, translate( "fps" ) );

			  drawFixedShadowString( fpsString, pos );

			  pos.x += 20 + numbersFontFixed->measureString( fpsString );
			  pos.y -= 20;

			  FloatColor yellow = { 1, 1, 0, 1 };
			  drawGraph( &fpsHistoryGraph, pos, yellow );

			  delete [] fpsString;
		  }
		  else {
			  drawFixedShadowString( translate( "fpsPending" ), pos );
		  }
	  }

	  if( showNet ) {
		  doublePair pos = lastScreenViewCenter;
		  pos.x -= 600;
		  pos.y += 300;

		  if( showFPS ) {
			  pos.y -= 50;
		  }
  // covered by tutorial sheets
		  if( mTutorialNumber > 0 || mGlobalMessageShowing ) {
			  pos.y -= 50;
		  }

		  double curTime = game_getCurrentTime();

		  if( netBatchMeasureStartTime == -1 ) {
			  netBatchMeasureStartTime = curTime;
		  }
		  else {

			  double batchLength = curTime - netBatchMeasureStartTime;

			  if( batchLength >= 1.0 ) {
  // full batch

				  messagesInPerSec = lrint( messagesInCount / batchLength );
				  messagesOutPerSec = lrint( messagesOutCount / batchLength );

				  bytesInPerSec = lrint( bytesInCount / batchLength );
				  bytesOutPerSec = lrint( bytesOutCount / batchLength );


				  addToGraph( &messagesInHistoryGraph, messagesInPerSec );
				  addToGraph( &messagesOutHistoryGraph, messagesOutPerSec );
				  addToGraph( &bytesInHistoryGraph, bytesInPerSec );
				  addToGraph( &bytesOutHistoryGraph, bytesOutPerSec );


  // new batch
				  messagesInCount = 0;
				  messagesOutCount = 0;
				  bytesInCount = 0;
				  bytesOutCount = 0;

				  netBatchMeasureStartTime = curTime;
			  }
		  }

		  if( messagesInPerSec != -1 ) {
			  char *netStringA =
					  autoSprintf( translate( "netStringA" ),
							  messagesOutPerSec, messagesInPerSec );
			  char *netStringB =
					  autoSprintf( translate( "netStringB" ),
							  bytesOutPerSec, bytesInPerSec );

			  drawFixedShadowString( netStringA, pos );

			  doublePair graphPos = pos;

			  graphPos.x += 20 + numbersFontFixed->measureString( netStringA );
			  graphPos.y -= 20;

			  FloatColor yellow = { 1, 1, 0, 1 };
			  drawGraph( &messagesOutHistoryGraph, graphPos, yellow );

			  graphPos.x += historyGraphLength + 10;
			  drawGraph( &messagesInHistoryGraph, graphPos, yellow );

			  pos.y -= 50;

			  drawFixedShadowString( netStringB, pos );

			  graphPos = pos;

			  graphPos.x += 20 + numbersFontFixed->measureString( netStringB );
			  graphPos.y -= 20;

			  drawGraph( &bytesOutHistoryGraph, graphPos, yellow );

			  graphPos.x += historyGraphLength + 10;
			  drawGraph( &bytesInHistoryGraph, graphPos, yellow );



			  delete [] netStringA;
			  delete [] netStringB;
		  }
		  else {
			  drawFixedShadowString( translate( "netPending" ), pos );
		  }
	  }

	  if( showPing ) {
		  if( pongDeltaTime != -1 &&
			  pingDisplayStartTime == -1 ) {
			  pingDisplayStartTime = game_getCurrentTime();
		  }

		  doublePair pos = lastScreenViewCenter;
		  pos.x += 300;
		  pos.y += 300;

		  if( mTutorialNumber > 0 || mGlobalMessageShowing ) {
			  pos.y -= 50;
		  }

		  char *pingString;

		  if( pongDeltaTime == -1 ) {
			  pingString =
					  autoSprintf( "%s...", translate( "ping" ) );
		  }
		  else {
			  pingString =
					  autoSprintf( "%s %d %s", translate( "ping" ),
							  lrint( pongDeltaTime * 1000 ),
							  translate( "ms" ) );
		  }

		  drawFixedShadowString( pingString, pos );

		  delete [] pingString;

		  if( pingDisplayStartTime != -1 &&
			  game_getCurrentTime() - pingDisplayStartTime > 10 ) {
			  showPing = false;
		  }
	  }

	  if( showHelp ) {

		  int columnNumber = 0;
		  int columnWidth = 450 * gui_fov_scale;
		  int columnHeight = 600 * gui_fov_scale;
		  int columnOffset = 300 * gui_fov_scale;

		  int lastDrawnColumn = 0;
		  int lineHeight = 40 * gui_fov_scale;

		  int columnStartX = -600 * gui_fov_scale;
		  int columnStartY = -100 * gui_fov_scale;

		  int temp;

		  doublePair writePos;
		  writePos.x = lastScreenViewCenter.x + columnStartX;
		  writePos.y = lastScreenViewCenter.y + columnStartY + columnHeight / 2;

		  File languagesDir( NULL, "languages" );
		  if ( languagesDir.exists() && languagesDir.isDirectory() ) {
			  File *helpFile = languagesDir.getChildFile( "help_English.txt" );
			  char *helpFileContents = helpFile->readFileContents();
			  if( helpFileContents != NULL ) {
				  int numLines;
				  char **lines = split( helpFileContents, "\n", &numLines );
				  char *subString;
				  for( int i=0; i<numLines; i++ ) {
					  bool isTitle = false;
					  bool isSub = false;
					  bool isCloseMessage = false;
					  if ( (lines[i][0] == '\0') || (lines[i][0] == '\r') ) {
  //continue;
					  }
					  else if ( strstr( lines[i], "@COLUMN_W" ) != NULL ) {
						  sscanf( lines[i], "@COLUMN_W=%d", &( temp ) );
						  columnWidth = gui_fov_scale * temp;
						  continue;
					  }
					  else if ( strstr( lines[i], "@COLUMN_H" ) != NULL ) {
						  sscanf( lines[i], "@COLUMN_H=%d", &( temp ) );
						  columnHeight = gui_fov_scale * temp;
						  writePos.y = lastScreenViewCenter.y + columnHeight/2;
						  continue;
					  }
					  else if ( strstr( lines[i], "@COLUMN_O=" ) != NULL ) {
						  sscanf( lines[i], "@COLUMN_O=%d", &( temp ) );
						  columnOffset = gui_fov_scale * temp;
						  continue;
					  }
					  else if ( strstr( lines[i], "@START_X" ) != NULL ) {
						  sscanf( lines[i], "@START_X=%d", &( temp ) );
						  columnStartX = gui_fov_scale * temp;
						  writePos.x = lastScreenViewCenter.x + columnStartX;
						  continue;
					  }
					  else if ( strstr( lines[i], "@START_Y" ) != NULL ) {
						  sscanf( lines[i], "@START_Y=%d", &( temp ) );
						  columnStartY = gui_fov_scale * temp;
						  writePos.y = lastScreenViewCenter.y + columnStartY + columnHeight/2;
						  continue;
					  }
					  else if ( strstr( lines[i], "@LINEHEIGHT" ) != NULL ) {
						  sscanf( lines[i], "@LINEHEIGHT=%d", &( temp ) );
						  lineHeight = gui_fov_scale * temp;
						  continue;
					  }
					  else if ( strstr( lines[i], "#sheet" ) != NULL ) {
						  sscanf( lines[i], "#sheet%d", &( columnNumber ) );
						  writePos.y = lastScreenViewCenter.y + columnStartY + columnHeight/2; //reset lineHeight additions
						  continue;
					  }
					  else if ( strstr( lines[i], "warning$" ) != NULL ) {
						  int hNumLines;
						  char **holder;
						  holder = split( lines[i], "$", &hNumLines);
						  lines[i] = holder[1];
						  isCloseMessage = true;
					  }
					  else if ( strstr( lines[i], "title$" ) != NULL ) {
						  int hNumLines;
						  char **holder;
						  holder = split( lines[i], "$", &hNumLines);
						  lines[i] = holder[1];
						  isTitle = true;
					  }
					  else if ( strstr( lines[i], "sub$" ) != NULL ) {
						  int hNumLines;
						  char **holder;
						  holder = split( lines[i], "$", &hNumLines);
						  lines[i] = holder[1];
						  subString = holder[2];
						  isSub = true;
					  }
					  else if ( strstr( lines[i], "space$" ) != NULL ) {
						  float lineScale;
						  sscanf( lines[i], "space$%f", &( lineScale ) );
						  writePos.y -= lineHeight * lineScale;
						  continue;
					  }

					  if ( columnNumber == 0 ) {
						  continue;
					  }
					  else if ( columnNumber > 1 ) {
						  int current_columnX = columnStartX + ( abs( columnWidth ) + columnOffset ) * ( columnNumber - 1 );
						  writePos.x = lastScreenViewCenter.x + current_columnX;
					  }

					  setDrawColor( 1, 1, 1, 0.85f );
					  if ( lastDrawnColumn != columnNumber ) {
						  if ( sheetSprites[columnNumber] == nullptr ) {
							  char columnName[11] = "sheet";
							  char n[6];
							  sprintf( n, "%d.tga", columnNumber );
							  strcat( columnName, n );
							  sheetSprites[columnNumber] = loadSprite( columnName, false );
						  }

						  doublePair drawPos;
						  drawPos.x = writePos.x + columnWidth/2;
						  drawPos.y = lastScreenViewCenter.y + columnStartY;
						  drawSprite( sheetSprites[columnNumber], drawPos, gui_fov_scale );
						  lastDrawnColumn = columnNumber;
					  }

					  if ( isCloseMessage ) {
						  closeMessage = lines[i];
					  }
					  else if ( isTitle ) {
						  setDrawColor( 0.1f, 0.1f, 0.1f, 1 );
						  int titleSize = titleFont->measureString( lines[i] );
						  titleFont->drawString( lines[i], { writePos.x + (columnWidth - titleSize)/2, writePos.y - lineHeight }, alignLeft );
						  writePos.y -= lineHeight * 1.5f;
					  }
					  else if ( isSub ) {
						  setDrawColor( 0.2f, 0.4f, 0.6f, 1 );
						  handwritingFont->drawString( lines[i], { writePos.x + 60 * gui_fov_scale, writePos.y - lineHeight * 0.75f }, alignLeft );
						  int subSize = handwritingFont->measureString( lines[i] );
						  setDrawColor( 0.1f, 0.1f, 0.1f, 1 );
						  pencilFont->drawString( subString, { writePos.x + subSize + 80 * gui_fov_scale, writePos.y - lineHeight * 0.75f }, alignLeft );
						  writePos.y -= lineHeight * 0.75;
					  }
					  else {
						  setDrawColor( 0.1f, 0.1f, 0.1f, 1 );
						  pencilFont->drawString( lines[i], { writePos.x + 40 * gui_fov_scale, writePos.y - lineHeight * 0.75f }, alignLeft );
						  writePos.y -= lineHeight;
					  }
				  }
				  delete [] lines;
			  }
		  }
	  }



	  doublePair slipPos = add( mult( recalcOffset( mHomeSlipPosOffset ), gui_fov_scale ), lastScreenViewCenter );

	  if( ! equal( mHomeSlipPosOffset, mHomeSlipHideOffset ) ) {
		  setDrawColor( 1, 1, 1, 1 );
		  drawSprite( mHomeSlipSprite, slipPos, gui_fov_scale_hud );


		  doublePair arrowPos = slipPos;

		  arrowPos.y += 35 * gui_fov_scale_hud;

		  if( ourLiveObject != NULL ) {

			  double homeDist = 0;

			  int arrowIndex = getHomeDir( ourLiveObject->currentPos, &homeDist );

			  if( arrowIndex == -1 || ! mHomeArrowStates[arrowIndex].solid ) {
  // solid change

  // fade any solid

				  int foundSolid = -1;
				  for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
					  if( mHomeArrowStates[i].solid ) {
						  mHomeArrowStates[i].solid = false;
						  foundSolid = i;
					  }
				  }
				  if( foundSolid != -1 ) {
					  for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
						  if( i != foundSolid ) {
							  mHomeArrowStates[i].fade -= 0.0625;
							  if( mHomeArrowStates[i].fade < 0 ) {
								  mHomeArrowStates[i].fade = 0;
							  }
						  }
					  }
				  }
			  }

			  if( arrowIndex != -1 ) {
				  mHomeArrowStates[arrowIndex].solid = true;
				  mHomeArrowStates[arrowIndex].fade = 1.0;
			  }

			  toggleMultiplicativeBlend( true );

			  toggleAdditiveTextureColoring( true );

			  for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
				  HomeArrow a = mHomeArrowStates[i];

				  if( ! a.solid ) {

					  float v = 1.0 - a.fade;
					  setDrawColor( v, v, v, 1 );
					  drawSprite( mHomeArrowErasedSprites[i], arrowPos, gui_fov_scale_hud );
				  }
			  }

			  toggleAdditiveTextureColoring( false );



			  if( arrowIndex != -1 ) {


				  setDrawColor( 1, 1, 1, 1 );

				  drawSprite( mHomeArrowSprites[arrowIndex], arrowPos, gui_fov_scale_hud );
			  }

			  toggleMultiplicativeBlend( false );

			  char drawTopAsErased = true;

			  doublePair distPos = arrowPos;

			  distPos.y -= 47 * gui_fov_scale_hud;

			  if( homeDist > 1000 ) {
				  drawTopAsErased = false;

				  setDrawColor( 0, 0, 0, 1 );

				  char *distString = NULL;

				  double thousands = homeDist / 1000;

				  if( thousands < 1000 ) {
					  if( thousands < 10 ) {
						  distString = autoSprintf( "%.1fK", thousands );
					  }
					  else {
						  distString = autoSprintf( "%.0fK",
								  thousands );
					  }
				  }
				  else {
					  double millions = homeDist / 1000000;
					  if( millions < 1000 ) {
						  if( millions < 10 ) {
							  distString = autoSprintf( "%.1fM", millions );
						  }
						  else {
							  distString = autoSprintf( "%.0fM", millions );
						  }
					  }
					  else {
						  double billions = homeDist / 1000000000;

						  distString = autoSprintf( "%.1fG", billions );
					  }
				  }



				  pencilFont->drawString( distString, distPos, alignCenter );

				  char alreadyOld = false;

				  for( int i=0; i<mPreviousHomeDistStrings.size(); i++ ) {
					  char *oldString =
							  mPreviousHomeDistStrings.getElementDirect( i );

					  if( strcmp( oldString, distString ) == 0 ) {
  // hit
						  alreadyOld = true;
  // move to top
						  mPreviousHomeDistStrings.deleteElement( i );
						  mPreviousHomeDistStrings.push_back( oldString );

						  mPreviousHomeDistFades.deleteElement( i );
						  mPreviousHomeDistFades.push_back( 1.0f );
						  break;
					  }
				  }

				  if( ! alreadyOld ) {
  // put new one top
					  mPreviousHomeDistStrings.push_back( distString );
					  mPreviousHomeDistFades.push_back( 1.0f );

  // fade old ones
					  for( int i=0; i<mPreviousHomeDistFades.size() - 1; i++ ) {
						  float fade =
								  mPreviousHomeDistFades.getElementDirect( i );

						  if( fade > 0.5 ) {
							  fade -= 0.20;
						  }
						  else {
							  fade -= 0.1;
						  }

						  *( mPreviousHomeDistFades.getElement( i ) ) =
								  fade;

						  if( fade <= 0 ) {
							  mPreviousHomeDistFades.deleteElement( i );
							  mPreviousHomeDistStrings.
									  deallocateStringElement( i );
							  i--;
						  }
					  }
				  }
				  else {
					  delete [] distString;
				  }
			  }

			  int numPrevious = mPreviousHomeDistStrings.size();

			  if( numPrevious > 1 ||
				  ( numPrevious == 1 && drawTopAsErased ) ) {

				  int limit = mPreviousHomeDistStrings.size() - 1;

				  if( drawTopAsErased ) {
					  limit += 1;
				  }
				  for( int i=0; i<limit; i++ ) {
					  float fade =
							  mPreviousHomeDistFades.getElementDirect( i );
					  char *string =
							  mPreviousHomeDistStrings.getElementDirect( i );

					  setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
					  pencilErasedFont->drawString(
							  string, distPos, alignCenter );
				  }
			  }
		  }
	  }



	  for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
		  if( ! equal( mHintPosOffset[i], mHintHideOffset[i] )
			  &&
			  mHintMessage[i] != NULL && !minitech::minitechEnabled ) {

			  doublePair hintPos  =
					  add( mult( recalcOffset( mHintPosOffset[i] ), gui_fov_scale ), lastScreenViewCenter );

			  hintPos = add( hintPos, mult( mHintExtraOffset[i], gui_fov_scale_hud ) );


			  char *pageNum = NULL;
			  double pageNumExtra = 0;

			  if( mNumTotalHints[i] > 1 ) {

				  pageNum =
						  autoSprintf( "(%d %s %d)",
								  mHintMessageIndex[i] + 1,
								  translate( "ofHint" ),
								  mNumTotalHints[i] );

				  double extraA = handwritingFont->measureString( pageNum );

				  double extraB =
						  handwritingFont->measureString( translate( "tabHint" ) );

				  if( extraB > extraA ) {
					  extraA = extraB;
				  }

				  pageNumExtra = extraA;

				  hintPos.x -= extraA;
				  hintPos.x -= 10 * gui_fov_scale_hud;
			  }


			  setDrawColor( 1, 1, 1, 1 );
  //FOV
  // Hint sheets have to be manually cut off in centered mode.
			  if( gui_hud_mode != 0 && gui_fov_target_scale_hud > 1.0f ) {
				  doublePair sheetPos[4] = {
						  { hintPos.x - getSpriteWidth( mHintSheetSprites[0] ) / 2.0f * gui_fov_scale_hud, hintPos.y + getSpriteHeight( mHintSheetSprites[0] ) / 2.0f * gui_fov_scale_hud },
						  { lastScreenViewCenter.x + 640 * gui_fov_scale_hud, hintPos.y + getSpriteHeight( mHintSheetSprites[0] ) / 2.0f * gui_fov_scale_hud },
						  { lastScreenViewCenter.x + 640 * gui_fov_scale_hud, hintPos.y - getSpriteHeight( mHintSheetSprites[0] ) / 2.0f * gui_fov_scale_hud },
						  { hintPos.x - getSpriteWidth( mHintSheetSprites[0] ) / 2.0f * gui_fov_scale_hud, hintPos.y - getSpriteHeight( mHintSheetSprites[0] ) / 2.0f * gui_fov_scale_hud },
				  };
				  double sheetLength = ( sheetPos[1].x - sheetPos[0].x ) / ( getSpriteWidth( mHintSheetSprites[0] ) * gui_fov_scale_hud );
				  doublePair sheetCoords[4] = {
						  { 0.0f, 0.0f },
						  { sheetLength, 0.0f },
						  { sheetLength, 1.0f },
						  { 0.0f, 1.0f },
				  };
				  drawSprite( mHintSheetSprites[i], sheetPos, sheetCoords );
			  }
			  else {
				  drawSprite( mHintSheetSprites[i], hintPos, gui_fov_scale_hud );
			  }


			  setDrawColor( 0, 0, 0, 1.0f );
			  double lineSpacing = handwritingFont->getFontHeight() / 2 + ( 5 * gui_fov_scale_hud );

			  int numLines;

			  char **lines = split( mHintMessage[i], "#", &numLines );

			  doublePair lineStart = hintPos;
			  lineStart.x -= (280 * gui_fov_scale_hud);
			  lineStart.y += (30 * gui_fov_scale_hud);
			  for( int l=0; l<numLines; l++ ) {

				  if( l == 1 ) {
					  doublePair drawPos = lineStart;
					  drawPos.x -= 5 * gui_fov_scale_hud;

					  handwritingFont->drawString( "+",
							  drawPos, alignRight );
				  }

				  if( l == 2 ) {
					  doublePair drawPos = lineStart;
					  drawPos.x -= 5 * gui_fov_scale_hud;

					  handwritingFont->drawString( "=",
							  drawPos, alignRight );
				  }

				  handwritingFont->drawString( lines[l],
						  lineStart, alignLeft );

				  delete [] lines[l];

				  lineStart.y -= lineSpacing;
			  }
			  delete [] lines;


			  if( pageNum != NULL ) {

  // now draw tab message

				  lineStart = hintPos;
				  lineStart.x -= (280 * gui_fov_scale_hud);

				  lineStart.x -= mHintExtraOffset[i].x * gui_fov_scale_hud;
				  lineStart.x += (20 * gui_fov_scale_hud);

				  lineStart.y += (30 * gui_fov_scale_hud);

				  handwritingFont->drawString( pageNum, lineStart, alignLeft );

				  delete [] pageNum;

				  lineStart.y -= 2 * lineSpacing;

				  lineStart.x += pageNumExtra;
				  handwritingFont->drawString( translate( "tabHint" ),
						  lineStart, alignRight );
			  }
		  }
	  }



  // now draw tutorial sheets
	  if( mTutorialNumber > 0 || mGlobalMessageShowing )
		  for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
			  if( ! equal( mTutorialPosOffset[i], mTutorialHideOffset[i] ) ) {

				  doublePair tutorialPos  =
						  add( mult( recalcOffset( mTutorialPosOffset[i], true ), gui_fov_scale ), lastScreenViewCenter );

				  if( i % 2 == 1 ) {
					  tutorialPos = sub( tutorialPos, mult( mTutorialExtraOffset[i], gui_fov_scale_hud ) );
				  }
				  else {
					  tutorialPos = add( tutorialPos, mult( mTutorialExtraOffset[i], gui_fov_scale_hud ) );
				  }

				  setDrawColor( 1, 1, 1, 1 );
  // rotate 180
				  drawSprite( mHintSheetSprites[i], tutorialPos, gui_fov_scale_hud, 0.5,
						  mTutorialFlips[i] );


				  setDrawColor( 0, 0, 0, 1.0f );
				  double lineSpacing = handwritingFont->getFontHeight() / 2 + 16 * gui_fov_scale_hud;

				  int numLines;

				  char **lines = split( mTutorialMessage[i], "##", &numLines );

				  doublePair lineStart = tutorialPos;

				  if( i % 2 == 1 ) {
					  lineStart.x -= 289 * gui_fov_scale_hud;
  //lineStart.x += mTutorialExtraOffset[i].x * gui_fov_scale_hud;
				  }
				  else {
					  lineStart.x += 289 * gui_fov_scale_hud;
					  lineStart.x -= mTutorialExtraOffset[i].x * gui_fov_scale_hud;
				  }

				  lineStart.y += 8 * gui_fov_scale_hud;
				  for( int l=0; l<numLines; l++ ) {

					  handwritingFont->drawString( lines[l],
							  lineStart, alignLeft );

					  delete [] lines[l];

					  lineStart.y -= lineSpacing;
				  }
				  delete [] lines;
			  }
		  }










	  double highestCravingYOffset = 0;

	  if( mLiveCravingSheetIndex != -1 ) {
  // craving showing
  // find highest one
		  highestCravingYOffset = 0;

		  for( int c=0; c<NUM_HINT_SHEETS; c++ ) {
			  double offset = mCravingPosOffset[c].y - mCravingHideOffset[c].y;
			  if( offset > highestCravingYOffset ) {
				  highestCravingYOffset = offset;
			  }
		  }
	  }




	  setDrawColor( 1, 1, 1, 1 );

	  for( int i=0; i<3; i++ ) {
		  if( !equal( mHungerSlipPosOffset[i], mHungerSlipHideOffsets[i] ) ) {
			  doublePair slipPos = lastScreenViewCenter;
			  slipPos = add( slipPos, mult( recalcOffset( mHungerSlipPosOffset[i] ), gui_fov_scale ) );

			  if( mHungerSlipWiggleAmp[i] > 0 ) {

				  double distFromHidden =
						  ( mHungerSlipPosOffset[i].y - mHungerSlipHideOffsets[i].y ) * gui_fov_scale_hud;

  // amplitude grows when we are further from
  // hidden, and shrinks again as we go back down

				  double slipHarmonic =
						  ( 0.5 * ( 1 - cos( mHungerSlipWiggleTime[i] ) ) ) *
						  mHungerSlipWiggleAmp[i] * distFromHidden;

				  slipPos.y += slipHarmonic;


				  if( i == 2 ) {

					  if( mStarvingSlipLastPos[0] != 0 &&
						  mStarvingSlipLastPos[1] != 0 ) {
						  double lastDir = mStarvingSlipLastPos[1] -
										   mStarvingSlipLastPos[0];


						  if( lastDir > 0 ) {

							  double newDir =
									  slipHarmonic - mStarvingSlipLastPos[1];

							  if( newDir < 0 ) {
  // peak
								  if( mPulseHungerSound &&
									  mHungerSound != NULL ) {
  // make sure it can be heard, even
  // if paused
									  setSoundLoudness( 1.0 );
									  playSoundSprite( mHungerSound,
											  getSoundEffectsLoudness(),
  // middle
											  0.5 );
								  }
							  }
						  }

					  }
					  mStarvingSlipLastPos[0] = mStarvingSlipLastPos[1];

					  mStarvingSlipLastPos[1] = slipHarmonic;
				  }
			  }

			  if ( showHelp ) {
				  setDrawColor( 1, 1, 1, 0.2f );
			  }

			  slipPos.y += lrint( highestCravingYOffset / 1.75 ) * gui_fov_scale_hud;

			  drawSprite( mHungerSlipSprites[i], slipPos, gui_fov_scale_hud );
		  }
	  }



	  for( int i=0; i<NUM_YUM_SLIPS; i++ ) {

		  if( ! equal( mYumSlipPosOffset[i], mYumSlipHideOffset[i] ) ) {
			  doublePair slipPos =
					  add( mult( recalcOffset( mYumSlipPosOffset[i] ), gui_fov_scale ), lastScreenViewCenter );

  // slipPos.y += lrint( highestCravingYOffset / 1.75 ) * gui_fov_scale_hud;

			  setDrawColor( 1, 1, 1, 1 );
			  drawSprite( mYumSlipSprites[i], slipPos, gui_fov_scale_hud );

			  doublePair messagePos = slipPos;
			  messagePos.y += 11 * gui_fov_scale_hud;

			  if( mYumSlipNumberToShow[i] != 0 ) {
				  char *s = autoSprintf( "%dx", mYumSlipNumberToShow[i] );

				  setDrawColor( 0, 0, 0, 1 );
				  handwritingFont->drawString( s, messagePos, alignCenter );
				  delete [] s;
			  }
			  if( i == 2 || i == 3 ) {
				  setDrawColor( 0, 0, 0, 1 );

				  const char *word;

				  if( i == 3 ) {
					  word = translate( "meh" );
				  }
				  else {
					  word = translate( "yum" );
				  }

				  handwritingFont->drawString( word, messagePos, alignCenter );
			  }
		  }
	  }



  // now draw craving sheets
	  if( mLiveCravingSheetIndex > -1 )
		  for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
			  if( ! equal( mCravingPosOffset[i], mCravingHideOffset[i] ) ) {

				  doublePair cravingPos  =
						  add( mult( recalcOffset( mCravingPosOffset[i] ), gui_fov_scale ), lastScreenViewCenter );

				  cravingPos = add( cravingPos, mult( mCravingExtraOffset[i], gui_fov_scale_hud ) );

				  setDrawColor( 1, 1, 1, 1.0 );
  // flip, don't rotate
				  drawSprite( mHintSheetSprites[i], cravingPos, gui_fov_scale_hud, 0.0, true );

				  setDrawColor( 0, 0, 0, 1.0f );

				  doublePair lineStart = cravingPos;

				  lineStart.x += 298 * gui_fov_scale_hud;
				  lineStart.x -= mCravingExtraOffset[i].x * gui_fov_scale_hud;

				  lineStart.y += 26 * gui_fov_scale_hud;

				  handwritingFont->drawString( mCravingMessage[i],
						  lineStart, alignLeft );

			  }
		  }




  // finally, draw chat note sheet, so that it covers craving sheet
  // whenever it is up.

	  int lineSpacing = 20 * gui_fov_scale_hud;

	  doublePair notePos = add( mult( recalcOffset( mNotePaperPosOffset ), gui_fov_scale ), lastScreenViewCenter );

	  if( ! equal( mNotePaperPosOffset, mNotePaperHideOffset ) ) {
		  setDrawColor( 1, 1, 1, 1 );
		  drawSprite( mNotePaperSprite, notePos, gui_fov_scale_hud );


		  doublePair drawPos = notePos;

		  drawPos.x += 160 * gui_fov_scale_hud;
		  drawPos.y += 79 * gui_fov_scale_hud;
		  drawPos.y += 22 * gui_fov_scale_hud;

		  drawPos.x += 27 * gui_fov_scale_hud;

		  setDrawColor( 0, 0, 0, 1 );

		  handwritingFont->drawString( translate( "enterHint" ),
				  drawPos,
				  alignRight );
	  }




	  doublePair paperPos = add( mult( recalcOffset( mNotePaperPosOffset ), gui_fov_scale ), lastScreenViewCenter );

	  if( mSayField.isFocused() ) {
		  char *partialSay = mSayField.getText();

		  char *strUpper = stringToUpperCase( partialSay );

		  delete [] partialSay;

		  SimpleVector<char*> *lines = splitLines( strUpper, ( 345 * gui_fov_scale_hud ) );

		  mNotePaperPosTargetOffset.y = mNotePaperHideOffset.y + 58;

		  if( lines->size() > 1 ) {
			  mNotePaperPosTargetOffset.y += 20 * ( lines->size() - 1 );
		  }

		  doublePair drawPos = paperPos;

		  drawPos.x -= 160 * gui_fov_scale_hud;
		  drawPos.y += 79 * gui_fov_scale_hud;


		  doublePair drawPosTemp = drawPos;


		  for( int i=0; i<mLastKnownNoteLines.size(); i++ ) {
			  char *oldString = mLastKnownNoteLines.getElementDirect( i );
			  int oldLen = strlen( oldString );

			  SimpleVector<doublePair> charPos;

			  pencilFont->getCharPos( &charPos,
					  oldString,
					  drawPosTemp,
					  alignLeft );

			  int newLen = 0;

			  if( i < lines->size() ) {
  // compare lines

				  newLen = strlen( lines->getElementDirect( i ) );

			  }


  // any extra chars?

			  for( int j=newLen; j<oldLen; j++ ) {
				  mErasedNoteChars.push_back( oldString[j] );

				  mErasedNoteCharOffsets.push_back(
						  sub( mult( charPos.getElementDirect( j ), 1. / gui_fov_scale_hud ),
								  paperPos ) );

				  mErasedNoteCharFades.push_back( 1.0f );
			  }

			  drawPosTemp.y -= lineSpacing;
		  }
		  mLastKnownNoteLines.deallocateStringElements();

		  for( int i=0; i<lines->size(); i++ ) {
			  mLastKnownNoteLines.push_back(
					  stringDuplicate( lines->getElementDirect(i) ) );
		  }



		  delete [] strUpper;



		  setDrawColor( 0, 0, 0, 1 );

		  mCurrentNoteChars.deleteAll();
		  mCurrentNoteCharOffsets.deleteAll();

		  for( int i=0; i<lines->size(); i++ ) {
			  char *line = lines->getElementDirect( i );

			  pencilFont->drawString( line, drawPos,
					  alignLeft );

			  SimpleVector<doublePair> charPos;

			  pencilFont->getCharPos( &charPos,
					  line,
					  drawPos,
					  alignLeft );

			  int lineSize = strlen( line );

			  for( int j=0; j<lineSize; j++ ) {
				  mCurrentNoteChars.push_back( line[j] );
				  mCurrentNoteCharOffsets.push_back(
						  sub( mult( charPos.getElementDirect( j ), 1. / gui_fov_scale_hud ), paperPos ) );
			  }

			  drawPos.y -= lineSpacing;
		  }
		  lines->deallocateStringElements();
		  delete lines;
	  }
	  else {
		  mNotePaperPosTargetOffset = mNotePaperHideOffset;

		  doublePair drawPos = paperPos;

		  drawPos.x -= 160 * gui_fov_scale_hud;
		  drawPos.y += 79 * gui_fov_scale_hud;

		  for( int i=0; i<mLastKnownNoteLines.size(); i++ ) {
  // whole line gone

			  char *oldString = mLastKnownNoteLines.getElementDirect( i );
			  int oldLen = strlen( oldString );

			  SimpleVector<doublePair> charPos;

			  pencilFont->getCharPos( &charPos,
					  oldString,
					  drawPos,
					  alignLeft );

			  for( int j=0; j<oldLen; j++ ) {
				  mErasedNoteChars.push_back( oldString[j] );

				  mErasedNoteCharOffsets.push_back(
						  sub( mult( charPos.getElementDirect( j ), 1. / gui_fov_scale_hud ),
								  paperPos ) );

				  mErasedNoteCharFades.push_back( 1.0f );
			  }

			  drawPos.y -= lineSpacing;

		  }
		  mLastKnownNoteLines.deallocateStringElements();
	  }



	  setDrawColor( 0, 0, 0, 1 );
	  for( int i=0; i<mErasedNoteChars.size(); i++ ) {
		  setDrawFade( mErasedNoteCharFades.getElementDirect( i ) *
					   pencilErasedFontExtraFade );

		  pencilErasedFont->
				  drawCharacterSprite(
				  mErasedNoteChars.getElementDirect( i ),
				  add( paperPos,
						  mult( mErasedNoteCharOffsets.getElementDirect( i ), gui_fov_scale_hud ) ) );
	  }






  // info panel at bottom, over top of all the other slips
	  setDrawColor( 1, 1, 1, 1 );
	  doublePair panelPos = lastScreenViewCenter;

  //FOV
	  panelPos.y -= recalcOffsetY( 242 + 32 + 16 + 6 ) * gui_fov_scale;
  // First left part.
	  if( gui_hud_mode == 0 ) {
		  panelPos.x = lastScreenViewCenter.x - recalcOffsetX( 384 ) * gui_fov_scale;
		  drawSprite( guiPanelLeftSprite, panelPos, gui_fov_scale_hud );
	  }
	  else if ( gui_hud_mode == 1 && gui_fov_target_scale_hud > 1.0f ) {
		  drawHUDBarPart(	lastScreenViewCenter.x - 640 * gui_fov_scale,
				  lastScreenViewCenter.y - recalcOffsetY( 360 ) * gui_fov_scale,
				  ( 1280.0 * gui_fov_scale / 2.0 ) - 640 * gui_fov_scale_hud,
				  getSpriteHeight( guiPanelTileSprite ) * gui_fov_scale_hud );
	  }

  // Now the middle.
	  if( gui_hud_mode == 0 )	{
		  drawHUDBarPart(	lastScreenViewCenter.x - recalcOffsetX( 128 ) * gui_fov_scale,
				  lastScreenViewCenter.y - recalcOffsetY( 360 ) * gui_fov_scale,
				  recalcOffsetX( 128 ) * 2 * gui_fov_scale,
				  getSpriteHeight( guiPanelTileSprite ) * gui_fov_scale_hud );
	  }
	  else {
		  drawSprite( mGuiPanelSprite, panelPos, gui_fov_scale_hud );
	  }

  // And finally draw the right end.
	  if( gui_hud_mode == 0 )	{
		  panelPos.x = lastScreenViewCenter.x + recalcOffsetX( 384 ) * gui_fov_scale;
		  drawSprite( guiPanelRightSprite, panelPos, gui_fov_scale_hud );
	  }
	  else if ( gui_hud_mode == 1 && gui_fov_target_scale_hud > 1.0f )	{
		  drawHUDBarPart(	lastScreenViewCenter.x + 640 * gui_fov_scale_hud,
				  lastScreenViewCenter.y - recalcOffsetY( 360 ) * gui_fov_scale,
				  ( 1280.0 * gui_fov_scale / 2.0 ) - 640 * gui_fov_scale_hud,
				  getSpriteHeight( guiPanelTileSprite ) * gui_fov_scale_hud );
	  }

	  panelPos.x = lastScreenViewCenter.x;

	  if ( showHelp && closeMessage != NULL ) {
		  setDrawColor( 0.4f, 0.1f, 0.1f, 1 );
		  handwritingFont->drawString( closeMessage, { lastScreenViewCenter.x - 0.5 * handwritingFont->measureString( closeMessage ), lastScreenViewCenter.y - 285 * gui_fov_scale }, alignLeft );
	  }

	  if( ourLiveObject != NULL &&
		  ourLiveObject->dying  &&
		  ! ourLiveObject->sick ) {
		  toggleMultiplicativeBlend( true );
		  doublePair bloodPos = panelPos;
		  bloodPos.y -= 32 * gui_fov_scale_hud;
		  bloodPos.x -= 32 * gui_fov_scale_hud;
		  drawSprite( mGuiBloodSprite, bloodPos, gui_fov_scale_hud );
		  toggleMultiplicativeBlend( false );
	  }




	  if( ourLiveObject != NULL ) {

  // draw curse token status
		  Font *curseTokenFont;
		  if( ourLiveObject->curseTokenCount > 0 ) {
			  setDrawColor( 0, 0, 0, 1.0 );
			  curseTokenFont = pencilFont;
		  }
		  else {
			  setDrawColor( 0, 0, 0, pencilErasedFontExtraFade );
			  curseTokenFont = pencilErasedFont;
		  }

  // show as a sigil to right of temp meter
		  doublePair curseTokenPos = { lastScreenViewCenter.x + ( recalcOffsetX( 621 ) * gui_fov_scale ),
									   lastScreenViewCenter.y - ( recalcOffsetY( 316 ) * gui_fov_scale )};
		  curseTokenFont->drawString( "C", curseTokenPos, alignCenter );
		  curseTokenFont->drawString( "+", curseTokenPos, alignCenter );
		  curseTokenPos.x += ( 6 * gui_fov_scale_hud );
		  curseTokenFont->drawString( "X", curseTokenPos, alignCenter );


  // for now, we receive at most one update per life, so
  // don't need to worry about showing erased version of this
		  if( ourLiveObject->excessCursePoints > 0 ) {
			  setDrawColor( 0, 0, 0, 1.0 );
			  doublePair pointsPos = curseTokenPos;
			  pointsPos.y -= curseTokenFont->getFontHeight();
			  pointsPos.x -= ( 3 * gui_fov_scale_hud );

			  char *pointString = autoSprintf( "%d",
					  ourLiveObject->excessCursePoints );
			  pencilFont->drawString( pointString, pointsPos, alignCenter );
			  delete [] pointString;
		  }

  //char *curseString = autoSprintf( "%d", ourLiveObject->curseLevel );
  //curseTokenPos.x -= ( 3 * scaleHUD );
  //curseTokenPos.y -= curseTokenFont->getFontHeight();
  //handwritingFont->drawString( curseString, curseTokenPos, alignCenter );

		  setDrawColor( 1, 1, 1, 1 );
		  toggleMultiplicativeBlend( true );

  //!draw foodMeter
		  for( int i=0; i<ourLiveObject->foodCapacity; i++ ) {
			  doublePair pos = { lastScreenViewCenter.x - ( recalcOffsetX( 590 ) * gui_fov_scale ),
								 lastScreenViewCenter.y - ( recalcOffsetY( 334 ) * gui_fov_scale )};
			  pos.x += i * ( 30 * gui_fov_scale_hud );

			  drawSprite(
					  mHungerBoxSprites[ i % NUM_HUNGER_BOX_SPRITES ],
					  pos, gui_fov_scale_hud );

			  if( i < ourLiveObject->foodStore ) {
				  drawSprite(
						  mHungerBoxFillSprites[ i % NUM_HUNGER_BOX_SPRITES ],
						  pos, gui_fov_scale_hud );
			  }
			  else if( i < ourLiveObject->maxFoodStore ) {
				  drawSprite(
						  mHungerBoxFillErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ],
						  pos, gui_fov_scale_hud );
			  }
		  }


  //!draw foodMeter
		  for( int i=ourLiveObject->foodCapacity;
			   i < ourLiveObject->maxFoodCapacity; i++ ) {
			  doublePair pos = { lastScreenViewCenter.x - ( recalcOffsetX( 590 ) * gui_fov_scale ),
								 lastScreenViewCenter.y - ( recalcOffsetY( 334 ) * gui_fov_scale )};
			  pos.x += i * ( 30 * gui_fov_scale_hud );

			  drawSprite(
					  mHungerBoxErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ],
					  pos, gui_fov_scale_hud );

			  if( i < ourLiveObject->maxFoodStore ) {
				  drawSprite(
						  mHungerBoxFillErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ],
						  pos, gui_fov_scale_hud );
			  }
		  }



		  doublePair pos = { lastScreenViewCenter.x + ( recalcOffsetX( 546 ) * gui_fov_scale ),
							 lastScreenViewCenter.y - ( recalcOffsetY( 319 ) * gui_fov_scale )};



		  if( mCurrentArrowHeat != -1 ) {

			  if( mCurrentArrowHeat != ourLiveObject->heat ) {

				  for( int i=0; i<mOldArrows.size(); i++ ) {
					  OldArrow *a = mOldArrows.getElement( i );

					  a->fade -= 0.01;
					  if( a->fade < 0 ) {
						  mOldArrows.deleteElement( i );
						  i--;
					  }
				  }


				  OldArrow a;
				  a.i = mCurrentArrowI;
				  a.heat = mCurrentArrowHeat;
				  a.fade = 1.0;

				  mOldArrows.push_back( a );

				  mCurrentArrowI++;
				  mCurrentArrowI = mCurrentArrowI % NUM_TEMP_ARROWS;
			  }
		  }


		  toggleAdditiveTextureColoring( true );

		  for( int i=0; i<mOldArrows.size(); i++ ) {
			  doublePair pos2 = pos;
			  OldArrow *a = mOldArrows.getElement( i );

			  float v = 1.0 - a->fade;
			  setDrawColor( v, v, v, 1 );
			  pos2.x += ( a->heat - 0.5 ) * ( 120 * gui_fov_scale_hud );

  // no sub pixel positions
			  pos2.x = round( pos2.x );

			  drawSprite( mTempArrowErasedSprites[a->i], pos2, gui_fov_scale_hud );
		  }
		  toggleAdditiveTextureColoring( false );

		  setDrawColor( 1, 1, 1, 1 );

		  mCurrentArrowHeat = ourLiveObject->heat;

		  pos.x += ( mCurrentArrowHeat - 0.5 ) * ( 120 * gui_fov_scale_hud );

  // no sub pixel positions
		  pos.x = round( pos.x );

		  drawSprite( mTempArrowSprites[mCurrentArrowI], pos, gui_fov_scale_hud );

		  toggleMultiplicativeBlend( false );


		  for( int i=0; i<mOldDesStrings.size(); i++ ) {
			  doublePair pos = { lastScreenViewCenter.x,
								 lastScreenViewCenter.y - ( recalcOffsetY( 313 ) * gui_fov_scale )};
			  float fade =
					  mOldDesFades.getElementDirect( i );

			  setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
			  pencilErasedFont->drawString(
					  mOldDesStrings.getElementDirect( i ), pos, alignCenter );
		  }

		  doublePair yumPos = { lastScreenViewCenter.x - ( recalcOffsetX( 480 ) * gui_fov_scale ),
								lastScreenViewCenter.y - ( recalcOffsetY( 313 ) * gui_fov_scale )};
		  setDrawColor( 0, 0, 0, 1 );
		  if( mYumBonus > 0 ) {
			  char *yumString = autoSprintf( "+%d", mYumBonus );

			  pencilFont->drawString( yumString, yumPos, alignLeft );
			  delete [] yumString;
		  }

		  for( int i=0; i<mOldYumBonus.size(); i++ ) {
			  float fade =
					  mOldYumBonusFades.getElementDirect( i );

			  setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
			  char *yumString = autoSprintf( "+%d",
					  mOldYumBonus.getElementDirect( i ) );
			  pencilErasedFont->drawString( yumString, yumPos, alignLeft );
			  delete [] yumString;
		  }



		  doublePair atePos = { lastScreenViewCenter.x,
								lastScreenViewCenter.y - ( recalcOffsetY( 347 ) * gui_fov_scale )};

		  int shortestFill = 100;


		  for( int i=0; i<mOldLastAteStrings.size(); i++ ) {
			  float fade =
					  mOldLastAteFades.getElementDirect( i );

			  setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );

			  pencilErasedFont->drawString(
					  mOldLastAteStrings.getElementDirect( i ), atePos, alignLeft );

			  toggleMultiplicativeBlend( true );
			  toggleAdditiveTextureColoring( true );

			  float v = 1.0f - mOldLastAteBarFades.getElementDirect( i );
			  setDrawColor( v, v, v, 1 );

			  int fillMax = mOldLastAteFillMax.getElementDirect( i );

			  if( fillMax < shortestFill ) {
				  shortestFill = fillMax;
			  }

			  drawHungerMaxFillLine( atePos,
					  fillMax,
					  mHungerBarErasedSprites,
					  mHungerDashErasedSprites,
					  false,
  // only draw dashes once, for longest
  // one
					  true );


			  toggleAdditiveTextureColoring( false );
			  toggleMultiplicativeBlend( false );
		  }

		  if( shortestFill < 100 ) {
			  toggleMultiplicativeBlend( true );
			  setDrawColor( 1, 1, 1, 1 );

			  drawHungerMaxFillLine( atePos,
					  shortestFill,
					  mHungerBarErasedSprites,
					  mHungerDashErasedSprites,
					  true,
  // draw longest erased dash line once
					  false );
			  toggleMultiplicativeBlend( false );
		  }


		  if( mCurrentLastAteString != NULL ) {
			  setDrawColor( 0, 0, 0, 1 );

			  pencilFont->drawString(
					  mCurrentLastAteString, atePos, alignLeft );


			  toggleMultiplicativeBlend( true );
			  setDrawColor( 1, 1, 1, 1 );

			  drawHungerMaxFillLine( atePos,
					  mCurrentLastAteFillMax,
					  mHungerBarSprites,
					  mHungerDashSprites,
					  false, false );

			  toggleMultiplicativeBlend( false );
		  }


		  if( mCurMouseOverID != 0 || mLastMouseOverID != 0 ) {
			  int idToDescribe = mCurMouseOverID;

			  if( mCurMouseOverID == 0 ) {
				  idToDescribe = mLastMouseOverID;
			  }



			  doublePair pos = { lastScreenViewCenter.x,
								 lastScreenViewCenter.y - ( recalcOffsetY( 313 ) * gui_fov_scale )};

			  char *des = NULL;
			  char *desToDelete = NULL;

			  if( idToDescribe == -99 ) {
				  if( ourLiveObject->holdingID > 0 &&
					  getObject( ourLiveObject->holdingID )->foodValue > 0 ) {

					  des = autoSprintf( "%s %s",
							  translate( "eat" ),
							  getObject( ourLiveObject->holdingID )->
									  description );
					  desToDelete = des;
				  }
				  else if( ourLiveObject->dying &&
						   ourLiveObject->holdingID > 0 ) {
					  des = autoSprintf( "%s %s",
							  translate( "youWith" ),
							  getObject( ourLiveObject->holdingID )->
									  description );
					  desToDelete = des;
				  }
				  else {
					  des = (char*)translate( "you" );
					  if( ourLiveObject->name != NULL ) {
						  des = autoSprintf( "%s - %s", des,
								  ourLiveObject->name );
						  desToDelete = des;
					  }
				  }
			  }
			  else if( idToDescribe < 0 ) {
				  LiveObject *otherObj = getLiveObject( -idToDescribe );

				  if( otherObj != NULL ) {
					  des = otherObj->relationName;
				  }
				  if( des == NULL ) {
					  des = (char*)translate( "unrelated" );
				  }
				  if( otherObj != NULL && otherObj->name != NULL ) {
					  des = autoSprintf( "%s - %s",
							  otherObj->name, des );
					  desToDelete = des;
				  }
				  if( otherObj != NULL &&
					  otherObj->dying && otherObj->holdingID > 0 ) {
					  des = autoSprintf( "%s - %s %s",
							  des,
							  translate( "with" ),
							  getObject( otherObj->holdingID )->
									  description );
					  if( desToDelete != NULL ) {
						  delete [] desToDelete;
					  }

					  desToDelete = des;
				  }
			  }
			  else {
				  ObjectRecord *o = getObject( idToDescribe );

				  des = o->description;

				  if( strstr( des, "origGrave" ) != NULL ) {
					  char found = false;

					  for( int g=0; g<mGraveInfo.size(); g++ ) {
						  GraveInfo *gI = mGraveInfo.getElement( g );

						  if( gI->worldPos.x == mCurMouseOverWorld.x &&
							  gI->worldPos.y == mCurMouseOverWorld.y ) {

							  char *desNoComment = stringDuplicate( des );
							  stripDescriptionComment( desNoComment );

  // a grave we know about
							  int years =
									  lrint(
											  ( game_getCurrentTime() -
												gI->creationTime ) *
											  ourLiveObject->ageRate );

							  if( gI->creationTimeUnknown ) {
								  years = 0;
							  }

							  double currentTime = game_getCurrentTime();

							  if( gI->lastMouseOverYears != -1 &&
								  currentTime - gI->lastMouseOverTime < 2 ) {
  // continuous mouse-over
  // don't let years tick up
								  years = gI->lastMouseOverYears;
								  gI->lastMouseOverTime = currentTime;
							  }
							  else {
  // save it for next time
								  gI->lastMouseOverYears = years;
								  gI->lastMouseOverTime = currentTime;
							  }

							  const char *yearWord;
							  if( years == 1 ) {
								  yearWord = translate( "yearAgo" );
							  }
							  else {
								  yearWord = translate( "yearsAgo" );
							  }

							  char *yearsString;

							  if( years > 20 ) {
								  if( years > 1000000 ) {
									  int mil = years / 1000000;
									  int remain = years % 1000000;
									  int thou = remain / 1000;
									  int extra = remain % 1000;
									  yearsString =
											  autoSprintf( "%d,%d,%d",
													  mil, thou, extra );
								  }
								  else if( years > 1000 ) {
									  yearsString =
											  autoSprintf( "%d,%d",
													  years / 1000,
													  years % 1000 );
								  }
								  else {
									  yearsString = autoSprintf( "%d", years );
								  }
							  }
							  else {
								  const char *numberKeys[21] = {
										  "zero",
										  "one",
										  "two",
										  "three",
										  "four",
										  "five",
										  "six",
										  "seven",
										  "eight",
										  "nine",
										  "ten",
										  "eleven",
										  "twelve",
										  "thirteen",
										  "fourteen",
										  "fifteen",
										  "sixteen",
										  "seventeen",
										  "eighteen",
										  "nineteen",
										  "twenty"
								  };
								  yearsString = stringDuplicate(
										  translate( numberKeys[ years ] ) );
							  }



							  char *deathPhrase;

							  if( years == 0 ) {
								  deathPhrase = stringDuplicate( "" );
							  }
							  else {
								  deathPhrase =
										  autoSprintf( " - %s %s %s",
												  translate( "died" ),
												  yearsString, yearWord );
							  }

							  delete [] yearsString;

							  des = autoSprintf( "%s %s %s%s",
									  desNoComment, translate( "of" ),
									  gI->relationName,
									  deathPhrase );
							  delete [] desNoComment;
							  delete [] deathPhrase;

							  desToDelete = des;
							  found = true;
							  break;
						  }
					  }



					  if( !found ) {

						  char alreadySent = false;
						  for( int i=0; i<graveRequestPos.size(); i++ ) {
							  if( equal( graveRequestPos.getElementDirect( i ),
									  mCurMouseOverWorld ) ) {
								  alreadySent = true;
								  break;
							  }
						  }

						  if( !alreadySent ) {
							  char *graveMessage =
									  autoSprintf( "GRAVE %d %d#",
											  mCurMouseOverWorld.x,
											  mCurMouseOverWorld.y );

							  sendToServerSocket( graveMessage );
							  delete [] graveMessage;
							  graveRequestPos.push_back( mCurMouseOverWorld );
						  }

  // blank des for now
  // avoid flicker when response arrives
						  des = stringDuplicate( "" );
						  desToDelete = des;
					  }

				  }
				  else if( o->isOwned ) {
					  char found = false;

					  for( int g=0; g<mOwnerInfo.size(); g++ ) {
						  OwnerInfo *gI = mOwnerInfo.getElement( g );

						  if( gI->worldPos.x == mCurMouseOverWorld.x &&
							  gI->worldPos.y == mCurMouseOverWorld.y ) {

							  char *desNoComment = stringDuplicate( des );
							  stripDescriptionComment( desNoComment );


							  const char *personName =
									  translate( "unknownPerson" );

							  double minDist = DBL_MAX;
							  LiveObject *ourLiveObject = getOurLiveObject();

							  for( int p=0; p< gI->ownerList->size(); p++ ) {
								  int pID = gI->ownerList->getElementDirect( p );

								  if( pID == ourID ) {
									  personName = translate( "YOU" );
									  break;
								  }
								  LiveObject *pO = getLiveObject( pID );
								  if( pO != NULL ) {
									  double thisDist =
											  distance( pO->currentPos,
													  ourLiveObject->currentPos );
									  if( thisDist < minDist ) {
										  minDist = thisDist;
										  if( pO->name != NULL ) {
											  personName = pO->name;
										  }
										  else {
											  personName =
													  translate( "namelessPerson" );
										  }
									  }
								  }
							  }


  // an owned object we know about
							  des = autoSprintf( "%s %s %s",
									  desNoComment,
									  translate( "ownedBy" ),
									  personName );
							  delete [] desNoComment;

							  desToDelete = des;
							  found = true;
							  break;
						  }
					  }

					  if( !found ) {

						  char alreadySent = false;
						  for( int i=0; i<ownerRequestPos.size(); i++ ) {
							  if( equal( ownerRequestPos.getElementDirect( i ),
									  mCurMouseOverWorld ) ) {
								  alreadySent = true;
								  break;
							  }
						  }

						  if( !alreadySent ) {
							  char *ownerMessage =
									  autoSprintf( "OWNER %d %d#",
											  mCurMouseOverWorld.x,
											  mCurMouseOverWorld.y );

							  sendToServerSocket( ownerMessage );
							  delete [] ownerMessage;
							  ownerRequestPos.push_back( mCurMouseOverWorld );
						  }

  // blank des for now
  // avoid flicker when response arrives
						  des = stringDuplicate( "" );
						  desToDelete = des;
					  }
				  }
			  }

			  char *stringUpper = stringToUpperCase( des );

			  if( desToDelete != NULL ) {
				  delete [] desToDelete;
			  }

			  stripDescriptionComment( stringUpper );


			  if( mCurrentDes == NULL ) {
				  mCurrentDes = stringDuplicate( stringUpper );
			  }
			  else {
				  if( strcmp( mCurrentDes, stringUpper ) != 0 ) {
  // adding a new one to stack, fade out old

					  for( int i=0; i<mOldDesStrings.size(); i++ ) {
						  float fade =
								  mOldDesFades.getElementDirect( i );

						  if( fade > 0.5 ) {
							  fade -= 0.20;
						  }
						  else {
							  fade -= 0.1;
						  }

						  *( mOldDesFades.getElement( i ) ) = fade;
						  if( fade <= 0 ) {
							  mOldDesStrings.deallocateStringElement( i );
							  mOldDesFades.deleteElement( i );
							  i--;
						  }
						  else if( strcmp( mCurrentDes,
								  mOldDesStrings.getElementDirect(i) )
								   == 0 ) {
  // already in stack, move to top
							  mOldDesStrings.deallocateStringElement( i );
							  mOldDesFades.deleteElement( i );
							  i--;
						  }
					  }

					  mOldDesStrings.push_back( mCurrentDes );
					  mOldDesFades.push_back( 1.0f );
					  mCurrentDes = stringDuplicate( stringUpper );
				  }
			  }


			  setDrawColor( 0, 0, 0, 1 );
			  pencilFont->drawString( stringUpper, pos, alignCenter );

			  delete [] stringUpper;
		  }
		  else {
			  if( mCurrentDes != NULL ) {
  // done with this des

  // move to top of stack
				  for( int i=0; i<mOldDesStrings.size(); i++ ) {
					  if( strcmp( mCurrentDes,
							  mOldDesStrings.getElementDirect(i) )
						  == 0 ) {
  // already in stack, move to top
						  mOldDesStrings.deallocateStringElement( i );
						  mOldDesFades.deleteElement( i );
						  i--;
					  }
				  }

				  mOldDesStrings.push_back( mCurrentDes );
				  mOldDesFades.push_back( 1.0f );
				  mCurrentDes = NULL;
			  }
		  }



		  if( this->feature.debugMessageEnabled ) {

			  setDrawColor( 1, 1, 1, 0.5 );

			  drawRect( lastScreenViewCenter, 612, 252 );


			  setDrawColor( 1, 1, 1, 0.5 );


			  setDrawColor( 0.2, 0.2, 0.2, 0.85  );

			  drawRect( lastScreenViewCenter, 600, 240 );

			  setDrawColor( 1, 1, 1, 1 );

			  doublePair messagePos = lastScreenViewCenter;

			  messagePos.y += 200;

			  switch( showBugMessage ) {
				  case 1:
					  drawMessage( "bugMessage1a", messagePos );
					  break;
				  case 2:
					  drawMessage( "bugMessage1b", messagePos );
					  break;
				  case 3:
					  drawMessage( "bugMessage1c", messagePos );
					  break;
			  }

			  messagePos = lastScreenViewCenter;


			  drawMessage( bugEmail, messagePos );

			  messagePos.y -= 200;
			  drawMessage( "bugMessage2", messagePos );
		  }
	  }


  // minitech
	  float worldMouseX, worldMouseY;
	  getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
	  screenToWorld( lastScreenMouseX,
			  lastScreenMouseY,
			  &worldMouseX,
			  &worldMouseY );
	  minitech::livingLifeDraw(worldMouseX, worldMouseY);

	  if( vogMode ) {
  // draw again, so we can see picker
		  PageComponent::base_draw( inViewCenter, inViewSize );
	  }
	  */
}