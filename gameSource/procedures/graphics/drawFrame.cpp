//
// Created by olivier on 07/12/2021.
//

#include "drawFrame.h"

#include <string.h>
#include "OneLife/gameSource/components/engines/GameSceneHandler.h"
#include "OneLife/gameSource/components/engines/screenRenderer.h"
#include "OneLife/gameSource/musicPlayer.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/components/banks/spriteBank.h"
#include "OneLife/gameSource/photos.h"
#include "OneLife/gameSource/controllers/LivingLifePage.h"
#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/procedures/graphics/modalObjects/drawPausePanel.h"
#include "OneLife/gameSource/debug.h"

extern GamePage *currentGamePage;
extern LivingLifePage *livingLifePage;
extern int holdDeleteKeySteps;
extern int stepsBetweenDeleteRepeat;
extern double frameRateFactor;
extern float pauseScreenFade;
extern char firstDrawFrameCalled;
extern char upKey;
extern char leftKey;
extern char downKey;
extern char rightKey;
extern char *currentUserTypedMessage;

void drawFrame( char inUpdate )
{
	if( !inUpdate )//paused
	{

// because this is a networked game, we can't actually pause
		stepSpriteBank();

		stepSoundBank();

		stepMusicPlayer();

		if(currentGamePage != NULL )
		{
			currentGamePage->base_step();
		}
		wakeUpPauseFrameRate();

		drawFrameNoUpdate( true );//seem to be useless

		//drawPauseScreen();

// handle delete key repeat
		if( holdDeleteKeySteps > -1 )
		{
			holdDeleteKeySteps ++;

			if( holdDeleteKeySteps > stepsBetweenDeleteRepeat ) {
// delete repeat

// platform layer doesn't receive event for key held down
// tell it we are still active so that it doesn't
// reduce the framerate during long, held deletes
				wakeUpPauseFrameRate();

// subtract from messsage
				deleteCharFromUserTypedMessage();

// shorter delay for subsequent repeats
				stepsBetweenDeleteRepeat = (int)( 2/ frameRateFactor );
				holdDeleteKeySteps = 0;
			}
		}

// fade in pause screen
		if( pauseScreenFade < 1 ) {
			pauseScreenFade += ( 1.0 / 30 ) * frameRateFactor;

			if( pauseScreenFade > 1 ) {
				pauseScreenFade = 1;
			}
		}

// keep checking for this signal even if paused
		if(currentGamePage == livingLifePage && livingLifePage->checkSignal( "died" ) )
		{
			showDiedPage();
		}
		if(currentGamePage == livingLifePage && livingLifePage->checkSignal( "disconnect" ) )
		{
			showReconnectPage();
		}

		return;
	}

	// not paused
	// fade pause screen out
	if( pauseScreenFade > 0 )
	{
		pauseScreenFade -= ( 1.0 / 30 ) * frameRateFactor;

		if( pauseScreenFade < 0 )
		{
			pauseScreenFade = 0;

			if( currentUserTypedMessage != NULL ) {

				// make sure it doesn't already end with a file separator
				// (never insert two in a row, even when player closes
				//  pause screen without typing anything)
				int lengthCurrent = strlen( currentUserTypedMessage );

				if( lengthCurrent < 2 || currentUserTypedMessage[ lengthCurrent - 2 ] != 28 )
				{
					// insert at file separator (ascii 28)
					char *oldMessage = currentUserTypedMessage;
					currentUserTypedMessage = autoSprintf( "%s %c ",oldMessage,28 );
					delete [] oldMessage;
				}
			}
		}
	}

	if( !firstDrawFrameCalled ) {

		// do final init step... stuff that shouldn't be done until
		// we have control of screen

		char *moveKeyMapping = SettingsManager::getStringSetting( "upLeftDownRightKeys" );

		if( moveKeyMapping != NULL ) {
			char *temp = stringToLowerCase( moveKeyMapping );
			delete [] moveKeyMapping;
			moveKeyMapping = temp;

			if( strlen( moveKeyMapping ) == 4 &&
				strcmp( moveKeyMapping, "wasd" ) != 0 ) {
				// different assignment

				upKey = moveKeyMapping[0];
				leftKey = moveKeyMapping[1];
				downKey = moveKeyMapping[2];
				rightKey = moveKeyMapping[3];
			}
			delete [] moveKeyMapping;
		}




		firstDrawFrameCalled = true;
	}


	// updates here
	stepSpriteBank();

	stepSoundBank();

	stepMusicPlayer();

	stepPhotos();

	if( pauseScreenFade > 0 ) drawPauseScreen(); // draw tail end of pause screen, if it is still visible
}
