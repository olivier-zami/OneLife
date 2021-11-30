//
// Created by olivier on 24/10/2021.
//

#include "GameSceneHandler.h"

#include <cstdio>
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cstdint>
#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"
//#include "minorGems/graphics/openGL/ScreenGL.h"
#include "minorGems/util/TranslationManager.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"
#include "minorGems/game/Font.h"
#include "minorGems/network/web/WebRequest.h"
#include "minorGems/graphics/Image.h"//bytes
#include "OneLife/gameSource/game.h"//Uint8
#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/procedures/graphics/drawString.h"
#include "OneLife/gameSource/dataTypes/socket.h"
#include "OneLife/gameSource/musicPlayer.h"
#include "OneLife/gameSource/dataTypes/sound.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/categoryBank.h"
#include "OneLife/gameSource/components/banks/spriteBank.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/animationBank.h"
#include "OneLife/gameSource/groundSprites.h"
#include "OneLife/gameSource/photos.h"
#include "OneLife/gameSource/emotion.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/lifeTokens.h"
#include "OneLife/gameSource/fitnessScore.h"
#include "OneLife/gameSource/transitionBank.h"
#include "OneLife/gameSource/liveObjectSet.h"
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
#include "OneLife/gameSource/procedures/graphics/screens.h"


#ifdef USE_JPEG
#include "minorGems/graphics/converters/JPEGImageConverter.h"
    static JPEGImageConverter screenShotConverter( 90 );
    static const char *screenShotExtension = "jpg";
#elif defined(USE_PNG)
#include "minorGems/graphics/converters/PNGImageConverter.h"
    static PNGImageConverter screenShotConverter;
    static const char *screenShotExtension = "png";
#else
static TGAImageConverter screenShotConverter;
static const char *screenShotExtension = "tga";
#endif

extern int targetFrameRate;
extern GamePage *currentGamePage;
extern char demoMode;
extern char *loadingFailedMessage;
extern char writeFailed;
extern double *soundSpriteMixingBufferL;
extern double *soundSpriteMixingBufferR;
extern char *webProxy;
extern FILE *aiffOutFile;
extern char recordAudio;
extern unsigned char *lastFrame_rgbaBytes;
extern char bufferSizeHinted;
extern SimpleVector<double> playingSoundSpriteRates;
extern SimpleVector<double> playingSoundSpriteVolumesR;
extern SimpleVector<double> playingSoundSpriteVolumesL;
extern SimpleVector<SoundSprite> playingSoundSprites;
extern SimpleVector<SoundSprite*> soundSprites;
extern GameSceneHandler *sceneHandler;
extern char soundOpen;
extern SDL_Cursor *ourCursor;
extern Image **screenShotImageDest;
extern char measureFrameRate;
extern SimpleVector<int> possibleFrameRates;
extern int targetFrameRate;
extern char countingOnVsync;
extern char shouldTakeScreenshot;
extern char manualScreenShot;
extern char loadingDone;
extern char enableSpeedControlKeys;
extern char hardToQuitMode;
extern SimpleVector<WebRequestRecord> webRequestRecords;
extern SpriteHandle instructionsSprite;
extern Font *mainFontReview;
extern Font *mainFontFixed;
extern Font *mainFontReview;
extern Font *handwritingFont;
extern Font *pencilFont;
extern Font *pencilErasedFont;
extern Font *smallFont;
extern char *currentUserTypedMessage;
extern ServerActionPage *getServerAddressPage;
extern FinalMessagePage *finalMessagePage;
extern LoadingPage *loadingPage;
extern AutoUpdatePage *autoUpdatePage;
extern LivingLifePage *livingLifePage;
extern ExistingAccountPage *existingAccountPage;
extern ExtendedMessagePage *extendedMessagePage;
extern RebirthChoicePage *rebirthChoicePage;
extern SettingsPage *settingsPage;
extern ReviewPage *reviewPage;
extern TwinPage *twinPage;
extern PollPage *pollPage;
extern GeneticHistoryPage *geneticHistoryPage;
extern char *reflectorURL;
extern char *serverIP;
extern char *userEmail;
extern char *accountKey;
extern char *userTwinCode;
extern char blendOutputFramePairs;
extern Font *numbersFontFixed;
extern float blendOutputFrameFraction;
extern doublePair lastScreenViewCenter;
extern double viewWidth;
extern char measureRecorded;
extern OneLife::game::Application *gameApplication;

// need to track these separately from SDL_GetModState so that
// we replay isCommandKeyDown properly during recorded game playback
char rCtrlDown = false;
char lCtrlDown = false;
char rAltDown = false;
char lAltDown = false;
char rMetaDown = false;
char lMetaDown = false;
char lShiftDown = false;
char rShiftDown = false;
char *screenShotPrefix = NULL;//TODO camera var ?
char outputAllFrames = false;// should each and every frame be saved to disk? // useful for making videos//TODO camera !
char loadingFailedFlag = false;
char *shutdownMessage = NULL;
int numPixelsDrawn = 0;
int mouseDownSteps = 1000;

// show measure screen longer if there's a vsync warning
static float viewCenterX = 0;
static float viewCenterY = 0;
char mouseWorldCoordinates = true;
static float viewSize = 2;// default -1 to +1
int screenWidth = 640;
int screenHeight = 480;
float visibleWidth = -1;
float visibleHeight = -1;
static char ignoreNextMouseEvent = false;
static char mouseDown = false;
static int lastMouseX = 0;
static int lastMouseY = 0;
static int lastMouseDownX = 0;
static int lastMouseDownY = 0;
char frameDrawerInited = false;
static int xCoordToIgnore, yCoordToIgnore;// start with last click expired
static int nextShotNumber = -1;
static char shotDirExists = false;
static int outputFrameCount = 0;
static unsigned int frameNumber = 0;

bool GameSceneHandler::mPaused = false;

GameSceneHandler::GameSceneHandler():
		  mPausedDuringFrameBatch( true ),
		  mLoadingDuringFrameBatch( true ),
		  mPausedSleepTime( 0 ),
		  mBlockQuitting( false ),
		  mLastFrameRate( targetFrameRate ),
		  mStartTimeSeconds( Time::timeSec() ),
		  mPrintFrameRate( true ),
		  mNumFrames( 0 ), mFrameBatchSize( 100 ),
		  mFrameBatchStartTimeSeconds( -1 ),
		  mBackgroundColor( 0, 0, 0, 1 ) {


	glClearColor( mBackgroundColor.r,
				  mBackgroundColor.g,
				  mBackgroundColor.b,
				  mBackgroundColor.a );


	// set external pointer so it can be used in calls below
	sceneHandler = this;

	gameApplication->addRedrawListener( sceneHandler );
}

GameSceneHandler::~GameSceneHandler() {
	gameApplication->removeMouseHandler( this );
	//gameApplication->removeSceneHandler( this );
	gameApplication->removeRedrawListener( this );

	if( demoMode ) {
		// panel has not freed itself yet
		freeDemoCodePanel();

		demoMode = false;
	}


	if( loadingFailedMessage != NULL ) {
		delete [] loadingFailedMessage;
		loadingFailedMessage = NULL;
	}


}

void GameSceneHandler::setPause(bool status)
{
	GameSceneHandler::mPaused = status;
}

void GameSceneHandler::switchPause()
{
	GameSceneHandler::mPaused = !GameSceneHandler::mPaused;
}

bool GameSceneHandler::isPaused()
{
	return GameSceneHandler::mPaused;
}

void GameSceneHandler::initFromFiles() {

}

void GameSceneHandler::drawScene() {
	numPixelsDrawn = 0;
	//!Screen selection
	//!FIND LEG000001


	this->doFeatureComputeFrame();

	redoDrawMatrix();

	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );

	if( visibleWidth != -1 && visibleHeight != -1 )
	{
		// draw letterbox

		// On most platforms, glViewport will clip image for us.
		// glScissor is also supposed to do this, but it is buggy on some
		// platforms
		// thus, to be safe, we keep glScissor off and manually draw letterboxes
		// just in case glViewport doesn't clip the image.

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();


		// viewport is square of largest dimension, centered on screen

		int bigDimension = screenWidth;

		if( screenHeight > bigDimension ) {
			bigDimension = screenHeight;
		}

		float excessX = ( bigDimension - screenWidth ) / 2;
		float excessY = ( bigDimension - screenHeight ) / 2;

		glOrtho( -excessX, -excessX + bigDimension,
				 -excessY + bigDimension, -excessY,
				 -1.0f, 1.0f );

		glViewport( -excessX,
					-excessY,
					bigDimension,
					bigDimension );

		glMatrixMode(GL_MODELVIEW);

		setDrawColor( 0, 0, 0, 1.00 );

		int extraWidth =
				lrint(
						screenWidth -
						( (double)visibleWidth / (double)visibleHeight ) *
						screenHeight );

		if( extraWidth > 0 ) {


			// left/right bars
			drawRect( 0,
					  0,
					  extraWidth / 2,
					  screenHeight );

			drawRect( screenWidth - extraWidth / 2,
					  0,
					  screenWidth,
					  screenHeight );
		}
		else {

			int extraHeight =
					lrint(
							screenHeight -
							( (double)visibleHeight / (double)visibleWidth ) *
							screenWidth );

			if( extraHeight > 0 ) {

				// top/bottom bars

				drawRect( 0,
						  0,
						  screenWidth,
						  extraHeight / 2 );

				drawRect( 0,
						  screenHeight - extraHeight / 2,
						  screenWidth,
						  screenHeight );
			}
		}
	}

	if( shouldTakeScreenshot )
	{
		takeScreenShot();

		if( !outputAllFrames ) {
			// just one
			shouldTakeScreenshot = false;
		}
		manualScreenShot = false;
	}

	frameNumber ++;
	//printf( "%d pixels drawn (%.2F MB textures resident)\n",
	//        numPixelsDrawn, totalLoadedTextureBytes / ( 1024.0 * 1024.0 ) );
}

void GameSceneHandler::doFeatureComputeFrame()
{
	// do this here, because it involves game_getCurrentTime() calls
	// which are recorded, and aren't available for playback in fireRedraw
	mNumFrames ++;

	if( mPrintFrameRate ) {

		if( mFrameBatchStartTimeSeconds == -1 ) {
			mFrameBatchStartTimeSeconds = game_getCurrentTime();
		}

		if( mNumFrames % mFrameBatchSize == 0 ) {
			// finished a batch

			double timeDelta = game_getCurrentTime() - mFrameBatchStartTimeSeconds;

			double actualFrameRate = (double)mFrameBatchSize / (double)timeDelta;

			//AppLog::getLog()->logPrintf(
			//    Log::DETAIL_LEVEL,
			//printf("Frame rate = %f frames/second\n", actualFrameRate );

			mFrameBatchStartTimeSeconds = game_getCurrentTime();

			// don't update reported framerate if this batch involved a pause
			// or if still loading
			if( !mPausedDuringFrameBatch && !mLoadingDuringFrameBatch ) {
				mLastFrameRate = actualFrameRate;
			}
			else {
				// consider measuring next frame batch
				// (unless a pause or loading occurs there too)
				mPausedDuringFrameBatch = false;
				mLoadingDuringFrameBatch = false;
			}
		}
	}
}


void GameSceneHandler::mouseMoved( int inX, int inY ) {
	if( ignoreNextMouseEvent ) {
		if( inX == xCoordToIgnore && inY == yCoordToIgnore ) {
			// seeing the event that triggered the ignore
			ignoreNextMouseEvent = false;
			return;
		}
		else {
			// stale pending event before the ignore
			// skip it too
			return;
		}
	}

	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerMove( x, y );

	lastMouseX = inX;
	lastMouseY = inY;
}

void GameSceneHandler::mouseDragged( int inX, int inY ) {
	if( ignoreNextMouseEvent ) {
		if( inX == xCoordToIgnore && inY == yCoordToIgnore ) {
			// seeing the event that triggered the ignore
			ignoreNextMouseEvent = false;
			return;
		}
		else {
			// stale pending event before the ignore
			// skip it too
			return;
		}
	}

	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerDrag( x, y );

	lastMouseX = inX;
	lastMouseY = inY;
}




void GameSceneHandler::mousePressed( int inX, int inY ) {
	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerDown( x, y );

	lastMouseX = inX;
	lastMouseY = inY;

	mouseDown = true;
	mouseDownSteps = 0;
	lastMouseDownX = inX;
	lastMouseDownY = inY;
}



void GameSceneHandler::mouseReleased( int inX, int inY ) {
	float x, y;
	screenToWorld( inX, inY, &x, &y );
	pointerUp( x, y );

	lastMouseX = inX;
	lastMouseY = inY;
	mouseDown = false;

	// start new animation for release
	mouseDownSteps = 0;
	lastMouseDownX = inX;
	lastMouseDownY = inY;
}



void GameSceneHandler::fireRedraw() {

	if( !loadingDone ) {
		mLoadingDuringFrameBatch = true;
	}

	if( mPaused ) {
		// ignore redraw event

		mPausedDuringFrameBatch = true;

		if( mPausedSleepTime > (unsigned int)( 5 * targetFrameRate ) ) {
			// user has touched nothing for 5 seconds

			// sleep to avoid wasting CPU cycles
			Thread::staticSleep( 500 );
		}

		mPausedSleepTime++;

		return;
	}


}

static unsigned char lastKeyPressed = '\0';


void GameSceneHandler::keyPressed( unsigned char inKey, int inX, int inY )
{

	if( writeFailed || loadingFailedFlag ) {
		exit( 0 );
	}

	if( measureFrameRate && measureRecorded ) {
		if( inKey == 'y' || inKey == 'Y' ) {
			saveFrameRateSettings();
			gameApplication->startRecordingOrPlayback();
			measureFrameRate = false;
		}
		else if( inKey == 27 ) {
			exit( 0 );
		}
	}



	// reset to become responsive while paused
	mPausedSleepTime = 0;


	if( mPaused && inKey == '%' && ! mBlockQuitting ) {
		// % to quit from pause
		exit( 0 );
	}


	if( inKey == 9 && isCommandKeyDown() && gameApplication->isPlayingBack() ) {

		printf( "Caught alt-tab during playback, pausing\n" );

		// alt-tab pressed during playback
		// but we aren't actually being minimized during playback
		// (because there's nothing to bring us back)
		// Still, force a pause, so that user's unpause action after
		// tabbing back in replays correctly
		mPaused = true;
	}


	if( enableSpeedControlKeys ) {

		if( inKey == '^' ) {
			// slow
			gameApplication->setMaxFrameRate( 2 );
			gameApplication->useFrameSleep( true );
		}
		if( inKey == '&' ) {
			// half
			gameApplication->setMaxFrameRate( targetFrameRate / 2 );
			gameApplication->useFrameSleep( true );
		}
		if( inKey == '*' ) {
			// normal
			gameApplication->setMaxFrameRate( targetFrameRate );

			if( countingOnVsync ) {
				gameApplication->useFrameSleep( false );
			}
			else {
				gameApplication->useFrameSleep( true );
			}
		}
		if( inKey == '(' ) {
			// fast forward
			gameApplication->setMaxFrameRate( targetFrameRate * 2 );
			gameApplication->useFrameSleep( true );
		}
		if( inKey == ')' ) {
			// fast fast forward
			gameApplication->setMaxFrameRate( targetFrameRate * 4 );
			gameApplication->useFrameSleep( true );
		}
		if( inKey == '-' ) {
			// fast fast fast forward
			gameApplication->setMaxFrameRate( targetFrameRate * 8 );
			gameApplication->useFrameSleep( true );
		}
	}

	if( !hardToQuitMode ) {
		// escape only

		if( inKey == 27 ) {
			// escape always toggles pause
			mPaused = !mPaused;
		}
	}
	else {
		// # followed by ESC
		if( lastKeyPressed == '#' && inKey == 27 ) {
			exit( 0 );
		}
		lastKeyPressed = inKey;
	}

	keyDown( inKey );
}



void GameSceneHandler::keyReleased(
		unsigned char inKey, int inX, int inY ) {

	keyUp( inKey );
}

void GameSceneHandler::specialKeyPressed(
		int inKey, int inX, int inY ) {

	if( writeFailed || loadingFailedFlag ) {
		exit( 0 );
	}

	switch( inKey ) {
		case MG_KEY_RCTRL:
			rCtrlDown = true;
			break;
		case MG_KEY_LCTRL:
			lCtrlDown = true;
			break;
		case MG_KEY_RALT:
			rAltDown = true;
			break;
		case MG_KEY_LALT:
			lAltDown = true;
			break;
		case MG_KEY_RMETA:
			rMetaDown = true;
			break;
		case MG_KEY_LMETA:
			lMetaDown = true;
			break;
		case MG_KEY_RSHIFT:
			rShiftDown = true;
			break;
		case MG_KEY_LSHIFT:
			lShiftDown = true;
			break;
	}


	specialKeyDown( inKey );
}



void GameSceneHandler::specialKeyReleased(
		int inKey, int inX, int inY ) {


	switch( inKey ) {
		case MG_KEY_RCTRL:
			rCtrlDown = false;
			break;
		case MG_KEY_LCTRL:
			lCtrlDown = false;
			break;
		case MG_KEY_RALT:
			rAltDown = false;
			break;
		case MG_KEY_LALT:
			lAltDown = false;
			break;
		case MG_KEY_RMETA:
			rMetaDown = false;
			break;
		case MG_KEY_LMETA:
			lMetaDown = false;
			break;
		case MG_KEY_RSHIFT:
			rShiftDown = false;
			break;
		case MG_KEY_LSHIFT:
			lShiftDown = false;
			break;
	}


	specialKeyUp( inKey );
}

void GameSceneHandler::actionPerformed( GUIComponent *inTarget ) {
}

void takeScreenShot() {//TODO: camera ?


	File shotDir( NULL, "screenShots" );

	if( !shotDirExists && !shotDir.exists() ) {
		shotDir.makeDirectory();
		shotDirExists = shotDir.exists();
	}

	if( nextShotNumber < 1 ) {
		if( shotDir.exists() && shotDir.isDirectory() ) {

			int numFiles;
			File **childFiles = shotDir.getChildFiles( &numFiles );

			nextShotNumber = 1;

			char *formatString = autoSprintf( "%s%%d.%s", screenShotPrefix,
											  screenShotExtension );

			for( int i=0; i<numFiles; i++ ) {

				char *name = childFiles[i]->getFileName();

				int number;

				int numRead = sscanf( name, formatString, &number );

				if( numRead == 1 ) {

					if( number >= nextShotNumber ) {
						nextShotNumber = number + 1;
					}
				}
				delete [] name;

				delete childFiles[i];
			}

			delete [] formatString;

			delete [] childFiles;
		}
	}


	if( nextShotNumber < 1 ) {
		return;
	}

	char *fileName = autoSprintf( "%s%05d.%s",
								  screenShotPrefix, nextShotNumber,
								  screenShotExtension );



	File *file = shotDir.getChildFile( fileName );

	delete [] fileName;


	if( outputAllFrames ) {
		printf( "Output Frame %d (%.2f sec)\n", outputFrameCount,
				outputFrameCount / (double) targetFrameRate );
		outputFrameCount ++;
	}


	Image *screenImage =
			getScreenRegionInternal( 0, 0, screenWidth, screenHeight );

	if( screenImage == NULL ) {
		// a skipped frame due to blending settings
		delete file;

		return;
	}





	if( screenShotImageDest != NULL ) {
		// skip writing to file
		*screenShotImageDest = screenImage;
	}
	else {
		FileOutputStream tgaStream( file );
		screenShotConverter.formatImage( screenImage, &tgaStream );
		delete screenImage;
	}

	delete file;

	nextShotNumber++;
}

void warpMouseToScreenPos( int inX, int inY ) {
	if( inX == lastMouseX && inY == lastMouseY ) {
		// mouse already there, no need to warp
		// (and warping when already there may or may not generate
		//  an event on some platforms, which causes trouble when we
		//  try to ignore the event)
		return;
	}

	if( SDL_GetAppState() & SDL_APPINPUTFOCUS ) {

		if( frameDrawerInited ) {
			// not ignoring mouse events currently due to demo code panel
			// or loading message... frame drawer not inited yet
			ignoreNextMouseEvent = true;
			xCoordToIgnore = inX;
			yCoordToIgnore = inY;
		}

		SDL_WarpMouse( inX, inY );
	}
}

void warpMouseToCenter( int *outNewMouseX, int *outNewMouseY ) {
	*outNewMouseX = screenWidth / 2;
	*outNewMouseY = screenHeight / 2;

	warpMouseToScreenPos( *outNewMouseX, *outNewMouseY );
}

// function that destroys object when exit is called.
// exit is the only way to stop the loop in  ScreenGL
void cleanUpAtExit() {

	if( ourCursor != NULL ) {
		SDL_FreeCursor( ourCursor );
	}

	AppLog::info( "exiting...\n" );

	if( soundOpen ) {
		AppLog::info( "exiting: calling SDL_CloseAudio\n" );
		SDL_CloseAudio();
	}
	soundOpen = false;


	AppLog::info( "exiting: Deleting sceneHandler\n" );
	delete sceneHandler;



	AppLog::info( "Freeing sound sprites\n" );
	for( int i=0; i<soundSprites.size(); i++ ) {
		SoundSprite *s = soundSprites.getElementDirect( i );
		delete [] s->samples;
		delete s;
	}
	soundSprites.deleteAll();
	playingSoundSprites.deleteAll();
	playingSoundSpriteRates.deleteAll();
	playingSoundSpriteVolumesL.deleteAll();
	playingSoundSpriteVolumesR.deleteAll();

	if( bufferSizeHinted ) {
		freeHintedBuffers();
		bufferSizeHinted = false;
	}


	if( frameDrawerInited ) {
		AppLog::info( "exiting: freeing frameDrawer\n" );
		freeFrameDrawer();
	}


	AppLog::info( "exiting: Deleting screen\n" );
	delete gameApplication;


	AppLog::info( "exiting: freeing drawString\n" );
	freeDrawString();

#ifdef RASPBIAN
	raspbianReleaseSurface();
#endif

	AppLog::info( "exiting: calling SDL_Quit()\n" );
	SDL_Quit();

	if( screenShotPrefix != NULL ) {
		AppLog::info( "exiting: deleting screenShotPrefix\n" );

		delete [] screenShotPrefix;
		screenShotPrefix = NULL;
	}

	if( lastFrame_rgbaBytes != NULL ) {
		AppLog::info( "exiting: deleting lastFrame_rgbaBytes\n" );

		delete [] lastFrame_rgbaBytes;
		lastFrame_rgbaBytes = NULL;
	}


	if( recordAudio ) {
		AppLog::info( "exiting: closing audio output file\n" );

		recordAudio = false;
		fclose( aiffOutFile );
		aiffOutFile = NULL;
	}

	for( int i=0; i<webRequestRecords.size(); i++ ) {
		AppLog::infoF( "exiting: Deleting lingering web request %d\n", i );

		WebRequestRecord *r = webRequestRecords.getElement( i );

		delete r->request;
	}

	if( webProxy != NULL ) {
		delete [] webProxy;
		webProxy = NULL;
	}

	if( soundSpriteMixingBufferL != NULL ) {
		delete [] soundSpriteMixingBufferL;
	}

	if( soundSpriteMixingBufferR != NULL ) {
		delete [] soundSpriteMixingBufferR;
	}


	AppLog::info( "exiting: Done.\n" );
}

void getLastMouseScreenPos( int *outX, int *outY ) {
	*outX = lastMouseX;
	*outY = lastMouseY;
}

void warpMouseToWorldPos( float inX, float inY ) {
	int worldX, worldY;
	worldToScreen( inX, inY, &worldX, &worldY );
	warpMouseToScreenPos( worldX, worldY );
}

char isCommandKeyDown() {
	SDLMod modState = SDL_GetModState();


	if( ( modState & KMOD_CTRL )
		||
		( modState & KMOD_ALT )
		||
		( modState & KMOD_META ) ) {

		return true;
	}

	if( gameApplication->isPlayingBack() ) {
		// ignore these, saved internally, unless we're playing back
		// they can fall out of sync with keyboard reality as the user
		// alt-tabs between windows and release events are lost.
		if( rCtrlDown || lCtrlDown ||
			rAltDown || lAltDown ||
			rMetaDown || lMetaDown ) {
			return true;
		}
	}

	return false;
}

void specialKeyUp( int inKey ) {
	if( isPaused() ) {
		return;
	}

	if( currentGamePage != NULL ) {
		currentGamePage->base_specialKeyUp( inKey );
	}
}

void worldToScreen( float inX, float inY, int *outX, int *outY )
{
	if( mouseWorldCoordinates )
	{
		// inverse of screenToWorld
		inX -= viewCenterX;
		inX /= viewSize;

		inX *= screenWidth;
		inX += screenWidth/2;

		*outX = round( inX );


		inY -= viewCenterY;
		inY /= viewSize;

		inY *= -screenWidth;
		inY += screenHeight/2;

		*outY = round( inY );
	}
	else {
		// raw screen coordinates
		*outX = inX;
		*outY = inY;
	}
}

void screenToWorld( int inX, int inY, float *outX, float *outY )
{

	if( mouseWorldCoordinates ) {

		// relative to center,
		// viewSize spreads out across screenWidth only (a square on screen)
		float x = (float)( inX - (screenWidth/2) ) / (float)screenWidth;
		float y = -(float)( inY - (screenHeight/2) ) / (float)screenWidth;

		*outX = x * viewSize + viewCenterX;
		*outY = y * viewSize + viewCenterY;
	}
	else {
		// raw screen coordinates
		*outX = inX;
		*outY = inY;
	}

}

doublePair getViewCenterPosition()
{
	doublePair p = { viewCenterX, viewCenterY };

	return p;
}

// for moving view around
void setViewCenterPosition( float inX, float inY ) {
	viewCenterX = inX;
	viewCenterY = inY;
	redoDrawMatrix();
}

void redoDrawMatrix() {
	// viewport square centered on screen (even if screen is rectangle)
	float hRadius = viewSize / 2;

	float wRadius = hRadius;

	if( visibleHeight > 0 ) {
		wRadius = visibleWidth / 2;
		hRadius = visibleHeight / 2;
	}


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho( viewCenterX - wRadius, viewCenterX + wRadius,
			 viewCenterY - hRadius, viewCenterY + hRadius, -1.0f, 1.0f);

	if( visibleHeight > 0 ) {

		float portWide = screenWidth;
		float portHigh = ( visibleHeight / visibleWidth ) * portWide;

		float screenHeightFraction = (float) screenHeight / (float) screenWidth;

		if( screenHeightFraction < 9.0 / 16.0 ) {
			// wider than 16:9

			// fill vertically instead
			portHigh = screenHeight;

			portWide = ( visibleWidth / visibleHeight ) * portHigh;
		}

		float excessW = screenWidth - portWide;
		float excessH = screenHeight - portHigh;

		glViewport( excessW / 2,
					excessH / 2,
					portWide,
					portHigh );
	}

	glMatrixMode(GL_MODELVIEW);
}

void setMouseReportingMode( char inWorldCoordinates ) {
	mouseWorldCoordinates = inWorldCoordinates;
}

void setViewSize( float inSize ) {
	viewSize = inSize;
	redoDrawMatrix();
}

void getScreenDimensions( int *outWidth, int *outHeight )
{
	*outWidth = screenWidth;
	*outHeight = screenHeight;
}

void setLetterbox( float inVisibleWidth, float inVisibleHeight ) {
	visibleWidth = inVisibleWidth;
	visibleHeight = inVisibleHeight;
}

void freeFrameDrawer() {


	freeSprite( instructionsSprite );

	delete mainFontReview;
	delete mainFontFixed;
	delete numbersFontFixed;

	delete handwritingFont;
	delete pencilFont;
	delete pencilErasedFont;

	delete smallFont;

	if( currentUserTypedMessage != NULL ) {
		delete [] currentUserTypedMessage;
		currentUserTypedMessage = NULL;
	}



	if( shutdownMessage != NULL ) {
		delete [] shutdownMessage;
		shutdownMessage = NULL;
	}


	delete getServerAddressPage;

	delete finalMessagePage;
	delete loadingPage;
	delete autoUpdatePage;
	if( livingLifePage != NULL ) {
		delete livingLifePage;
		livingLifePage = NULL;
	}

	delete existingAccountPage;
	delete extendedMessagePage;
	delete rebirthChoicePage;
	delete settingsPage;
	delete reviewPage;
	delete twinPage;
	delete pollPage;
	delete geneticHistoryPage;

	//if( testPage != NULL ) {
	//    delete testPage;
	//    testPage = NULL;
	//    }


	freeGroundSprites();

	freeAnimationBank();
	freeObjectBank();
	freeSpriteBank();

	freeTransBank();

	freeCategoryBank();

	freeLiveObjectSet();

	freeSoundBank();

	freeMusicPlayer();
	freeEmotion();

	freePhotos();
	freeLifeTokens();
	freeFitnessScore();

	if( reflectorURL != NULL ) {
		delete [] reflectorURL;
		reflectorURL = NULL;
	}

	if( serverIP != NULL ) {
		delete [] serverIP;
		serverIP = NULL;
	}


	if( userEmail != NULL ) {
		delete [] userEmail;
	}
	if( accountKey != NULL ) {
		delete [] accountKey;
	}
	if( userTwinCode != NULL ) {
		delete [] userTwinCode;
	}
}


// if manualScreenShot false, then any blend settings (for saving blended
// double-frames) are applied
// can return NULL in this case (when this frame should not be output
// according to blending settings)
//
// Region in screen pixels
Image *getScreenRegionInternal(
		int inStartX, int inStartY, int inWidth, int inHeight,
		char inForceManual) {

	int numBytes = inWidth * inHeight * 3;

	unsigned char *rgbBytes =
			new unsigned char[ numBytes ];

	// w and h might not be multiples of 4
	GLint oldAlignment;
	glGetIntegerv( GL_PACK_ALIGNMENT, &oldAlignment );

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );

	glReadPixels( inStartX, inStartY, inWidth, inHeight,
				  GL_RGB, GL_UNSIGNED_BYTE, rgbBytes );

	glPixelStorei( GL_PACK_ALIGNMENT, oldAlignment );


	if( ! inForceManual &&
		! manualScreenShot &&
		blendOutputFramePairs &&
		frameNumber % 2 != 0 &&
		lastFrame_rgbaBytes != NULL ) {

		// save blended frames on odd frames
		if( blendOutputFrameFraction > 0 ) {
			float blendA = 1 - blendOutputFrameFraction;
			float blendB = blendOutputFrameFraction;

			for( int i=0; i<numBytes; i++ ) {
				rgbBytes[i] =
						(unsigned char)(
								blendA * rgbBytes[i] +
								blendB * lastFrame_rgbaBytes[i] );
			}
		}

	}
	else if( ! inForceManual &&
			 ! manualScreenShot &&
			 blendOutputFramePairs &&
			 frameNumber % 2 == 0 ) {

		// skip even frames, but save them for next blending

		if( lastFrame_rgbaBytes != NULL ) {
			delete [] lastFrame_rgbaBytes;
			lastFrame_rgbaBytes = NULL;
		}

		lastFrame_rgbaBytes = rgbBytes;

		return NULL;
	}


	Image *screenImage = new Image( inWidth, inHeight, 3, false );

	double *channelOne = screenImage->getChannel( 0 );
	double *channelTwo = screenImage->getChannel( 1 );
	double *channelThree = screenImage->getChannel( 2 );

	// image of screen is upside down
	int outputRow = 0;
	for( int y=inHeight - 1; y>=0; y-- ) {
		for( int x=0; x<inWidth; x++ ) {

			int outputPixelIndex = outputRow * inWidth + x;


			int regionPixelIndex = y * inWidth + x;
			int byteIndex = regionPixelIndex * 3;

			// optimization found:  should unroll this loop over 3 channels
			// divide by 255, with a multiply
			channelOne[outputPixelIndex] =
					rgbBytes[ byteIndex++ ] * 0.003921569;
			channelTwo[outputPixelIndex] =
					rgbBytes[ byteIndex++ ] * 0.003921569;
			channelThree[outputPixelIndex] =
					rgbBytes[ byteIndex++ ] * 0.003921569;
		}
		outputRow++;
	}

	delete [] rgbBytes;

	return screenImage;
}


Image *getScreenRegionRaw(
		int inStartX, int inStartY, int inWidth, int inHeight ) {

	return getScreenRegionInternal( inStartX, inStartY, inWidth, inHeight,
									true );
}

Image *getScreenRegion( double inX, double inY,
						double inWidth, double inHeight ) {

	double endX = inX + inWidth;
	double endY = inY + inHeight;

	// rectangle specified in integer screen coordinates

	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];

	glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );

	GLdouble winStartX, winStartY, winStartZ;
	GLdouble winEndX, winEndY, winEndZ;

	gluProject( inX, inY, 0,
				modelview, projection, viewport,
				&winStartX, &winStartY, &winStartZ );

	gluProject( endX, endY, 0,
				modelview, projection, viewport,
				&winEndX, &winEndY, &winEndZ );




	char oldManual = manualScreenShot;
	manualScreenShot = true;


	Image *result =
			getScreenRegionInternal(
					lrint( winStartX ), lrint( winStartY ),
					lrint( winEndX - winStartX ), lrint( winEndY - winStartY ) );

	manualScreenShot = oldManual;

	return result;
}

void saveFrameRateSettings() {
	SettingsManager::setSetting( "targetFrameRate", targetFrameRate );

	int settingValue = 0;
	if( countingOnVsync ) {
		settingValue = 1;
	}

	SettingsManager::setSetting( "countingOnVsync", settingValue );
}

void loadingFailed( const char *inFailureMessage ) {//TODO: check if unused
	loadingFailedFlag = true;

	if( loadingFailedMessage != NULL ) {
		delete [] loadingFailedMessage;
	}
	loadingFailedMessage = stringDuplicate( inFailureMessage );
}

void deleteCharFromUserTypedMessage() {
	if( currentUserTypedMessage != NULL ) {

		int length = strlen( currentUserTypedMessage );

		char fileSeparatorDeleted = false;
		if( length > 2 ) {
			if( currentUserTypedMessage[ length - 2 ] == 28 ) {
				// file separator with spaces around it
				// delete whole thing with one keypress
				currentUserTypedMessage[ length - 3 ] = '\0';
				fileSeparatorDeleted = true;
			}
		}
		if( !fileSeparatorDeleted && length > 0 ) {
			currentUserTypedMessage[ length - 1 ] = '\0';
		}
	}
}

