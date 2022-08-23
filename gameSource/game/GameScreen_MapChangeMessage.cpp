//
// Created by olivier on 23/08/2022.
//

int numLines;
char **lines = split( message, "\n", &numLines );

if( numLines > 0 ) {
// skip fist
delete [] lines[0];
}

char idBuffer[500];

for( int i=1; i<numLines; i++ ) {

int x, y, floorID, responsiblePlayerID;
int oldX, oldY;
float speed = 0;


int numRead = sscanf( lines[i], "%d %d %d %499s %d %d %d %f",
					  &x, &y, &floorID,
					  idBuffer, &responsiblePlayerID,
					  &oldX, &oldY, &speed );
if( numRead == 5 || numRead == 8) {

applyReceiveOffset( &x, &y );

int mapX = x - mMapOffsetX + mMapD / 2;
int mapY = y - mMapOffsetY + mMapD / 2;

if( mapX >= 0 && mapX < mMapD
&&
		mapY >= 0 && mapY < mMapD ) {

int mapI = mapY * mMapD + mapX;

int oldFloor = mMapFloors[ mapI ];

mMapFloors[ mapI ] = floorID;


int old = mMap[mapI];

int newID = -1;

int oldContainedCount =
		mMapContainedStacks[mapI].size();

if( responsiblePlayerID != -1 ) {
int rID = responsiblePlayerID;
if( rID < -1 ) {
rID = -rID;
}
LiveObject *rObj = getLiveObject( rID );

if( rObj != NULL &&
		rObj->pendingReceivedMessages.size() > 0 ) {


printf( "Holding MX message caused by "
"%d until later, "
"%d other messages pending for them\n",
rID,
rObj->pendingReceivedMessages.size() );

rObj->pendingReceivedMessages.push_back(
		autoSprintf( "MX\n%s\n#",
					 lines[i] ) );

delete [] lines[i];
continue;
}
}


if( strstr( idBuffer, "," ) != NULL ) {
int numInts;
char **ints =
		split( idBuffer, ",", &numInts );


newID = atoi( ints[0] );

mMap[mapI] = newID;

delete [] ints[0];

SimpleVector<int> oldContained;
// player triggered
// with no changed to container
// look for contained change
if( speed == 0 &&
old == newID &&
		responsiblePlayerID < 0 ) {

oldContained.push_back_other(
		&( mMapContainedStacks[mapI] ) );
}

mMapContainedStacks[mapI].deleteAll();
mMapSubContainedStacks[mapI].deleteAll();

int numContained = numInts - 1;

for( int c=0; c<numContained; c++ ) {

SimpleVector<int> newSubStack;

mMapSubContainedStacks[mapI].push_back(
		newSubStack );

mMapContainedStacks[mapI].push_back(
		atoi( ints[ c + 1 ] ) );

if( strstr( ints[c + 1], ":" ) != NULL ) {
// sub-container items

int numSubInts;
char **subInts =
		split( ints[c + 1], ":", &numSubInts );

delete [] subInts[0];
int numSubCont = numSubInts - 1;

SimpleVector<int> *subStack =
		mMapSubContainedStacks[mapI].
				getElement(c);

for( int s=0; s<numSubCont; s++ ) {
subStack->push_back(
		atoi( subInts[ s + 1 ] ) );
delete [] subInts[ s + 1 ];
}

delete [] subInts;
}

delete [] ints[ c + 1 ];
}
delete [] ints;

if( speed == 0 &&
old == newID &&
		responsiblePlayerID < 0
&&
oldContained.size() ==
mMapContainedStacks[mapI].size() ) {
// no change in number of items
// count number that change
int changeCount = 0;
int changeIndex = -1;
for( int i=0; i<oldContained.size(); i++ ) {
if( oldContained.
getElementDirect( i )
!=
mMapContainedStacks[mapI].
getElementDirect( i ) ) {
changeCount++;
changeIndex = i;
}
}
if( changeCount == 1 ) {
// single item changed
// play sound for it?

int oldContID =
		oldContained.
				getElementDirect( changeIndex );
int newContID =
		mMapContainedStacks[mapI].
				getElementDirect( changeIndex );


// watch out for swap case, with single
// item
// don't play sound then
LiveObject *causingPlayer =
		getLiveObject( - responsiblePlayerID );

if( causingPlayer != NULL &&
		causingPlayer->holdingID
!= oldContID ) {


ObjectRecord *newObj =
		getObject( newContID );

if( shouldCreationSoundPlay(
		oldContID, newContID ) ) {
if( newObj->
creationSound.numSubSounds
> 0 ) {

playSound(
		newObj->creationSound,
getVectorFromCamera(
		x, y ) );
}
}
else if(
causingPlayer != NULL &&
		causingPlayer->holdingID == 0 &&
bothSameUseParent( newContID,
		oldContID ) &&
newObj->
usingSound.numSubSounds > 0 ) {

ObjectRecord *oldObj =
		getObject( oldContID );

// only play sound if new is
// less used than old (filling back
// up sound)
if( getObjectParent( oldContID ) ==
newContID ||
oldObj->thisUseDummyIndex <
		newObj->thisUseDummyIndex ) {

playSound(
		newObj->usingSound,
getVectorFromCamera(
		x, y ) );
}
}
}
}
}
}
else {
// a single int
newID = atoi( idBuffer );
mMap[mapI] = newID;
mMapContainedStacks[mapI].deleteAll();
mMapSubContainedStacks[mapI].deleteAll();
}

if( speed > 0 ) {
// this cell moved from somewhere

applyReceiveOffset( &oldX, &oldY );


GridPos oldPos = { oldX, oldY };

// check if we have a move-to object "in the air"
// that is supposed to end up at this location
// if so, make it snap there
for( int i=0;
i<mMapExtraMovingObjects.size(); i++ ) {

if( equal(
		mMapExtraMovingObjectsDestWorldPos.
getElementDirect( i ),
		oldPos ) ) {
endExtraObjectMove( i );
break;
}
}



int oldMapI = getMapIndex( oldX, oldY );

int sourceObjID = 0;
if( oldMapI != -1 ) {
sourceObjID = mMap[ oldMapI ];

// check what move-trans for sourceID
// produces.  If it produces something
// show that moving instead

TransRecord *moveTrans =
		getTrans( -1, sourceObjID );

if( moveTrans != NULL &&
		moveTrans->move > 0 ) {
sourceObjID = moveTrans->newTarget;
}
}

ExtraMapObject oldObj;

if( old > 0 && sourceObjID != 0 &&
getTrans( sourceObjID, old ) != NULL ) {

// save old object while we
// set up new object
oldObj = copyFromMap( mapI );
oldObj.objectID = old;
if( old == -1 ) {
oldObj.objectID = 0;
}
}

mMapMoveSpeeds[mapI] = speed;



doublePair oldOffset = { 0, 0 };

char moveEastOrWest = false;

if( oldMapI != -1 ) {
// location where we came from
oldOffset = mMapMoveOffsets[ oldMapI ];

mMapAnimationFrameCount[ mapI ] =
mMapAnimationFrameCount[ oldMapI ];

mMapAnimationLastFrameCount[ mapI ] =
mMapAnimationLastFrameCount[ oldMapI ];


mMapAnimationFrozenRotFrameCount[ mapI ] =
mMapAnimationFrozenRotFrameCount[
oldMapI ];

mMapCurAnimType[ mapI ] =
mMapCurAnimType[ oldMapI ];
mMapLastAnimType[ mapI ] =
mMapLastAnimType[ oldMapI ];

mMapLastAnimFade[ mapI ] =
mMapLastAnimFade[ oldMapI ];

if( mMapLastAnimFade[ mapI ] == 0 &&
mMapCurAnimType[ mapI ] != moving ) {
// not in the middle of a fade

// fade to moving
mMapLastAnimType[ mapI ] =
mMapCurAnimType[ mapI ];
mMapCurAnimType[ mapI ] = moving;
mMapLastAnimFade[ mapI ] = 1;

if( mMapAnimationFrozenRotFrameCountUsed[
mapI ] ) {
mMapAnimationFrameCount[ mapI ] =
mMapAnimationFrozenRotFrameCount[
oldMapI ];
}
}

int oldLocID = mMap[ oldMapI ];

if( oldLocID > 0 ) {
TransRecord *decayTrans =
		getTrans( -1, oldLocID );

if( decayTrans != NULL ) {

if( decayTrans->move == 6 ||
decayTrans->move == 7 ) {
moveEastOrWest = true;
}
}
}
}

double oldTrueX = oldX + oldOffset.x;
double oldTrueY = oldY + oldOffset.y;

mMapMoveOffsets[mapI].x = oldTrueX - x;
mMapMoveOffsets[mapI].y = oldTrueY - y;

if( moveEastOrWest || x > oldTrueX ) {
mMapTileFlips[mapI] = false;
}
else if( x < oldTrueX ) {
mMapTileFlips[mapI] = true;
}


if( old > 0 && sourceObjID != 0 &&
getTrans( sourceObjID, old ) != NULL ) {

// now that we've set up new object in dest
// copy it
ExtraMapObject newObj = copyFromMap( mapI );
// leave source in place during move
newObj.objectID = sourceObjID;

// save it as an extra obj
mMapExtraMovingObjects.push_back( newObj );
GridPos worldDestPos = { x, y };
mMapExtraMovingObjectsDestWorldPos.push_back(
		worldDestPos );

mMapExtraMovingObjectsDestObjectIDs.push_back(
		newID );

// put old object back in place for now
putInMap( mapI, &oldObj );
}


if( old == 0 && sourceObjID != 0 ) {
// object moving into empty spot
// track where it came from as old
// so that we don't play initial
// creation sound by accident
old = sourceObjID;
}
}
else {
mMapMoveSpeeds[mapI] = 0;
mMapMoveOffsets[mapI].x = 0;
mMapMoveOffsets[mapI].y = 0;
}


TransRecord *nextDecayTrans = getTrans( -1, newID );

if( nextDecayTrans != NULL ) {

if( nextDecayTrans->move == 6 ||
nextDecayTrans->move == 7 ) {
// this object will move to left/right in future
// force no flip now
mMapTileFlips[mapI] = false;
}
}


if( oldFloor != floorID && floorID > 0 ) {
// floor changed

ObjectRecord *obj = getObject( floorID );
if( obj->creationSound.numSubSounds > 0 ) {

playSound( obj->creationSound,
getVectorFromCamera( x, y ) );
}
}


LiveObject *responsiblePlayerObject = NULL;

if( responsiblePlayerID > 0 ) {
responsiblePlayerObject =
getGameObject( responsiblePlayerID );
}

if( old > 0 &&
newID > 0 &&
old != newID &&
		responsiblePlayerID == - ourID ) {

// check for photo triggered
if( strstr( getObject( newID )->description,
"+photo" ) != NULL ) {

takingPhotoGlobalPos.x = x;
takingPhotoGlobalPos.y = y;
takingPhotoFlip = mMapTileFlips[ mapI ];
takingPhoto = true;
}

}


if( old > 0 &&
old == newID &&
		mMapContainedStacks[mapI].size() >
oldContainedCount &&
		responsiblePlayerObject != NULL &&
		responsiblePlayerObject->holdingID == 0 ) {

// target is changed container and
// responsible player's hands now empty


// first, try and play the "using"
// sound for the container
char soundPlayed = false;

SoundUsage contSound =
		getObject( mMap[mapI] )->usingSound;

if( contSound.numSubSounds > 0 ) {
playSound( contSound,
		getVectorFromCamera( x, y ) );
soundPlayed = true;
}

if( ! soundPlayed ) {
// no container using sound defined


// play player's using sound

SoundUsage s = getObject(
		responsiblePlayerObject->displayID )->
		usingSound;

if( s.numSubSounds > 0 ) {
playSound( s, getVectorFromCamera( x, y ) );
}
}
}



if( responsiblePlayerID == -1 ) {
// no one dropped this

// our placement status cleared
mMapPlayerPlacedFlags[mapI] = false;
}


// Check if a home marker has been set or removed
if( responsiblePlayerID != -1 &&
( old != 0 ||
newID != 0 ) ) {
// player-triggered change

int rID = responsiblePlayerID;
if( rID < -1 ) {
rID = -rID;
}

if( rID == ourID ) {
// local player triggered

char addedOrRemoved = false;

if( newID != 0 &&
getObject( newID )->homeMarker ) {

addHomeLocation( x, y );
addedOrRemoved = true;
}
else if( old != 0 &&
getObject( old )->homeMarker ) {
removeHomeLocation( x, y );
addedOrRemoved = true;
}

if( addedOrRemoved ) {
// look in region for our home locations
// that may have been removed by others
// (other people pulling up our stake)
// and clear them.  Our player can
// no longer clear them, because they
// don't exist on the map anymore

for( int ry=y-7; ry<=y+7; ry++ ) {
for( int rx=x-7; rx<=x+7; rx++ ) {

int mapRX =
		rx - mMapOffsetX + mMapD / 2;
int mapRY =
		ry - mMapOffsetY + mMapD / 2;

if( mapRX >= 0 && mapRX < mMapD
&&
		mapRY >= 0 && mapRY < mMapD ) {

int mapRI =
		mapRY * mMapD + mapRX;

int cellID = mMap[ mapRI ];

if( cellID == 0 ||
( cellID > 0 &&
! getObject( cellID )->
homeMarker ) ) {

removeHomeLocation(
		rx, ry );
}
}
}
}
}
}
}



if( old != newID &&
		newID != 0 &&
responsiblePlayerID <= -1 ) {
// ID change, and not just a player setting
// an object down

ObjectRecord *newObj = getObject( newID );


if( old > 0 ) {

if( responsiblePlayerID == -1 ) {
// object auto-decayed from some other
// object

// play decay sound
ObjectRecord *obj = getObject( old );
if( obj->decaySound.numSubSounds > 0 ) {

playSound(
		obj->decaySound,
getVectorFromCamera( x, y ) );
}
}
else if( responsiblePlayerID < -1 ) {
// player caused this object to change

// sound will be played elsewhere
}
}



if( newObj->creationSound.numSubSounds > 0 ) {

if( old == 0 && responsiblePlayerID < -1 ) {
// new placement, but not set-down
// we don't have information
// from on-ground objects to
// check for ancestor relationships

// check what the player is left
// holding instead

LiveObject *responsiblePlayerObject =
		getGameObject( -responsiblePlayerID );
if( responsiblePlayerObject != NULL ) {

old =
responsiblePlayerObject->holdingID;
if( old < 0 ) {
old = 0;
}

if( old > 0 ) {
TransRecord *p =
		getTransProducing( old,
						   newID );

if( p != NULL &&
		p->actor > 0 ) {
old = p->actor;
}
}
}
}


if( shouldCreationSoundPlay( old, newID ) ) {
playSound( newObj->creationSound,
getVectorFromCamera( x, y ) );
}
}
}



if( mMap[mapI] != 0 ) {
ObjectRecord *newObj = getObject( mMap[mapI] );

if( newObj->permanent && newObj->blocksWalking ) {
// clear the locally-stored flip for this
// tile
if( speed == 0 ) //allow blocking objects that move to flip e.g. beaver
mMapTileFlips[mapI] = false;
}
}

if( old != mMap[mapI] && mMap[mapI] != 0 ) {
// new placement

printf( "New placement, responsible=%d\n",
responsiblePlayerID );

if( mMapMoveSpeeds[mapI] == 0 ) {

if( old == 0 ) {
// set down into an empty spot
// reset frame count
mMapAnimationFrameCount[mapI] = 0;
mMapAnimationLastFrameCount[mapI] = 0;
}
else {
// else, leave existing frame count alone,
// since object has simply gone through a
// transition

// UNLESS it is a force-zero-start
// animation
AnimationRecord *newAnim =
		getAnimation( mMap[mapI], ground );

if( newAnim != NULL  &&
		newAnim->forceZeroStart ) {

mMapAnimationFrameCount[mapI] = 0;
mMapAnimationLastFrameCount[mapI] = 0;
}
}


mMapCurAnimType[mapI] = ground;
mMapLastAnimType[mapI] = ground;
mMapLastAnimFade[mapI] = 0;
mMapAnimationFrozenRotFrameCount[mapI] = 0;
}
else {
mMapLastAnimType[mapI] = mMapCurAnimType[mapI];
mMapCurAnimType[mapI] = moving;
if( mMapLastAnimType[mapI] != moving ) {
mMapLastAnimFade[mapI] = 1;
}
}


LiveObject *responsiblePlayerObject = NULL;


if( responsiblePlayerID > 0 ) {
responsiblePlayerObject =
getGameObject( responsiblePlayerID );

}
if( responsiblePlayerID != -1 &&
responsiblePlayerID != 0 &&
getObjectHeight( mMap[mapI] ) < CELL_D ) {

// try flagging objects as through-mousable
// for any change caused by a player,
// as long as object is small enough
mMapPlayerPlacedFlags[mapI] = true;
}
else if( getObjectHeight( mMap[mapI] ) >= CELL_D ) {
// object that has become tall enough
// that through-highlights feel strange
mMapPlayerPlacedFlags[mapI] = false;
}
// don't forget placement flags for objects
// that someone placed but have naturally changed,
// but remain short



if( responsiblePlayerObject == NULL ||
!responsiblePlayerObject->onScreen ) {

// set it down instantly, no drop animation
// (player's held offset isn't valid)
AnimationRecord *animR =
		getAnimation( mMap[mapI], ground );

if( animR != NULL &&
		animR->randomStartPhase ) {
mMapAnimationFrameCount[mapI] =
randSource.getRandomBoundedInt(
0, 10000 );
mMapAnimationLastFrameCount[i] =
mMapAnimationFrameCount[mapI];
}
mMapDropOffsets[mapI].x = 0;
mMapDropOffsets[mapI].y = 0;
mMapDropRot[mapI] = 0;
mMapDropSounds[mapI] = blankSoundUsage;

if( responsiblePlayerObject != NULL ) {
// copy their flip, even if off-screen
mMapTileFlips[mapI] =
responsiblePlayerObject->holdingFlip;
}
else if( responsiblePlayerID < -1 &&
							   old == 0 &&
							   mMap[ mapI ] > 0 &&
! getObject( mMap[ mapI ] )->
permanent ) {
// use-on-bare-ground
// with non-permanent result
// honor flip direction of player
mMapTileFlips[mapI] =
getLiveObject( -responsiblePlayerID )->
holdingFlip;
}
}
else {
// copy last frame count from last holder
// of this object (server tracks
// who was holding it and tell us about it)

mMapLastAnimType[mapI] = held;
mMapLastAnimFade[mapI] = 1;

mMapAnimationFrozenRotFrameCount[mapI] =
responsiblePlayerObject->
heldFrozenRotFrameCount;


if( responsiblePlayerObject->
lastHeldAnimFade == 0 ) {

mMapAnimationLastFrameCount[mapI] =
responsiblePlayerObject->
heldAnimationFrameCount;

mMapLastAnimType[mapI] =
responsiblePlayerObject->curHeldAnim;
}
else {
// dropped object was already
// in the middle of a fade
mMapCurAnimType[mapI] =
responsiblePlayerObject->curHeldAnim;

mMapAnimationFrameCount[mapI] =
responsiblePlayerObject->
heldAnimationFrameCount;

mMapLastAnimType[mapI] =
responsiblePlayerObject->lastHeldAnim;

mMapAnimationLastFrameCount[mapI] =
responsiblePlayerObject->
lastHeldAnimationFrameCount;


mMapLastAnimFade[mapI] =
responsiblePlayerObject->
lastHeldAnimFade;
}



mMapDropOffsets[mapI].x =
responsiblePlayerObject->
		heldObjectPos.x - x;

mMapDropOffsets[mapI].y =
responsiblePlayerObject->
		heldObjectPos.y - y;

mMapDropRot[mapI] =
responsiblePlayerObject->heldObjectRot;

mMapDropSounds[mapI] =
getObject(
		responsiblePlayerObject->displayID )->
usingSound;

mMapTileFlips[mapI] =
responsiblePlayerObject->holdingFlip;

if( responsiblePlayerObject->holdingID > 0 &&
old == 0 ) {
// use on bare ground transition

// don't use drop offset
mMapDropOffsets[mapI].x = 0;
mMapDropOffsets[mapI].y = 0;

mMapDropRot[mapI] = 0;
mMapDropSounds[mapI] =
blankSoundUsage;

if( getObject( mMap[ mapI ] )->
permanent ) {
// resulting in something
// permanent
// on ground.  Never flip it
mMapTileFlips[mapI] = false;
}
}



if( x >
responsiblePlayerObject->xServer ) {
responsiblePlayerObject->holdingFlip =
false;
}
else if( x <
		responsiblePlayerObject->xServer ) {
responsiblePlayerObject->holdingFlip =
true;
}
}
}
}
}

delete [] lines[i];
}

delete [] lines;
