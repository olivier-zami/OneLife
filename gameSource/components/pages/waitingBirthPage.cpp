//
// Created by olivier on 25/10/2021.
//

/*
 * @note LECACY: from LivingLifePage::draw(...)
 */

setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

if( getSpriteBankLoadFailure() != NULL || getSoundBankLoadFailure() != NULL )
{
setSignal( "loadFailure" );
}

// draw this to cover up utility text field, but not
// waiting icon at top
setDrawColor( 0, 0, 0, 1 );
drawSquare( lastScreenViewCenter, 100 );

setDrawColor( 1, 1, 1, 1 );
doublePair pos = { lastScreenViewCenter.x, lastScreenViewCenter.y };



if( connectionMessageFade > 0 ) {

if( serverSocketConnected ) {
connectionMessageFade -= 0.05 * frameRateFactor;

if( connectionMessageFade < 0 ) {
connectionMessageFade = 0;
}
}


doublePair conPos = pos;
conPos.y += 128;
drawMessage( "connecting", conPos, false, connectionMessageFade );
}


setDrawColor( 1, 1, 1, 1 );

if( usingCustomServer ) {
char *upperIP = stringToUpperCase( serverIP );

char *message = autoSprintf( translate( "customServerMesssage" ),
							 upperIP, serverPort );
delete [] upperIP;

doublePair custPos = pos;
custPos.y += 192;
drawMessage( message, custPos );

delete [] message;
}



if( ! serverSocketConnected ) {
// don't draw waiting message, not connected yet
if( userReconnect ) {
drawMessage( "waitingReconnect", pos );
}
}
else if( userReconnect ) {
drawMessage( "waitingReconnect", pos );
}
else if( mPlayerInFlight ) {
drawMessage( "waitingArrival", pos );
}
else if( userTwinCode == NULL ) {
drawMessage( "waitingBirth", pos );
}
else {
const char *sizeString = translate( "twins" );

if( userTwinCount == 3 ) {
sizeString = translate( "triplets" );
}
else if( userTwinCount == 4 ) {
sizeString = translate( "quadruplets" );
}
char *message = autoSprintf( translate( "waitingBirthFriends" ),
							 sizeString );

drawMessage( message, pos );
delete [] message;

if( !mStartedLoadingFirstObjectSet ) {
doublePair tipPos = pos;
tipPos.y -= 200;

drawMessage( translate( "cancelWaitingFriends" ), tipPos );
}
}


// hide map loading progress, because for now, it's almost
// instantaneous
if( false && mStartedLoadingFirstObjectSet ) {

pos.y -= 100;
drawMessage( "loadingMap", pos );

// border
setDrawColor( 1, 1, 1, 1 );

drawRect( pos.x - 100, pos.y - 120,
pos.x + 100, pos.y - 100 );

// inner black
setDrawColor( 0, 0, 0, 1 );

drawRect( pos.x - 98, pos.y - 118,
pos.x + 98, pos.y - 102 );


// progress
setDrawColor( .8, .8, .8, 1 );
drawRect( pos.x - 98, pos.y - 118,
pos.x - 98 + mFirstObjectSetLoadingProgress * ( 98 * 2 ),
pos.y - 102 );
}

return;
