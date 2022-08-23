//
// Created by olivier on 23/08/2022.
//


LiveObject *o = gameObjects.getElement( i );

if( o->currentSpeech != NULL ) {
if( game_getCurrentTime() > o->speechFadeETATime ) {

o->speechFade -= 0.05 * frameRateFactor;

if( o->speechFade <= 0 ) {
delete [] o->currentSpeech;
o->currentSpeech = NULL;
o->speechFade = 1.0;
o->speechIsSuccessfulCurse = false;
}
}
}


if( o->currentEmot != NULL ) {
if( game_getCurrentTime() > o->emotClearETATime ) {

// play decay sounds for this emot

if( !o->outOfRange ) {
for( int s=0; s<getEmotionNumObjectSlots(); s++ ) {

int id = getEmotionObjectByIndex( o->currentEmot, s );

if( id > 0 ) {
ObjectRecord *obj = getObject( id );

if( obj->decaySound.numSubSounds > 0 ) {

playSound(
		obj->decaySound,
getVectorFromCamera(
		o->currentPos.x,
o->currentPos.y ) );
}
}
}
}

o->currentEmot = NULL;
}
}


double animSpeed = o->lastSpeed;


if( o->holdingID > 0 ) {
ObjectRecord *heldObj = getObject( o->holdingID );

if( heldObj->speedMult > 1.0 ) {
// don't speed up animations just because movement
// speed has increased
// but DO slow animations down
animSpeed /= heldObj->speedMult;
}
}

double oldFrameCount = o->animationFrameCount;
o->animationFrameCount += animSpeed / BASE_SPEED;
o->lastAnimationFrameCount += animSpeed / BASE_SPEED;


if( o->curAnim == moving ) {
o->frozenRotFrameCount += animSpeed / BASE_SPEED;

}


char holdingRideable = false;

if( o->holdingID > 0 &&
getObject( o->holdingID )->rideable ) {
holdingRideable = true;
}


if( o->curAnim != moving || !holdingRideable ) {
// don't play player moving sound if riding something

AnimType t = o->curAnim;
doublePair pos = o->currentPos;

if( o->heldByAdultID != -1 ) {
t = held;


for( int j=0; j<gameObjects.size(); j++ ) {

LiveObject *parent = gameObjects.getElement( j );

if( parent->id == o->heldByAdultID ) {

pos = parent->currentPos;
}
}
}


handleAnimSound( o->displayID,
computeCurrentAge( o ),
		t,
		oldFrameCount, o->animationFrameCount,
pos.x,
pos.y );

if( o->currentEmot != NULL ) {
int numSlots = getEmotionNumObjectSlots();

for( int e=0; e<numSlots; e++ ) {
int oID =
		getEmotionObjectByIndex( o->currentEmot, e );

if( oID != 0 ) {

handleAnimSound( oID,
0,
t,
oldFrameCount, o->animationFrameCount,
pos.x,
pos.y );
}
}
}
}




if( o->lastAnimFade > 0 ) {

if( o->lastAnimFade == 1 ) {
// fade just started
// check if it's necessary

if( isAnimFadeNeeded( o->displayID,
o->lastAnim, o->curAnim ) ) {
// fade needed, do nothing
}
else {
// fade not needed
// jump to end of it
o->lastAnimFade = 0;
}
}


o->lastAnimFade -= 0.05 * frameRateFactor;
if( o->lastAnimFade < 0 ) {
o->lastAnimFade = 0;

if( o->futureAnimStack->size() > 0 ) {
// move on to next in stack

addNewAnimDirect(
		o,
		o->futureAnimStack->getElementDirect( 0 ) );

// pop from stack
o->futureAnimStack->deleteElement( 0 );
}

}
}

oldFrameCount = o->heldAnimationFrameCount;
o->heldAnimationFrameCount += animSpeed / BASE_SPEED;
o->lastHeldAnimationFrameCount += animSpeed / BASE_SPEED;

if( o->curHeldAnim == moving ) {
o->heldFrozenRotFrameCount += animSpeed / BASE_SPEED;
}

if( o->holdingID > 0 ) {
handleAnimSound( o->holdingID, 0, o->curHeldAnim,
oldFrameCount, o->heldAnimationFrameCount,
o->currentPos.x,
o->currentPos.y );
}


if( o->lastHeldAnimFade > 0 ) {

if( o->lastHeldAnimFade == 1 ) {
// fade just started
// check if it's necessary

int heldDisplayID = o->holdingID;


if( o->holdingID < 0 ) {
LiveObject *babyO = getGameObject( - o->holdingID );

if( babyO != NULL ) {
heldDisplayID = babyO->displayID;
}
else {
heldDisplayID = 0;
}
}


if( heldDisplayID > 0 &&
isAnimFadeNeeded( heldDisplayID,
		o->lastHeldAnim, o->curHeldAnim ) ) {
// fade needed, do nothing
}
else {
// fade not needed
// jump to end of it
o->lastHeldAnimFade = 0;
}
}


o->lastHeldAnimFade -= 0.05 * frameRateFactor;
if( o->lastHeldAnimFade < 0 ) {
o->lastHeldAnimFade = 0;

if( o->futureHeldAnimStack->size() > 0 ) {
// move on to next in stack

addNewHeldAnimDirect(
		o,
		o->futureHeldAnimStack->getElementDirect( 0 ) );

// pop from stack
o->futureHeldAnimStack->deleteElement( 0 );
}

}
}


if( o->currentSpeed != 0 && o->pathToDest != NULL ) {

GridPos curStepDest = o->pathToDest[ o->currentPathStep ];
GridPos nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];

doublePair startPos = { (double)curStepDest.x,
						(double)curStepDest.y };

doublePair endPos = { (double)nextStepDest.x,
					  (double)nextStepDest.y };

while( distance( o->currentPos, endPos ) <= o->currentSpeed &&
		o->currentPathStep < o->pathLength - 2 ) {

// speed too great, overshooting next step

o->currentPathStep ++;
o->numFramesOnCurrentStep = 0;

nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];

endPos.x = nextStepDest.x;
endPos.y = nextStepDest.y;
}


doublePair dir = normalize( sub( endPos, o->currentPos ) );


double turnFactor = 0.35;

if( o->currentPathStep == o->pathLength - 2 ) {
// last segment
// speed up turn toward final spot so that we
// don't miss it and circle around it forever
turnFactor = 0.5;
}

if( o->currentGridSpeed > 4 ) {
// tighten turns as speed increases to avoid
// circling around forever
turnFactor *= o->currentGridSpeed / 4;
}


if( dot( dir, o->currentMoveDirection ) >= 0 ) {
// a right angle turn or smaller


o->currentMoveDirection =
normalize( add( o->currentMoveDirection,
				mult( dir,
					  turnFactor * frameRateFactor ) ) );
}
else {
// a double-back in the path
// don't tot smooth turn through this, because
// it doesn't resolve
// instead, just turn sharply
o->currentMoveDirection = dir;
}

if( o->numFramesOnCurrentStep * o->currentSpeed  * frameRateFactor
> 2 ) {
// spent twice as much time reaching this tile as we should
// may be circling
// go directly there instead
o->currentMoveDirection = dir;
}

if( o->currentGridSpeed * frameRateFactor > 12 ) {
// at high speed, can't round turns at all without circling
o->currentMoveDirection = dir;
}




// don't change flip unless moving substantially in x
if( fabs( o->currentMoveDirection.x ) > 0.5 ) {
if( o->currentMoveDirection.x > 0 ) {
o->holdingFlip = false;
}
else {
o->holdingFlip = true;
}
}


if( o->currentPathStep < o->pathLength - 2 ) {

o->currentPos = add( o->currentPos,
					 mult( o->currentMoveDirection,
						   o->currentSpeed ) );

addNewAnim( o, moving );


if( pathStepDistFactor * distance( o->currentPos,
								   startPos )
>
distance( o->currentPos,
endPos ) ) {

o->currentPathStep ++;
o->numFramesOnCurrentStep = 0;
}
}
else {

if( o->id == ourID && mouseDown && shouldMoveCamera &&
		o->pathLength > 2 ) {

float worldMouseX, worldMouseY;

screenToWorld( lastScreenMouseX,
		lastScreenMouseY,
		&worldMouseX,
		&worldMouseY );

doublePair worldMouse = { worldMouseX, worldMouseY };

doublePair worldCurrent = mult( o->currentPos,
								CELL_D );
doublePair delta = sub( worldMouse, worldCurrent );

// if player started by clicking on nothing
// allow continued movement right away
// however, if they started by clicking on something
// make sure they are really holding the mouse down
// (give them time to unpress the mouse)
if( nextActionMessageToSend == NULL ||
mouseDownFrames >
minMouseDownFrames / frameRateFactor ) {

double absX = fabs( delta.x );
double absY = fabs( delta.y );


if( absX > CELL_D * 1
||
absY > CELL_D * 1 ) {

if( absX < CELL_D * 4
		   &&
		   absY < CELL_D * 4
		   &&
		   mouseDownFrames >
		minMouseDownFrames / frameRateFactor ) {

// they're holding mouse down very close
// to to their character

// throw mouse way out, further in the same
// direction

// we don't want to repeatedly find a bunch
// of short-path moves when mouse
// is held down

doublePair mouseVector =
		mult(
				normalize(
						sub( worldMouse, worldCurrent ) ),
				CELL_D * 4 );

doublePair fakeClick = add( worldCurrent,
							mouseVector );

o->useWaypoint = true;
// leave some wiggle room here
// path through waypoint might get extended
// if it involves obstacles
o->maxWaypointPathLength = 10;

o->waypointX = lrint( worldMouseX / CELL_D );
o->waypointY = lrint( worldMouseY / CELL_D );

pointerDown( fakeClick.x, fakeClick.y );

o->useWaypoint = false;
}
else {
pointerDown( worldMouseX, worldMouseY );
}
}
}
}
else if( o->id == ourID && o->pathLength >= 2 &&
nextActionMessageToSend == NULL &&
		distance( endPos, o->currentPos )
< o->currentSpeed ) {

// reached destination of bare-ground click

// check for auto-walk on road

GridPos prevStep = o->pathToDest[ o->pathLength - 2 ];
GridPos finalStep = o->pathToDest[ o->pathLength - 1 ];

int mapIP = getMapIndex( prevStep.x, prevStep.y );
int mapIF = getMapIndex( finalStep.x, finalStep.y );

if( mapIF != -1 && mapIP != -1 ) {
int floor = mMapFloors[ mapIF ];

if( floor > 0 && mMapFloors[ mapIP ] == floor &&
		getObject( floor )->rideable ) {

// rideable floor is a road!

int xDir = finalStep.x - prevStep.x;
int yDir = finalStep.y - prevStep.y;

GridPos nextStep = finalStep;
nextStep.x += xDir;
nextStep.y += yDir;

int len = 0;

if( isSameFloor( floor, finalStep, xDir, yDir ) ) {
// floor continues in same direction
// go as far as possible in that direction
// with next click
while( len < 5 && isSameFloor( floor, nextStep,
		xDir, yDir ) ) {
nextStep.x += xDir;
nextStep.y += yDir;
len ++;
}
}
else {
nextStep = finalStep;
char foundPerp = false;

// first step in same dir goes off floor
// try a perp move instead
if( xDir != 0 && yDir == 0 ) {
xDir = 0;
yDir = 1;

if( isSameFloor( floor, finalStep, xDir,
		yDir ) ) {
foundPerp = true;
}
else {
yDir = -1;
if( isSameFloor( floor, finalStep, xDir,
		yDir ) ) {
foundPerp = true;
}
}
}
else if( xDir == 0 && yDir != 0 ) {
xDir = 1;
yDir = 0;

if( isSameFloor( floor, finalStep, xDir,
		yDir ) ) {
foundPerp = true;
}
else {
xDir = -1;
if( isSameFloor( floor, finalStep, xDir,
		yDir ) ) {
foundPerp = true;
}
}
}

if( foundPerp ) {
nextStep.x += xDir;
nextStep.y += yDir;

while( len < 5 &&
isSameFloor( floor, nextStep,
		xDir, yDir ) ) {
nextStep.x += xDir;
nextStep.y += yDir;
len++;
}
}
}

if( ! equal( nextStep, finalStep ) ) {
// found straight-line continue of road
// auto-click there (but don't hold

// avoid clicks on self and objects
// when walking on road
mForceGroundClick = true;
pointerDown( nextStep.x * CELL_D,
		nextStep.y * CELL_D );

pointerUp( nextStep.x * CELL_D,
		nextStep.y * CELL_D );

mForceGroundClick = false;

endPos.x = (double)( nextStep.x );
endPos.y = (double)( nextStep.y );
}
}
}
}


if( distance( endPos, o->currentPos )
< o->currentSpeed ) {

// reached destination
o->currentPos = endPos;
o->currentSpeed = 0;
o->currentGridSpeed = 0;

playPendingReceivedMessages( o );

//trailColor.r = randSource.getRandomBoundedDouble( 0, .5 );
//trailColor.g = randSource.getRandomBoundedDouble( 0, .5 );
//trailColor.b = randSource.getRandomBoundedDouble( 0, .5 );


if( ( o->id != ourID &&
! o->somePendingMessageIsMoreMovement )
||
( o->id == ourID &&
		nextActionMessageToSend == NULL ) ) {

// simply stop walking
if( o->holdingID != 0 ) {
addNewAnim( o, ground2 );
}
else {
addNewAnim( o, ground );
}
}

printf( "Reached dest (%.0f,%.0f) %f seconds early\n",
endPos.x, endPos.y,
o->moveEtaTime - game_getCurrentTime() );
}
else {

addNewAnim( o, moving );

o->currentPos = add( o->currentPos,
					 mult( o->currentMoveDirection,
						   o->currentSpeed ) );

if( 1.5 * distance( o->currentPos,
startPos )
>
distance( o->currentPos,
endPos ) ) {

o->onFinalPathStep = true;
}
}
}

// correct move speed based on how far we have left to go
// and eta wall-clock time

// make this correction once per second
if( game_getCurrentTime() - o->timeOfLastSpeedUpdate
> .25 ) {

//updateMoveSpeed( o );
}

o->numFramesOnCurrentStep++;
}



double progressInc = 0.025 * frameRateFactor;

if( o->id == ourID &&
( o->pendingAction || o->pendingActionAnimationProgress != 0 ) ) {

o->pendingActionAnimationProgress += progressInc;

if( o->pendingActionAnimationProgress > 1 ) {
if( o->pendingAction ) {
// still pending, wrap around smoothly
o->pendingActionAnimationProgress -= 1;
}
else {
// no longer pending, finish last cycle by snapping
// back to 0
o->pendingActionAnimationProgress = 0;
o->actionTargetTweakX = 0;
o->actionTargetTweakY = 0;
}
}
}
else if( o->id != ourID && o->pendingActionAnimationProgress != 0 ) {

o->pendingActionAnimationProgress += progressInc;

if( o->pendingActionAnimationProgress > 1 ) {
// no longer pending, finish last cycle by snapping
// back to 0
o->pendingActionAnimationProgress = 0;
o->actionTargetTweakX = 0;
o->actionTargetTweakY = 0;
}
}