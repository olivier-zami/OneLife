int versionNumber = 256;
int dataVersionNumber = 0;

int binVersionNumber = versionNumber;


// NOTE that OneLife doesn't use account hmacs

// retain an older version number here if server is compatible
// with older client versions.
// Change this number (and number on server) if server has changed
// in a way that breaks old clients.
int accountHmacVersionNumber = 0;



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

//#define USE_MALLINFO

#ifdef USE_MALLINFO
#include <malloc.h>
#endif

#include "minorGems/graphics/Color.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/CustomRandomSource.h"
#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"
#include "minorGems/crypto/hashes/sha1.h"

// static seed
extern CustomRandomSource randSource;

#include "minorGems/util/log/AppLog.h"
#include "OneLife/gameSource/misc.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"
#include "OneLife/gameSource/components/banks/spriteBank.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/categoryBank.h"
#include "OneLife/gameSource/transitionBank.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/liveObjectSet.h"
#include "OneLife/gameSource/groundSprites.h"
#include "OneLife/gameSource/emotion.h"
#include "OneLife/gameSource/photos.h"
#include "OneLife/gameSource/lifeTokens.h"
#include "OneLife/gameSource/fitnessScore.h"
#include "OneLife/gameSource/components/pages/AutoUpdatePage.h"
#include "OneLife/gameSource/components/pages/ExistingAccountPage.h"
#include "OneLife/gameSource/components/pages/ExtendedMessagePage.h"
#include "OneLife/gameSource/components/pages/FinalMessagePage.h"
#include "OneLife/gameSource/components/pages/GeneticHistoryPage.h"
#include "OneLife/gameSource/components/pages/LivingLifePage.h"
#include "OneLife/gameSource/components/pages/LoadingPage.h"
#include "OneLife/gameSource/components/pages/PollPage.h"
#include "OneLife/gameSource/components/pages/RebirthChoicePage.h"
#include "OneLife/gameSource/components/pages/ReviewPage.h"
#include "OneLife/gameSource/components/pages/ServerActionPage.h"
#include "OneLife/gameSource/components/pages/SettingsPage.h"
#include "OneLife/gameSource/components/pages/TwinPage.h"
//#include "TestPage.h"
#include "OneLife/gameSource/ageControl.h"
#include "OneLife/gameSource/musicPlayer.h"
#include "OneLife/gameSource/whiteSprites.h"
#include "OneLife/gameSource/procedures/graphics/sprites/drawMessage.h"
#include "OneLife/gameSource/components/engines/audioRenderer.h"
#include "OneLife/gameSource/components/engines/screenRenderer.h"
#include "OneLife/gameSource/settings.h"
#include "OneLife/gameSource/misc.h"
#include "OneLife/gameSource/components/engines/GameSceneHandler.h"

extern double viewWidth;
extern double viewHeight;
extern int musicOff;
extern int gui_hud_mode;
extern float gui_fov_scale;
extern float gui_fov_preferred_max_scale;
extern float gui_fov_scale_hud;
extern float gui_fov_target_scale_hud;
extern int gui_fov_offset_x;
extern int gui_fov_offset_y;
extern float gui_fov_scale_hud;

// should we pull the map
char autoLogIn = 0;
// start at reflector URL
char *reflectorURL = NULL;
char *userEmail = NULL;
char *accountKey = NULL;
char *userTwinCode = NULL;
int userTwinCount = 0;
char userReconnect = false;
// these are needed by ServerActionPage, but we don't use them
int userID = -1;
int serverSequenceNumber = 0;
FinalMessagePage *finalMessagePage;
ServerActionPage *getServerAddressPage;
LoadingPage *loadingPage;
AutoUpdatePage *autoUpdatePage;
LivingLifePage *livingLifePage;
ExistingAccountPage *existingAccountPage;
ExtendedMessagePage *extendedMessagePage;
RebirthChoicePage *rebirthChoicePage;
SettingsPage *settingsPage;
ReviewPage *reviewPage;
TwinPage *twinPage;
PollPage *pollPage;
GeneticHistoryPage *geneticHistoryPage;
GamePage *currentGamePage = NULL;//TestPage *testPage = NULL;
int loadingStepBatchSize = 1;
int numLoadingSteps = 20;
SpriteHandle instructionsSprite;



// position of view in world
doublePair lastScreenViewCenter = {0, 0 };

// this is the desired visible width
// if our screen is wider than this (wider than 16:9 aspect ratio)
// then we will put letterbox bars on the sides
// Usually, if screen is not 16:9, it will be taller, not wider,
// and we will put letterbox bars on the top and bottom 
double visibleViewWidth = viewWidth;

void setFOVScale() {

	gui_hud_mode = SettingsManager::getIntSetting( "hudDrawMode", 0 );
	if( gui_hud_mode < 0 ) gui_hud_mode = 0;
	else if( gui_hud_mode > 2 ) gui_hud_mode = 2;
	SettingsManager::setSetting( "hudDrawMode", gui_hud_mode );

    gui_fov_scale = SettingsManager::getFloatSetting( "fovDefault", 1.25f );
    if( ! gui_fov_scale || gui_fov_scale < 1 )
		gui_fov_scale = 1.0f;
    else if ( gui_fov_scale > 6 )
		gui_fov_scale = 6.0f;
	SettingsManager::setSetting( "fovDefault", gui_fov_scale );
	SettingsManager::setSetting( "fovScale", gui_fov_scale );

    gui_fov_preferred_max_scale = SettingsManager::getFloatSetting( "fovMax", 2.25f );
    if( ! gui_fov_preferred_max_scale || gui_fov_preferred_max_scale < 1 )
		gui_fov_preferred_max_scale = 1.0f;
	else if ( gui_fov_preferred_max_scale > 6 )
		gui_fov_preferred_max_scale = 6.0f;
	SettingsManager::setSetting( "fovMax", gui_fov_preferred_max_scale );

	gui_fov_scale_hud = gui_fov_scale / gui_fov_target_scale_hud;
    gui_fov_offset_x = (int)(((1280 * gui_fov_target_scale_hud) - 1280)/2);
    gui_fov_offset_y = (int)(((720 * gui_fov_target_scale_hud) - 720)/2);
    viewWidth = 1280 * gui_fov_scale;
    viewHeight = 720 * gui_fov_scale;
    visibleViewWidth = viewWidth;
}

// Used in hue shifting objects, animaionts and ground sprites
// when the character is tripping
bool isTrippingEffectOn = false;
// fraction of viewWidth visible vertically (aspect ratio)
double viewHeightFraction;
int screenW, screenH;
char initDone = false;
float mouseSpeed;
int maxSimultaneousExpectedSoundEffects = 10;

// fraction of full volume devoted to music
// Note that musicLoudness and soundEffectLoudness settings still
// effect absolute loudness of each, beyond this setting
// this setting is used to trim music volume relative to sound effects
// if both are at full volume

// 1.0 makes it as loud as the sound effect mix
// on the other hand, it's stereo, compressed, full-frequency etc.
// so it's subjectively louder
double musicHeadroom = 1.0;
float musicLoudness;
int webRetrySeconds;
double frameRateFactor = 1;
int baseFramesPerSecond = 60;
int targetFramesPerSecond = baseFramesPerSecond;
int firstServerMessagesReceived = 0;

char shouldNativeScreenResolutionBeUsed() {
    return true;
    }

char isNonIntegerScalingAllowed() {
    return true;
    }

const char *getWindowTitle() {
    return "OneLife";
    }

const char *getAppName() {
    return "OneLife";
    }

int getAppVersion() {
    return versionNumber;
    }

const char *getLinuxAppName() {
    // no dir-name conflict here because we're using all caps for app name
    return "OneLifeApp";
    }

const char *getFontTGAFileName() {
    return "font_32_64.tga";
    }

char isDemoMode() {
    return false;
    }

const char *getDemoCodeSharedSecret() {
    return "fundamental_right";
    }

const char *getDemoCodeServerURL() {
    return "http://FIXME/demoServer/server.php";
    }

char gamePlayingBack = false;
Font *mainFont;
Font *mainFontFixed;
// closer spacing
Font *mainFontReview;
Font *numbersFontFixed;
Font *handwritingFont;
Font *pencilFont;
Font *pencilErasedFont;
Font *smallFont;
Font *titleFont;
float pauseScreenFade = 0;
char *currentUserTypedMessage = NULL;
// for delete key repeat during message typing
int holdDeleteKeySteps = -1;
int stepsBetweenDeleteRepeat;

static void updateDataVersionNumber() {
    File file( NULL, "dataVersionNumber.txt" );
    
    if( file.exists() ) {
        char *contents = file.readFileContents();
        
        if( contents != NULL ) {
            sscanf( contents, "%d", &dataVersionNumber );
        
            delete [] contents;

            if( dataVersionNumber > versionNumber ) {
                versionNumber = dataVersionNumber;
                }
            }
        }
    }

#define SETTINGS_HASH_SALT "another_loss"

static const char *customDataFormatWriteString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";

static const char *customDataFormatReadString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";

char *getCustomRecordedGameData() {    
    
    updateDataVersionNumber();

    float mouseSpeedSetting = 
        SettingsManager::getFloatSetting( "mouseSpeed", 1.0f );
    int musicOffSetting = 
        SettingsManager::getIntSetting( "musicOff", 0 );
    float musicLoudnessSetting = 
        SettingsManager::getFloatSetting( "musicLoudness", 1.0f );
    
    int webRetrySecondsSetting = 
        SettingsManager::getIntSetting( "webRetrySeconds", 10 );
    

    char * result = autoSprintf(
        customDataFormatWriteString,
        versionNumber, mouseSpeedSetting, musicOffSetting, 
        musicLoudnessSetting,
        webRetrySecondsSetting );
    

    return result;
    }

char showMouseDuringPlayback() {
    // since we rely on the system mouse pointer during the game (and don't
    // draw our own pointer), we need to see the recorded pointer position
    // to make sense of game playback
    return true;
    }

char *getHashSalt() {
    return stringDuplicate( SETTINGS_HASH_SALT );
    }

void initDrawString( int inWidth, int inHeight ) {

	setFOVScale();

    toggleLinearMagFilter( true );
    toggleMipMapGeneration( true );
    toggleMipMapMinFilter( true );
    toggleTransparentCropping( true );
    
    mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16 );
    mainFont->setMinimumPositionPrecision( 1 );

    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;
    
    if( viewHeightFraction < 9.0 / 16.0 ) {
        // weird, wider than 16:9 aspect ratio
        
        viewWidth = viewHeight / viewHeightFraction;
        }
    

    setViewSize( viewWidth );
    setLetterbox( visibleViewWidth, viewHeight );
    }

void freeDrawString() {
    delete mainFont;
    }

void initFrameDrawer( int inWidth, int inHeight, int inTargetFrameRate,
					  const char *inCustomRecordedGameData,
					  char inPlayingBack ) {

	// it's always safe to call this, just in case we're launching post-update
	postUpdate();


	instructionsSprite = loadWhiteSprite( "instructions.tga" );



	initAgeControl();

	updateDataVersionNumber();


	AppLog::printOutNextMessage();
	AppLog::infoF( "OneLife client v%d (binV=%d, dataV=%d) starting up",
			versionNumber, binVersionNumber, dataVersionNumber );


	toggleLinearMagFilter( true );
	toggleMipMapGeneration( true );
	toggleMipMapMinFilter( true );
	toggleTransparentCropping( true );

	gamePlayingBack = inPlayingBack;

	screenW = inWidth;
	screenH = inHeight;

	if( inTargetFrameRate != baseFramesPerSecond ) {
		frameRateFactor = (double)baseFramesPerSecond / (double)inTargetFrameRate;
		numLoadingSteps /= frameRateFactor;
	}

	targetFramesPerSecond = inTargetFrameRate;




	setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

	viewHeightFraction = inHeight / (double)inWidth;


	if( viewHeightFraction < 9.0 / 16.0 ) {
		// weird, wider than 16:9 aspect ratio

		viewWidth = viewHeight / viewHeightFraction;
	}

	setViewSize( viewWidth );
	setLetterbox( visibleViewWidth, viewHeight );
	setCursorVisible( true );

	/******************************************************************************************************************/
	//!set default screen
	grabInput( false );
	/******************************************************************************************************************/

	// world coordinates
	setMouseReportingMode( true );



	mainFontReview = new Font( getFontTGAFileName(), 4, 8, false, 16 );
	mainFontReview->setMinimumPositionPrecision( 1 );

	mainFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16 );
	numbersFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16, 16 );

	mainFontFixed->setMinimumPositionPrecision( 1 );
	numbersFontFixed->setMinimumPositionPrecision( 1 );

	smallFont = new Font( getFontTGAFileName(), 3, 8, false, 8 * gui_fov_scale_hud );

	titleFont =
			new Font( "font_handwriting_32_32.tga", 3, 6, false, 20 * gui_fov_scale_hud );

	handwritingFont =
			new Font( "font_handwriting_32_32.tga", 3, 6, false, 16 * gui_fov_scale_hud );

	handwritingFont->setMinimumPositionPrecision( 1 );

	pencilFont =
			new Font( "font_pencil_32_32.tga", 3, 6, false, 16 * gui_fov_scale_hud );

	pencilFont->setMinimumPositionPrecision( 1 );

	pencilErasedFont =
			new Font( "font_pencil_erased_32_32.tga", 3, 6, false, 16 * gui_fov_scale_hud );

	pencilErasedFont->setMinimumPositionPrecision( 1 );

	pencilErasedFont->copySpacing( pencilFont );


	float mouseSpeedSetting = 1.0f;

	int musicOffSetting = 0;
	float musicLoudnessSetting = 1.0f;

	int webRetrySecondsSetting = 10;


	int readVersionNumber;

	int numRead = sscanf( inCustomRecordedGameData,
			customDataFormatReadString,
			&readVersionNumber,
			&mouseSpeedSetting,
			&musicOffSetting,
			&musicLoudnessSetting,
			&webRetrySecondsSetting );
	if( numRead != 6 ) {
		// no recorded game?
	}
	else {

		if( readVersionNumber != versionNumber ) {
			AppLog::printOutNextMessage();
			AppLog::warningF(
					"WARNING:  version number in playback file is %d "
					"but game version is %d...",
					readVersionNumber, versionNumber );
		}
	}


	userEmail = SettingsManager::getStringSetting( "email" );
	accountKey = SettingsManager::getStringSetting( "accountKey" );


	double mouseParam = 0.000976562;

	mouseParam *= mouseSpeedSetting;

	mouseSpeed = mouseParam * inWidth / viewWidth;

	musicOff = musicOffSetting;
	musicLoudness = musicLoudnessSetting;


	webRetrySeconds = webRetrySecondsSetting;

	reflectorURL = SettingsManager::getStringSetting( "reflectorURL" );

	if( reflectorURL == NULL ) {
		reflectorURL =
				stringDuplicate(
						"http://localhost/jcr13/oneLifeReflector/server.php" );
	}



	setSoundLoudness( 1.0 );
	setSoundPlaying( true );




	const char *resultNamesA[4] = { "serverIP", "serverPort",
									"requiredVersionNumber",
									"autoUpdateURL" };

	getServerAddressPage = new ServerActionPage( reflectorURL,
			"reflect",
			4, resultNamesA, false );


	finalMessagePage = new FinalMessagePage;
	loadingPage = new LoadingPage;
	autoUpdatePage = new AutoUpdatePage;
	livingLifePage = NULL;
	existingAccountPage = new ExistingAccountPage;
	extendedMessagePage = new ExtendedMessagePage;
	rebirthChoicePage = new RebirthChoicePage;
	settingsPage = new SettingsPage;


	char *reviewURL =
			SettingsManager::getStringSetting( "reviewServerURL", "" );

	if( strcmp( reviewURL, "" ) == 0 ) {
		existingAccountPage->showReviewButton( false );
		rebirthChoicePage->showReviewButton( false );
	}

	reviewPage = new ReviewPage( reviewURL );


	twinPage = new TwinPage();

	pollPage = new PollPage( reviewURL );
	delete [] reviewURL;

	geneticHistoryPage = new GeneticHistoryPage();


	// 0 music headroom needed, because we fade sounds before playing music
	setVolumeScaling( 10, 0 );
	//setSoundSpriteRateRange( 0.95, 1.05 );
	setSoundSpriteVolumeRange( 0.60, 1.0 );

	char rebuilding;

	int numSprites =
			initSpriteBankStart( &rebuilding );

	if( rebuilding ) {
		loadingPage->setCurrentPhase( translate( "spritesRebuild" ) );
	}
	else {
		loadingPage->setCurrentPhase( translate( "sprites" ) );
	}
	loadingPage->setCurrentProgress( 0 );


	loadingStepBatchSize = numSprites / numLoadingSteps;

	if( loadingStepBatchSize < 1 ) {
		loadingStepBatchSize = 1;
	}

	// for filter support in LivingLifePage
	enableObjectSearch( true );


	currentGamePage = loadingPage;

	//testPage = new TestPage;
	//currentGamePage = testPage;

	currentGamePage->base_makeActive( true );

	initDone = true;
}


// store mouse data for use as unguessable randomizing data
// for key generation, etc.
#define MOUSE_DATA_BUFFER_SIZE 20
int mouseDataBufferSize = MOUSE_DATA_BUFFER_SIZE;
int nextMouseDataIndex = 0;
// ensure that stationary mouse data (same value over and over)
// doesn't overwrite data from actual motion
float lastBufferedMouseValue = 0;
float mouseDataBuffer[ MOUSE_DATA_BUFFER_SIZE ];


void pointerMove( float inX, float inY ) {

    // save all mouse movement data for key generation
    float bufferValue = inX + inY;
    // ignore mouse positions that are the same as the last one
    // only save data when mouse actually moving
    if( bufferValue != lastBufferedMouseValue ) {
        
        mouseDataBuffer[ nextMouseDataIndex ] = bufferValue;
        lastBufferedMouseValue = bufferValue;
        
        nextMouseDataIndex ++;
        if( nextMouseDataIndex >= mouseDataBufferSize ) {
            nextMouseDataIndex = 0;
            }
        }
    

    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerMove( inX, inY );
        }
    }

void pointerDown( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDown( inX, inY );
        }
    }

void pointerDrag( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDrag( inX, inY );
        }
    }

void pointerUp( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerUp( inX, inY );
        }
}

void keyDown( unsigned char inASCII ) {

    // taking screen shot is ALWAYS possible
    if( inASCII == '=' ) {    
        saveScreenShot( "screen" );
        }
    /*
    if( inASCII == 'N' ) {
        toggleMipMapMinFilter( true );
        }
    if( inASCII == 'n' ) {
        toggleMipMapMinFilter( false );
        }
    */

    
    if( isPaused() ) {
        // block general keyboard control during pause


        switch( inASCII ) {
            case 13:  // enter
                // unpause
                pauseGame();
                break;
            }
        
        // don't let user type on pause screen anymore
        return;

        
        if( inASCII == 127 || inASCII == 8 ) {
            // subtract from it

            deleteCharFromUserTypedMessage();

            holdDeleteKeySteps = 0;
            // start with long delay until first repeat
            stepsBetweenDeleteRepeat = (int)( 30 / frameRateFactor );
            }
        else if( inASCII >= 32 ) {
            // add to it
            if( currentUserTypedMessage != NULL ) {
                
                char *oldMessage = currentUserTypedMessage;

                currentUserTypedMessage = autoSprintf( "%s%c", 
                                                       oldMessage, inASCII );
                delete [] oldMessage;
                }
            else {
                currentUserTypedMessage = autoSprintf( "%c", inASCII );
                }
            }
        
        return;
        }
    

    if( currentGamePage != NULL ) {
        currentGamePage->base_keyDown( inASCII );
        }


    
    switch( inASCII ) {
        case 'm':
        case 'M': {
#ifdef USE_MALLINFO
            struct mallinfo meminfo = mallinfo();
            printf( "Mem alloc: %d\n",
                    meminfo.uordblks / 1024 );
#endif
            }
            break;
        }
}

void keyUp( unsigned char inASCII ) {
    if( inASCII == 127 || inASCII == 8 ) {
        // delete no longer held
        // even if pause screen no longer up, pay attention to this
        holdDeleteKeySteps = -1;
        }

    if( ! isPaused() ) {

        if( currentGamePage != NULL ) {
            currentGamePage->base_keyUp( inASCII );
            }
        }

}

void specialKeyDown( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_specialKeyDown( inKey );
        }
}