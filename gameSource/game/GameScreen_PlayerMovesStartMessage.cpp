//
// Created by olivier on 23/08/2022.
//


int numLines;
char **lines = split( message, "\n", &numLines );

if( numLines > 0 ) {
// skip fist
delete [] lines[0];
}


for( int i=1; i<numLines; i++ ) {

LiveObject o;

double etaSec;

int startX, startY;

int truncated;

int numRead = sscanf( lines[i], "%d %d %d %lf %lf %d",
					  &( o.id ),
					  &( startX ),
					  &( startY ),
					  &( o.moveTotalTime ),
					  &etaSec,
					  &truncated );

SimpleVector<char *> *tokens =
		tokenizeString( lines[i] );


applyReceiveOffset( &startX, &startY );


o.pathLength = 0;
o.pathToDest = NULL;

// require an even number at least 8
if( tokens->size() < 8 || tokens->size() % 2 != 0 ) {
}
else {
int numTokens = tokens->size();

o.pathLength = (numTokens - 6) / 2 + 1;

o.pathToDest = new GridPos[ o.pathLength ];

o.pathToDest[0].x = startX;
o.pathToDest[0].y = startY;

for( int e=1; e<o.pathLength; e++ ) {

char *xToken =
		tokens->getElementDirect( 6 + (e-1) * 2 );
char *yToken =
		tokens->getElementDirect( 6 + (e-1) * 2 + 1 );


sscanf( xToken, "%d", &( o.pathToDest[e].x ) );
sscanf( yToken, "%d", &( o.pathToDest[e].y ) );

// make them absolute
o.pathToDest[e].x += startX;
o.pathToDest[e].y += startY;
}

}

tokens->deallocateStringElements();
delete tokens;




o.moveEtaTime = etaSec + game_getCurrentTime();


if( numRead == 6 && o.pathLength > 0 ) {

o.xd = o.pathToDest[ o.pathLength -1 ].x;
o.yd = o.pathToDest[ o.pathLength -1 ].y;


for( int j=0; j<gameObjects.size(); j++ ) {
if( gameObjects.getElement(j)->id == o.id ) {

LiveObject *existing = gameObjects.getElement(j);

if( existing->
pendingReceivedMessages.size() > 0 ) {

// we've got older messages pending
// make this pending too

printf(
"Holding PM message for %d "
"until later, "
"%d other messages pending for them\n",
existing->id,
existing->pendingReceivedMessages.size() );

existing->pendingReceivedMessages.push_back(
		autoSprintf( "PM\n%s\n#",
					 lines[i] ) );
existing->somePendingMessageIsMoreMovement =
true;

break;
}

// actually playing the PM message
// that means nothing else is pending yet
existing->somePendingMessageIsMoreMovement = false;

// receiving a PM means they aren't out of
// range anymore
existing->outOfRange = false;



double timePassed =
		o.moveTotalTime - etaSec;

double fractionPassed =
		timePassed / o.moveTotalTime;

if( existing->id != ourID ) {
// stays in motion until we receive final
// PLAYER_UPDATE from server telling us
// that move is over

// don't do this for our local object
// we track our local inMotion status
existing->inMotion = true;
}

int oldPathLength = 0;
GridPos oldCurrentPathPos;
int oldCurrentPathIndex = -1;
SimpleVector<GridPos> oldPath;

if( existing->currentSpeed != 0
&&
existing->pathToDest != NULL ) {

// an interrupted move
oldPathLength = existing->pathLength;
oldCurrentPathPos =
existing->pathToDest[
		existing->currentPathStep ];

oldPath.appendArray( existing->pathToDest,
existing->pathLength );
oldCurrentPathIndex = existing->currentPathStep;
}


if( existing->id != ourID ) {
// remove any double-backs from path
// because they confuse smooth path following

removeDoubleBacksFromPath( &( o.pathToDest ),
&( o.pathLength ) );

}




if( existing->id != ourID ||
truncated ) {
// always replace path for other players
// with latest from server

// only replace OUR path if we
// learn that a path we submitted
// was truncated

existing->pathLength = o.pathLength;

if( existing->pathToDest != NULL ) {
delete [] existing->pathToDest;
}
existing->pathToDest =
new GridPos[ o.pathLength ];

memcpy( existing->pathToDest,
o.pathToDest,
sizeof( GridPos ) * o.pathLength );

existing->xd = o.xd;
existing->yd = o.yd;

existing->destTruncated = truncated;

if( existing->id != ourID ) {
// look at how far we think object is
// from current fractional position
// along path

int b =
		(int)floor(
				fractionPassed *
				( existing->pathLength - 1 ) );

if( b >= existing->pathLength ) {
b = existing->pathLength - 1;
}

// we may be getting a move for
// an object that has been off-chunk
// for a while and is now moving on-chunk

doublePair curr;
curr.x = existing->pathToDest[ b ].x;
curr.y = existing->pathToDest[ b ].y;

if( distance( curr,
		existing->currentPos ) >
5 ) {

// 5 is too far

// jump right to current loc
// on new path
existing->currentPos = curr;
}
}

}



if( existing->id != ourID ) {
// don't force-update these
// for our object
// we control it locally, to keep
// illusion of full move interactivity

char usingOldPathStep = false;
char appendingLeadPath = false;

if( oldPathLength != 0 ) {
// this move interrupts or truncates
// the move we were already on

// look to see if the old path step
// is on our new path
char found = false;
int foundStep = -1;

for( int p=0;
p<existing->pathLength - 1;
p++ ) {

if( equal( existing->pathToDest[p],
oldCurrentPathPos ) ) {

found = true;
foundStep = p;
}
}

if( found ) {
usingOldPathStep = true;

existing->currentPathStep =
foundStep;

doublePair foundWorld =
		gridToDouble(
				existing->
						pathToDest[ foundStep ] );

// where we should move toward
doublePair nextWorld;

if( foundStep <
		existing->pathLength - 1 ) {

// point from here to our
// next step
nextWorld =
gridToDouble(
		existing->
				pathToDest[
				foundStep + 1 ] );
}
else {
// at end of path, point right
// to it
nextWorld = foundWorld;
}

existing->currentMoveDirection =
normalize(
		sub( nextWorld,
			 existing->currentPos ) );
}
else {
// other case
// check if new start on old path

// maybe this new path branches
// off old path before or after
// where we are

printf( "    CUR PATH:  " );
printPath( oldPath.getElementArray(),
		oldPathLength );
printf( "    WE AT:  %d (%d,%d)  \n",
oldCurrentPathIndex,
oldCurrentPathPos.x,
oldCurrentPathPos.y );

int foundStartIndex = -1;

for( int i=0; i<oldPathLength; i++ ) {
GridPos p =
		oldPath.getElementDirect( i );

if( p.x == startX &&
		p.y == startY ) {

foundStartIndex = i;
break;
}
}

if( foundStartIndex != -1 ) {

int step = 1;

if( foundStartIndex >
oldCurrentPathIndex ) {
step = 1;
}
else if( foundStartIndex <
		oldCurrentPathIndex ) {
step = -1;
}
appendingLeadPath = true;

SimpleVector<GridPos> newPath;

for( int i=oldCurrentPathIndex;
i != foundStartIndex;
i += step ) {

newPath.push_back(
		oldPath.
getElementDirect( i ) );
}

for( int i=0;
i<existing->pathLength;
i++ ) {
// now add rest of new path
newPath.push_back(
		existing->pathToDest[i] );
}

printf( "    OLD PATH:  " );
printPath( existing->pathToDest,
existing->pathLength );


// now replace path
// with new, lead-appended path
existing->pathLength =
newPath.size();

delete [] existing->pathToDest;

existing->pathToDest =
newPath.getElementArray();
existing->currentPathStep = 0;
existing->numFramesOnCurrentStep
= 0;


printf( "    NEW PATH:  " );
printPath( existing->pathToDest,
existing->pathLength );

removeDoubleBacksFromPath(
		&( existing->pathToDest ),
&( existing->pathLength ) );

printf(
"    NEW PATH (DB free):  " );
printPath( existing->pathToDest,
existing->pathLength );

if( existing->pathLength == 1 ) {
fixSingleStepPath( existing );
}


doublePair nextWorld =
		gridToDouble(
				existing->pathToDest[1] );

// point toward next path pos
existing->currentMoveDirection =
normalize(
		sub( nextWorld,
			 existing->
					 currentPos ) );
}
}

}


if( ! usingOldPathStep &&
! appendingLeadPath ) {

// we don't have enough info
// to patch path

// change to walking toward next
// path step from wherever we are
// but DON'T jump existing obj's
// possition suddenly

printf( "Manually forced\n" );

// find closest spot along path
// to our current pos
double minDist = DBL_MAX;

// prev step
int b = -1;

for( int testB=0;
testB < existing->pathLength - 1;
testB ++ ) {

doublePair worldPos = gridToDouble(
		existing->pathToDest[testB] );

double thisDist =
		distance( worldPos,
				  existing->currentPos );
if( thisDist < minDist ) {
b = testB;
minDist = thisDist;
}
}


// next step
int n = b + 1;

existing->currentPathStep = b;

doublePair nWorld =
		gridToDouble(
				existing->pathToDest[n] );

// point toward next path pos
existing->currentMoveDirection =
normalize(
		sub( nWorld,
			 existing->currentPos ) );

}


existing->moveTotalTime = o.moveTotalTime;
existing->moveEtaTime = o.moveEtaTime;

if( usingOldPathStep ) {
// we're ignoring where server
// says we should be

// BUT, we should change speed to
// compensate for the difference

double oldFractionPassed =
		measurePathLength(
				existing,
				existing->currentPathStep + 1 )
		/
		measurePathLength(
				existing,
				existing->pathLength );


// if this is positive, we are
// farther along than we should be
// we need to slow down (moveEtaTime
// should get bigger)

// if negative, we are behind
// we should speed up (moveEtaTime
// should get smaller)
double fractionDiff =
		oldFractionPassed - fractionPassed;

double timeAdjust =
		existing->moveTotalTime * fractionDiff;

if( fractionDiff < 0 ) {
// only speed up...
// never slow down, because
// it's always okay if we show
// player arriving early

existing->moveEtaTime += timeAdjust;
}
}


updateMoveSpeed( existing );
}
else if( truncated ) {
// adjustment to our own movement

// cancel pending action upon arrival
// (no longer possible, since truncated)

existing->pendingActionAnimationProgress = 0;
existing->pendingAction = false;

playerActionPending = false;
waitingForPong = false;
playerActionTargetNotAdjacent = false;

if( nextActionMessageToSend != NULL ) {
delete [] nextActionMessageToSend;
nextActionMessageToSend = NULL;
}

// this path may be different
// from what we actually requested
// from sever

char stillOnPath = false;

if( oldPathLength > 0 ) {
// on a path, perhaps some other one

// check if our current pos
// is on this new, truncated path

char found = false;
int foundStep = -1;
for( int p=0;
p<existing->pathLength - 1;
p++ ) {

if( equal( existing->pathToDest[p],
oldCurrentPathPos ) ) {

found = true;
foundStep = p;
}
}

if( found ) {
stillOnPath = true;

existing->currentPathStep =
foundStep;
}

}



// only jump around if we must

if( ! stillOnPath ) {
// path truncation from what we last knew
// for ourselves, and we're off the end
// of the new path

// hard jump back
existing->currentSpeed = 0;
existing->currentGridSpeed = 0;

playPendingReceivedMessages( existing );

existing->currentPos.x =
existing->xd;
existing->currentPos.y =
existing->yd;
}
}





break;
}
}
}

if( o.pathToDest != NULL ) {
delete [] o.pathToDest;
}

delete [] lines[i];
}


delete [] lines;

