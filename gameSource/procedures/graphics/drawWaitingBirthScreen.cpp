//
// Created by olivier on 25/10/2021.
//

/*
 * @note LECACY: from LivingLifePage::draw(...)
 */

#include "screens.h"

#include <cstddef>
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "OneLife/gameSource/components/engines/GameSceneHandler.h"
#include "OneLife/gameSource/components/banks/spriteBank.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/dataTypes/uiComponent/screens.h"

void OneLife::game::graphic::drawWaitingBirthScreen(void* dataScreen)
{
	OneLife::dataType::uiComponent::SceneBuilder* screen = (OneLife::dataType::uiComponent::SceneBuilder*)dataScreen;
	if(!screen) return;

	setViewCenterPosition( screen->center.x, screen->center.y );

	setDrawColor( 0, 0, 0, 1 );
	drawSquare( screen->center, 100 );// draw this to cover up utility text field, but not waiting icon at top

	setDrawColor( 1, 1, 1, 1 );
	doublePair pos = { screen->component.connectingMessage.position.x, screen->component.connectingMessage.position.y };
	drawMessage( "connecting", pos, false, screen->component.connectingMessage.color.alpha);

	if(screen->component.serverAddressMessage.value)
	{
		setDrawColor( 1, 1, 1, 1 );
		drawMessage(screen->component.serverAddressMessage.value, screen->component.serverAddressMessage.position );
	}

	/*
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
*/
}

void OneLife::game::graphic::drawWaitingBirthScreen(//TODO: rename drawLoadingLocalMapScreen
		void* screen,
		OneLife::game::component::Socket* socket,
		doublePair lastScreenViewCenter,
		float connectionMessageFade,
		double frameRateFactor,
		float mFirstObjectSetLoadingProgress,
		char usingCustomServer,
		char* serverIP,
		int serverPort,
		char userReconnect,
		char mPlayerInFlight,
		char *userTwinCode,
		int userTwinCount,
		char mStartedLoadingFirstObjectSet)
{
	setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

	// draw this to cover up utility text field, but not
	// waiting icon at top
	setDrawColor( 0, 0, 0, 1 );
	drawSquare( lastScreenViewCenter, 100 );
	setDrawColor( 1, 1, 1, 1 );
	doublePair pos = { lastScreenViewCenter.x, lastScreenViewCenter.y };

	if( connectionMessageFade > 0 ) {

		if(socket->isConnected() )
		{
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



	if(!socket->isConnected() )
	{
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
}
