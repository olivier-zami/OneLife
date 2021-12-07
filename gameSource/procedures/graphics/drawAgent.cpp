//
// Created by olivier on 26/10/2021.
//

#include "drawAgent.h"

//#include <functionnal> require g++7 fot std::invoke
#include <cmath>
#include <cstddef>
#include "OneLife/gameSource/groundSprites.h"// CELL_D + GroundSpriteSet()
#include "OneLife/gameSource/objectBank.h"//ObjectRecord *getObject( int inID ) prepareToSkipSprites()
#include "OneLife/gameSource/animationBank.h"//TODO: put ObjectAnimPack in another file drawObjectAnim( )
#include "minorGems/game/gameGraphics.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/util/MinPriorityQueue.h"
//#include "OneLife/gameSource/controllers/LivingLifePage.h"//TODO: put LiveObject, DrawOrderRecord, ExtraMapObject in another file
#include "OneLife/gameSource/dataTypes/game.h"
#include "OneLife/gameSource/GridPos.h"
#include "OneLife/gameSource/dataTypes/animation.h"

void drawAgent(
	LivingLifePage* livingLifePage,
	PtrDrawMapCell drawMapCell,//void (*drawMapCell)( int inMapI, int inScreenX, int inScreenY, char inHighlightOnly, char inNoTimeEffects),
	PtrPutInMap putInMap,//void (*putInMap)( int inMapI, ExtraMapObject *inObj ),
	PtrCopyFromMap copyFromMap,//ExtraMapObject (*copyFromMap)( int inMapI ),
	PtrGetLiveObject getLiveObject,//LiveObject* (*getLiveObject)( int inID ),
	PtrDrawLiveObject drawLiveObject,
	int xStart,
	int xEnd,
	int yStart,
	int yEnd,
	int mMapOffsetX,
	int mMapOffsetY,
	int mapWidth,//mMapD from LivingLifePage
	int mapHeight,//mMapD from LivingLifePage
	char* cellDrawn,
	int *mMap,
	double *mMapMoveSpeeds,//livingLifePAge::mMap
	//doublePair cellPos,
	SimpleVector<LiveObject> gameObjects,
	int numMoving,
	int* movingObjectsIndices,
	doublePair *mMapMoveOffsets,
	SimpleVector<ExtraMapObject> mMapExtraMovingObjects,
	SimpleVector<LiveObject *>* speakers,
	SimpleVector<doublePair>* speakersPos,
	Font *mainFont,
	SimpleVector<GridPos> mMapExtraMovingObjectsDestWorldPos)
{
	int mMapD = mapWidth;

	for( int y=yEnd; y>=yStart; y-- )
	{

		int worldY = y + mMapOffsetY - mMapD / 2;

		int screenY = CELL_D * worldY;


// draw marked objects behind everything else, including players

		for( int x=xStart; x<=xEnd; x++ ) {

			int worldX = x + mMapOffsetX - mMapD / 2;


			int mapI = y * mMapD + x;

			if( cellDrawn[mapI] ) {
				continue;
			}

			int screenX = CELL_D * worldX;

			if( mMap[ mapI ] > 0 &&
				mMapMoveSpeeds[ mapI ] == 0 ) {

				ObjectRecord *o = getObject(mMap[mapI]);

				if( o->drawBehindPlayer ) {
					(livingLifePage->*drawMapCell)(mapI, screenX, screenY, false, false);//drawMapCell( mapI, screenX, screenY, false, false);
					cellDrawn[mapI] = true;
				}
				else if( o->anySpritesBehindPlayer ) {

// draw only behind layers now
					prepareToSkipSprites( o,
										  true );
					(livingLifePage->*drawMapCell)( mapI, screenX, screenY, false, true );//drawMapCell( mapI, screenX, screenY, false,true );// no time effects, because we'll draw // again later
					restoreSkipDrawing( o );
				}

			}


			if(false)//! debugging grid
			{

				doublePair cellPos = {(double) screenX, (double) screenY};

				if( mMap[ mapI ] < 0 ) {
					setDrawColor( 0, 0, 0, 0.5 );
				}
				else {
					if(
							lrint( abs(worldY)
							) % 2 == 0 ) {
						if(
								lrint( abs(worldX)
								) % 2 == 1 ) {
							setDrawColor( 1, 1, 1, 0.25 );
						}
						else {
							setDrawColor( 0, 0, 0, 0.25 );
						}
					}
					else {
						if(
								lrint( abs(worldX)
								) % 2 == 1 ) {
							setDrawColor( 0, 0, 0, 0.25 );
						}
						else {
							setDrawColor( 1, 1, 1, 0.25 );
						}
					}

				}

//doublePair cellPos = { (double)screenX, (double)screenY };
				drawSquare( cellPos, CELL_D
									 / 2 );

				FloatColor c = getDrawColor();

				c.
						r = 1 - c.r;
				c.
						g = 1 - c.g;
				c.
						b = 1 - c.b;
				c.
						a = .75;

				setDrawColor( c );

				char *xString = autoSprintf("x:%d", worldX);
				char *yString = autoSprintf("y:%d", worldY);

				doublePair xDrawPos = cellPos;
				doublePair yDrawPos = cellPos;

				xDrawPos.y += CELL_D / 6;
				yDrawPos.y -= CELL_D / 6;

				xDrawPos.x -= CELL_D / 3;
				yDrawPos.x -= CELL_D / 3;


				mainFont->
						drawString( xString, xDrawPos, alignLeft
				);
				mainFont->
						drawString( yString, yDrawPos, alignLeft
				);

				delete []
						yString;
				delete []
						xString;
			}
		}


		SimpleVector<ObjectAnimPack> heldToDrawOnTop;

// sorted queue of players and moving objects in this row
// build it, then draw them in sorted order
		MinPriorityQueue<DrawOrderRecord> drawQueue;

// draw players behind the objects in this row

// run this loop twice, once for
// adults, and then afterward for recently-dropped babies
// that are still sliding into place (so that they remain
// visibly on top of the adult who dropped them
		for( int d=0; d<2; d++ )
			for( int x=xStart; x<=xEnd; x++ ) {
				int worldX = x + mMapOffsetX - mMapD / 2;


				for( int i=0; i<gameObjects.size(); i++ ) {

					LiveObject *o = gameObjects.getElement( i );

					if( o->heldByAdultID != -1 ) {
// held by someone else, don't draw now
						continue;
					}

					if( d == 0 &&
						( o->heldByDropOffset.x != 0 ||
						  o->heldByDropOffset.y != 0 ) ) {
// recently dropped baby, skip
						continue;
					}
					else if( d == 1 &&
							 o->heldByDropOffset.x == 0 &&
							 o->heldByDropOffset.y == 0 ) {
// not a recently-dropped baby, skip
						continue;
					}


					int oX = o->xd;
					int oY = o->yd;

					if( o->currentSpeed != 0 ) {
						oX = lrint( o->currentPos.x );
						oY = lrint( o->currentPos.y - 0.20 );
					}


					if( oY == worldY && oX == worldX ) {

// there's a player here, insert into sort queue

						DrawOrderRecord drawRec;
						drawRec.person = true;
						drawRec.personO = o;


						double depth = 0 - o->currentPos.y;

						if( lrint( depth ) - depth == 0 ) {
// break ties (co-occupied cells) by drawing
// younger players in front
// (so that babies born appear in front of
//  their mothers)

// vary by a tiny amount, so we don't change
// the way they are sorted relative to other objects
							depth += ( 60.0 - o->age ) / 6000.0;
						}

						drawQueue.insert( drawRec, depth );
					}
				}
			}

// now sort moving objects that fall in this row
		for( int i=0; i<numMoving; i++ ) {

			int mapI = movingObjectsIndices[i];

			if( cellDrawn[mapI] ) {
				continue;
			}


			int oX = mapI % mMapD;
			int oY = mapI / mMapD;

			int movingX = lrint( oX + mMapMoveOffsets[mapI].x );

			double movingTrueCellY = oY + mMapMoveOffsets[mapI].y;

			double movingTrueY =  movingTrueCellY - 0.1;

			int movingCellY = lrint( movingTrueCellY - 0.40 );

			if( movingCellY == y && movingX >= xStart && movingX <= xEnd ) {

				int movingScreenX = CELL_D * ( oX + mMapOffsetX - mMapD / 2 );
				int movingScreenY = CELL_D * ( oY + mMapOffsetY - mMapD / 2 );

				double worldMovingY =  movingTrueY + mMapOffsetY - mMapD / 2;


//drawMapCell( mapI, movingScreenX, movingScreenY );

// add to depth sorting queue
				DrawOrderRecord drawRec;
				drawRec.person = false;
				drawRec.extraMovingObj = false;
				drawRec.mapI = mapI;
				drawRec.screenX = movingScreenX;
				drawRec.screenY = movingScreenY;
				drawQueue.insert( drawRec, 0 - worldMovingY );

				cellDrawn[mapI] = true;
			}
		}





// now sort extra moving objects that fall in this row

		int worldXStart = xStart + mMapOffsetX - mMapD / 2;
		int worldXEnd = xEnd + mMapOffsetX - mMapD / 2;

		for( int i=0; i<mMapExtraMovingObjects.size(); i++ ) {

			GridPos movingWorldPos =
					mMapExtraMovingObjectsDestWorldPos.getElementDirect( i );

			ExtraMapObject *extraO =
					mMapExtraMovingObjects.getElement( i );


			int movingX = lrint( movingWorldPos.x + extraO->moveOffset.x );

			double movingTrueCellY = movingWorldPos.y + extraO->moveOffset.y;

			double movingTrueY =  movingTrueCellY - 0.2;

			int movingCellY = lrint( movingTrueCellY - 0.50 );

			if( movingCellY == worldY &&
				movingX >= worldXStart &&
				movingX <= worldXEnd ) {

				int mapX = movingWorldPos.x - mMapOffsetX + mMapD / 2;
				int mapY = movingWorldPos.y - mMapOffsetY + mMapD / 2;

				int mapI = mapY * mMapD + mapX;


				int movingScreenX = CELL_D * movingWorldPos.x;
				int movingScreenY = CELL_D * movingWorldPos.y;

				double worldMovingY =  movingTrueY + mMapOffsetY - mMapD / 2;


//drawMapCell( mapI, movingScreenX, movingScreenY );

// add to depth sorting queue
				DrawOrderRecord drawRec;
				drawRec.person = false;
				drawRec.extraMovingObj = true;
				drawRec.extraMovingIndex = i;
				drawRec.mapI = mapI;
				drawRec.screenX = movingScreenX;
				drawRec.screenY = movingScreenY;

				drawQueue.insert( drawRec, 0 - worldMovingY );
			}
		}





// now move through queue in order, drawing
		int numQueued = drawQueue.size();
		for( int q=0; q<numQueued; q++ ) {
			DrawOrderRecord drawRec = drawQueue.removeMin();

			if( drawRec.person ) {
				LiveObject *o = drawRec.personO;

				ObjectAnimPack heldPack = (livingLifePage->*drawLiveObject)(o, speakers, speakersPos);//drawLiveObject( o, speakers, speakersPos );//legacy drawLiveObject( o, &speakers, &speakersPos );

				if( heldPack.inObjectID != -1 ) {
// holding something, not drawn yet

					if( o->holdingID < 0 ) {
						LiveObject *babyO = (livingLifePage->*getLiveObject)(- o->holdingID);//getLiveObject( - o->holdingID );
						if( babyO != NULL
							&& babyO->dying && babyO->holdingID > 0  ) {
// baby still holding something while dying,
// likely a wound
// add to pack to draw it on top of baby
							heldPack.additionalHeldID =
									babyO->holdingID;
						}
					}


					if( ! o->heldPosOverride ) {
// not sliding into place
// draw it now

						char skippingSome = false;
						if( heldPack.inObjectID > 0 &&
							getObject( heldPack.inObjectID )->rideable &&
							getObject( heldPack.inObjectID )->
									anySpritesBehindPlayer ) {
							skippingSome = true;
						}
						if( skippingSome ) {
							prepareToSkipSprites(
									getObject( heldPack.inObjectID ),
									false );
						}
						drawObjectAnim( heldPack );
						if( skippingSome ) {
							restoreSkipDrawing(
									getObject( heldPack.inObjectID ) );
						}
					}
					else {
						heldToDrawOnTop.push_back( heldPack );
					}
				}
			}
			else if( drawRec.extraMovingObj ) {
				ExtraMapObject *mO = mMapExtraMovingObjects.getElement(
						drawRec.extraMovingIndex );

// hold non-moving dest object
				ExtraMapObject curO = (livingLifePage->*copyFromMap)(drawRec.mapI);//copyFromMap( drawRec.mapI );

// temporarily insert extra object for drawing
				(livingLifePage->*putInMap)( drawRec.mapI, mO );//putInMap( drawRec.mapI, mO );

				(livingLifePage->*drawMapCell)( drawRec.mapI, drawRec.screenX, drawRec.screenY, false, false );//drawMapCell( drawRec.mapI, drawRec.screenX, drawRec.screenY, false, false );

// copy back out of map to preserve effects of draw call
// (frame count update, etc.)
				*mO = (livingLifePage->*copyFromMap)( drawRec.mapI );//copyFromMap( drawRec.mapI );

// put original one back
				(livingLifePage->*putInMap)(drawRec.mapI, &curO);//putInMap( drawRec.mapI, &curO );
			}
			else {
				(livingLifePage->*drawMapCell)( drawRec.mapI, drawRec.screenX, drawRec.screenY, false, false );//drawMapCell( drawRec.mapI, drawRec.screenX, drawRec.screenY, false, false );
			}
		}


// now draw non-behind-marked map objects in this row
// OVER the player objects in this row (so that pick up and set down
// looks correct, and so players are behind all same-row objects)

// we determine what counts as a wall through floorHugging

// first permanent, non-wall objects
		for( int x=xStart; x<=xEnd; x++ ) {
			int mapI = y * mMapD + x;

			if( cellDrawn[ mapI ] ) {
				continue;
			}

			int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


			if( mMap[ mapI ] > 0 ) {
				ObjectRecord *o = getObject( mMap[ mapI ] );

				if( ! o->drawBehindPlayer &&
					! o->floorHugging &&
					o->permanent &&
					mMapMoveSpeeds[ mapI ] == 0 ) {

					if( o->anySpritesBehindPlayer ) {
// draw only non-behind layers now
						prepareToSkipSprites( o, false );
					}

					(livingLifePage->*drawMapCell)( mapI, screenX, screenY, false, false );//drawMapCell( mapI, screenX, screenY, false, false );

					if( o->anySpritesBehindPlayer ) {
						restoreSkipDrawing( o );
					}

					cellDrawn[ mapI ] = true;
				}
			}
		}


// then non-permanent, non-wall objects
		for( int x=xStart; x<=xEnd; x++ ) {
			int mapI = y * mMapD + x;

			if( cellDrawn[ mapI ] ) {
				continue;
			}

			int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


			if( mMap[ mapI ] > 0 ) {
				ObjectRecord *o = getObject( mMap[ mapI ] );

				if( ! o->drawBehindPlayer &&
					! o->floorHugging &&
					! o->permanent &&
					mMapMoveSpeeds[ mapI ] == 0 ) {

					if( o->anySpritesBehindPlayer ) {
// draw only non-behind layers now
						prepareToSkipSprites( o, false );
					}

					(livingLifePage->*drawMapCell)( mapI, screenX, screenY, false, false );//drawMapCell( mapI, screenX, screenY, false, false );

					if( o->anySpritesBehindPlayer ) {
						restoreSkipDrawing( o );
					}

					cellDrawn[ mapI ] = true;
				}
			}
		}


// now draw held flying objects on top of objects in this row
// but still behind walls in this row
		for( int i=0; i<heldToDrawOnTop.size(); i++ ) {
			drawObjectAnim( heldToDrawOnTop.getElementDirect( i ) );
		}



// then permanent, non-container, wall objects
		for( int x=xStart; x<=xEnd; x++ ) {
			int mapI = y * mMapD + x;

			if( cellDrawn[ mapI ] ) {
				continue;
			}

			int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


			if( mMap[ mapI ] > 0 ) {
				ObjectRecord *o = getObject( mMap[ mapI ] );

				if( ! o->drawBehindPlayer &&
					o->floorHugging &&
					o->permanent &&
					o->numSlots == 0 &&
					mMapMoveSpeeds[ mapI ] == 0 ) {

					if( o->anySpritesBehindPlayer ) {
// draw only non-behind layers now
						prepareToSkipSprites( o, false );
					}

					(livingLifePage->*drawMapCell)( mapI, screenX, screenY, false, false );//drawMapCell( mapI, screenX, screenY, false, false );

					if( o->anySpritesBehindPlayer ) {
						restoreSkipDrawing( o );
					}

					cellDrawn[ mapI ] = true;
				}
			}
		}

// then permanent, container, wall objects (walls with signs)
		for( int x=xStart; x<=xEnd; x++ ) {
			int mapI = y * mMapD + x;

			if( cellDrawn[ mapI ] ) {
				continue;
			}

			int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


			if( mMap[ mapI ] > 0 ) {
				ObjectRecord *o = getObject( mMap[ mapI ] );

				if( ! o->drawBehindPlayer &&
					o->floorHugging &&
					o->permanent &&
					o->numSlots > 0 &&
					mMapMoveSpeeds[ mapI ] == 0 ) {

					if( o->anySpritesBehindPlayer ) {
// draw only non-behind layers now
						prepareToSkipSprites( o, false );
					}

					(livingLifePage->*drawMapCell)( mapI, screenX, screenY, false, false );//drawMapCell( mapI, screenX, screenY, false, false );

					if( o->anySpritesBehindPlayer ) {
						restoreSkipDrawing( o );
					}

					cellDrawn[ mapI ] = true;
				}
			}
		}
	} // end loop over rows on screen
}