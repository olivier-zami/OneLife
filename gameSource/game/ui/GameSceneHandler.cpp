//
// Created by olivier on 18/08/2022.
//

#include "GameSceneHandler.h"

#include "../Application.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"
#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"//translate?
#include "panel/DemoCodePanel.h"
#include "sdl.h"

#include <SDL/SDL.h>
#include <GL/gl.h>

char enableSpeedControlKeys = false;// ^ and & keys to slow down and speed up for testing// read from settings folder
char loadingMessageShown = false;
char mouseDown = false;
char mouseRightDown = false;
int numPixelsDrawn = 0;
int pauseOnMinimize = 1;

extern char bufferSizeHinted;
extern char countingOnVsync;
extern int cursorMode;
extern char demoMode;
extern double emulatedCursorScale;
extern char frameDrawerInited;
extern unsigned int frameNumber;
extern char hardToQuitMode;
extern int gameHeight;
extern int gameWidth;
extern int idealTargetFrameRate;
extern char ignoreNextMouseEvent;
extern int lastMouseX;
extern int lastMouseY;
extern char loadingDone;
extern char loadingFailedFlag;
extern char *loadingFailedMessage;
extern char manualScreenShot;
extern char measureFrameRate;
extern char outputAllFrames;
extern int pixelZoomFactor;
extern char recordAudio;
extern GameSceneHandler *sceneHandler;
extern ScreenGL *screen;
extern int screenHeight;
extern int screenWidth;
extern char shouldTakeScreenshot;
extern int soundSampleRate;
extern double *soundSpriteMixingBufferR;
extern double *soundSpriteMixingBufferL;
extern int targetFrameRate;
extern float visibleWidth;
extern float visibleHeight;
extern char writeFailed;

extern char rCtrlDown;
extern char lCtrlDown;
extern char rAltDown;
extern char lAltDown;
extern char rMetaDown;
extern char lMetaDown;
extern char lShiftDown;
extern char rShiftDown;

extern int lastMouseX;
extern int lastMouseY;
extern int lastMouseDownX;
extern int lastMouseDownY;
extern char mouseDown;
extern char mouseRightDown;
int mouseDownSteps = 1000;// start with last click expired
extern char ignoreNextMouseEvent;
extern int xCoordToIgnore, yCoordToIgnore;

static unsigned char lastKeyPressed = '\0';
static char measureRecorded = false;
static double noWarningSecondsToMeasure = 1;// show measure screen longer if there's a vsync warning
static int numFramesMeasured = 0;
static int numFramesSkippedBeforeMeasure = 0;
static int numFramesToSkipBeforeMeasure = 30;
SimpleVector<int> possibleFrameRates;
static double secondsToMeasure = noWarningSecondsToMeasure;
static double startMeasureTime = 0;
static char startMeasureTimeRecorded = false;
static double warningSecondsToMeasure = 3;

GameSceneHandler::GameSceneHandler( ScreenGL *inScreen )
		: mScreen( inScreen ),
		  mPaused( false ),
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


	mScreen->addSceneHandler( sceneHandler );
	mScreen->addRedrawListener( sceneHandler );
}

GameSceneHandler::~GameSceneHandler() {
	mScreen->removeMouseHandler( this );
	mScreen->removeSceneHandler( this );
	mScreen->removeRedrawListener( this );

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

void GameSceneHandler::initFromFiles() {

}

/**
 * TODO: check for connecting routine
 */
void GameSceneHandler::drawScene() {
	numPixelsDrawn = 0;
	/*
	glClearColor( mBackgroundColor->r,
				  mBackgroundColor->g,
				  mBackgroundColor->b,
				  mBackgroundColor->a );
	*/




	// do this here, because it involves game_getCurrentTime() calls
	// which are recorded, and aren't available for playback in fireRedraw
	mNumFrames ++;

	if( mPrintFrameRate ) {

		if( mFrameBatchStartTimeSeconds == -1 ) {
			mFrameBatchStartTimeSeconds = game_getCurrentTime();
		}

		if( mNumFrames % mFrameBatchSize == 0 ) {
			// finished a batch

			double timeDelta =
					game_getCurrentTime() - mFrameBatchStartTimeSeconds;

			double actualFrameRate =
					(double)mFrameBatchSize / (double)timeDelta;

			//AppLog::getLog()->logPrintf(
			//    Log::DETAIL_LEVEL,
			printf(
					"Frame rate = %f frames/second\n", actualFrameRate );

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

	redoDrawMatrix();
	glDisable( GL_CULL_FACE );
	glDisable( GL_DEPTH_TEST );

	if( demoMode )
	{

		if( ! isDemoCodePanelShowing() ) {

			// stop demo mode when panel done
			demoMode = false;

			mScreen->addMouseHandler( this );
			mScreen->addKeyboardHandler( this );

			screen->startRecordingOrPlayback();
		}
	}
	else if( writeFailed )
	{
		drawString( translate( "writeFailed" ), true );
	}
	else if( !screen->isPlayingBack() && measureFrameRate )
	{
		if( !measureRecorded )
		{
			screen->useFrameSleep( false );
		}

		if( numFramesSkippedBeforeMeasure < numFramesToSkipBeforeMeasure )
		{
			numFramesSkippedBeforeMeasure++;

			drawString( translate( "measuringFPS" ), true );
		}
		else if( ! startMeasureTimeRecorded )
		{
			startMeasureTime = Time::getCurrentTime();
			startMeasureTimeRecorded = true;

			drawString( translate( "measuringFPS" ), true );
		}
		else
		{
			numFramesMeasured++;
			double totalTime = Time::getCurrentTime() - startMeasureTime;
			double timePerFrame = totalTime / ( numFramesMeasured );
			double frameRate = 1 / timePerFrame;
			int closestTargetFrameRate = 0;
			double closestFPSDiff = 9999999;
			for( int i=0; i<possibleFrameRates.size(); i++ )
			{
				int v = possibleFrameRates.getElementDirect( i );
				double diff = fabs( frameRate - v );
				if( diff < closestFPSDiff )
				{
					closestTargetFrameRate = v;
					closestFPSDiff = diff;
				}
			}

			double overAllowFactor = 1.05;
			if( numFramesMeasured > 10 && frameRate > overAllowFactor * closestTargetFrameRate )
			{
				secondsToMeasure = warningSecondsToMeasure;
			}
			else
			{
				secondsToMeasure = noWarningSecondsToMeasure;
			}

			if( totalTime <= secondsToMeasure )
			{
				char *message = autoSprintf( "%s\n%0.2f\nFPS",
											 translate( "measuringFPS" ),
											 frameRate );
				drawString( message, true );
				delete [] message;
			}

			if( totalTime > secondsToMeasure )
			{
				if( ! measureRecorded )
				{
					if( targetFrameRate == idealTargetFrameRate )
					{
						// not invoking halfFrameRate
						AppLog::infoF( "Measured frame rate = %f fps\n", frameRate );
						AppLog::infoF("Closest possible frame rate = %d fps\n", closestTargetFrameRate );
						if( frameRate > overAllowFactor * closestTargetFrameRate )
						{
							AppLog::infoF(
									"Vsync to enforce closested frame rate of "
									"%d fps doesn't seem to be in effect.\n",
									closestTargetFrameRate );
							AppLog::infoF(
									"Will sleep each frame to enforce desired "
									"frame rate of %d fps\n",
									idealTargetFrameRate );
							targetFrameRate = idealTargetFrameRate;
							screen->useFrameSleep( true );
							countingOnVsync = false;
						}
						else
						{
							AppLog::infoF(
									"Vsync seems to be enforcing an allowed frame "
									"rate of %d fps.\n", closestTargetFrameRate );
							targetFrameRate = closestTargetFrameRate;
							screen->useFrameSleep( false );
							countingOnVsync = true;
						}
					}
					else
					{
						// half frame rate must be set
						AppLog::infoF(
								"User has halfFrameRate set, so we're going "
								"to manually sleep to enforce a target "
								"frame rate of %d fps.\n", targetFrameRate );
						screen->useFrameSleep( true );
						countingOnVsync = false;
					}
					screen->setFullFrameRate( targetFrameRate );
					measureRecorded = true;
				}

				if( !countingOnVsync )
				{
					// show warning message
					char *message =
							autoSprintf( "%s\n%s\n\n%s\n\n\n%s",
										 translate( "vsyncWarning" ),
										 translate( "vsyncWarning2" ),
										 translate( "vsyncWarning3" ),
										 translate( "vsyncContinueMessage" ) );
					drawString( message, true );

					delete [] message;
				}
				else
				{
					// auto-save it now
					saveFrameRateSettings();
					screen->startRecordingOrPlayback();
					measureFrameRate = false;
				}
			}
		}

		return;
	}
	else if( !loadingMessageShown )
	{
		drawString( translate( "loading" ), true );
		loadingMessageShown = true;
	}
	else if( loadingFailedFlag )
	{
		drawString( loadingFailedMessage, true );
	}
	else if( !writeFailed && !loadingFailedFlag && !frameDrawerInited )
	{
		drawString( translate( "loading" ), true );

		initFrameDrawer( pixelZoomFactor * gameWidth,
						 pixelZoomFactor * gameHeight,
						 targetFrameRate,
						 screen->getCustomRecordedGameData(),
						 screen->isPlayingBack() );

		int readCursorMode = SettingsManager::getIntSetting( "cursorMode", -1 );

		if( readCursorMode < 0 )
		{
			// never set before

			// check if we are ultrawidescreen
			char ultraWide = false;

			const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

			int currentW = currentScreenInfo->current_w;
			int currentH = currentScreenInfo->current_h;

			double aspectRatio = (double)currentW / (double)currentH;

			// give a little wiggle room above 16:9
			// ultrawide starts at 21:9
			if( aspectRatio > 18.0 / 9.0 ) {
				ultraWide = true;
			}

			if( ultraWide ) {
				// drawn cursor, because system native cursor
				// is off-target on ultrawide displays

				setCursorMode( 1 );

				double startingScale = 1.0;

				int forceBigPointer =
						SettingsManager::getIntSetting( "forceBigPointer", 0 );
				if( forceBigPointer ||
					screenWidth > 1920 || screenHeight > 1080 ) {

					startingScale *= 2;
				}
				setEmulatedCursorScale( startingScale );
			}
		}
		else {
			setCursorMode( readCursorMode );

			double readCursorScale =
					SettingsManager::
					getDoubleSetting( "emulatedCursorScale", -1.0 );

			if( readCursorScale >= 1 ) {
				setEmulatedCursorScale( readCursorScale );
			}
		}

		frameDrawerInited = true;

		// this is a good time, a while after launch, to do the post
		// update step
		postUpdate();
	}
	else if( !writeFailed && !loadingFailedFlag  ) {
		// demo mode done or was never enabled
		// carry on with game

		// auto-pause when minimized
		if( pauseOnMinimize && screen->isMinimized() )
		{
			mPaused = true;
		}

		// don't update while paused
		char update = !mPaused;

		drawFrame( update );

		if( cursorMode > 0 )
		{
			// draw emulated cursor

			// draw using same projection used to drawFrame
			// so that emulated cursor lines up with screen position of buttons

			float xf, yf;
			screenToWorld( lastMouseX, lastMouseY, &xf, &yf );

			double x = xf;
			double y = yf;

			double sizeFactor = 25 * emulatedCursorScale;

			// white border of pointer

			setDrawColor( 1, 1, 1, 1 );

			double vertsA[18] =
					{ // body of pointer
							x, y,
							x, y - sizeFactor * 0.8918,
							x + sizeFactor * 0.6306, y - sizeFactor * 0.6306,
							// left collar of pointer
							x, y,
							x, y - sizeFactor * 1.0,
							x + sizeFactor * 0.2229, y - sizeFactor * 0.7994,
							// right collar of pointer
							x + sizeFactor * 0.4077, y - sizeFactor * 0.7229,
							x + sizeFactor * 0.7071, y - sizeFactor * 0.7071,
							x, y };

			drawTriangles( 3, vertsA );

			// neck of pointer
			double vertsB[8] = {
					x + sizeFactor * 0.2076, y - sizeFactor * 0.7625,
					x + sizeFactor * 0.376, y - sizeFactor * 1.169,
					x + sizeFactor * 0.5607, y - sizeFactor * 1.0924,
					x + sizeFactor * 0.3924, y - sizeFactor * 0.6859 };

			drawQuads( 1, vertsB );


			// black fill of pointer
			setDrawColor( 0, 0, 0, 1 );

			double vertsC[18] =
					{ // body of pointer
							x + sizeFactor * 0.04, y - sizeFactor * 0.0966,
							x + sizeFactor * 0.04, y - sizeFactor * 0.814,
							x + sizeFactor * 0.5473, y - sizeFactor * 0.6038,
							// left collar of pointer
							x + sizeFactor * 0.04, y - sizeFactor * 0.0966,
							x + sizeFactor * 0.04, y - sizeFactor * 0.9102,
							x + sizeFactor * 0.2382, y - sizeFactor * 0.7319,
							// right collar of pointer
							x + sizeFactor * 0.3491, y - sizeFactor * 0.6859,
							x + sizeFactor * 0.6153, y - sizeFactor * 0.6719,
							x + sizeFactor * 0.04, y - sizeFactor * 0.0966 };

			drawTriangles( 3, vertsC );

			// neck of pointer
			double vertsD[8] = {
					x + sizeFactor * 0.2229, y - sizeFactor * 0.6949,
					x + sizeFactor * 0.3976, y - sizeFactor * 1.1167,
					x + sizeFactor * 0.5086, y - sizeFactor * 1.0708,
					x + sizeFactor * 0.3338, y - sizeFactor * 0.649 };

			drawQuads( 1, vertsD );
		}

		if( recordAudio )
		{
			// frame-accurate audio recording
			int samplesPerFrame = soundSampleRate / targetFrameRate;

			// stereo 16-bit
			int bytesPerSample = 4;

			int numSampleBytes = bytesPerSample * samplesPerFrame;

			Uint8 *bytes = new Uint8[ numSampleBytes ];

			if( !bufferSizeHinted ) {
				hintBufferSize( numSampleBytes );

				soundSpriteMixingBufferL = new double[ samplesPerFrame ];
				soundSpriteMixingBufferR = new double[ samplesPerFrame ];

				bufferSizeHinted = true;
			}

			audioCallback( NULL, bytes, numSampleBytes );

			delete [] bytes;
		}

		if( screen->isPlayingBack() && screen->shouldShowPlaybackDisplay() )
		{
			char *progressString = autoSprintf(
					"%s %.1f\n%s\n%s",
					translate( "playbackTag" ),
					screen->getPlaybackDoneFraction() * 100,
					translate( "playbackToggleMessage" ),
					translate( "playbackEndMessage" ) );
			drawString( progressString );
			delete [] progressString;
		}

		if( screen->isPlayingBack() &&
			screen->shouldShowPlaybackDisplay() &&
			showMouseDuringPlayback() )
		{
			// draw mouse position info
			if( mouseDown )
			{
				if( isLastMouseButtonRight() ) {
					mouseRightDown = true;
					setDrawColor( 1, 0, 1, 0.5 );
				}
				else {
					mouseRightDown = false;
					setDrawColor( 1, 0, 0, 0.5 );
				}
			}
			else
			{
				setDrawColor( 1, 1, 1, 0.5 );
			}

			// step mouse click animation even after mouse released
			// (too hard to see it otherwise for fast clicks)
			mouseDownSteps ++;

			float sizeFactor = 5.0f;
			float clickSizeFactor = 5.0f;
			char showClick = false;
			float clickFade = 1.0f;

			int mouseClickDisplayDuration = 20 * targetFrameRate / 60.0;

			if( mouseDownSteps < mouseClickDisplayDuration )
			{
				float mouseClickProgress = mouseDownSteps / (float)mouseClickDisplayDuration;
				clickSizeFactor *= 5 * mouseClickProgress;
				showClick = true;
				clickFade *= 1.0f - mouseClickProgress;
			}

			// mouse coordinates in screen space
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			// viewport is square of largest dimension, centered on screen
			int bigDimension = screenWidth;
			if( screenHeight > bigDimension )
			{
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


			double verts[8] =
					{lastMouseX - sizeFactor, lastMouseY - sizeFactor,
					 lastMouseX - sizeFactor, lastMouseY + sizeFactor,
					 lastMouseX + sizeFactor, lastMouseY + sizeFactor,
					 lastMouseX + sizeFactor, lastMouseY - sizeFactor};

			drawQuads( 1, verts );


			double centerSize = 2;


			if( showClick ) {
				double clickVerts[8] =
						{lastMouseDownX - clickSizeFactor,
						 lastMouseDownY - clickSizeFactor,
						 lastMouseDownX - clickSizeFactor,
						 lastMouseDownY + clickSizeFactor,
						 lastMouseDownX + clickSizeFactor,
						 lastMouseDownY + clickSizeFactor,
						 lastMouseDownX + clickSizeFactor,
						 lastMouseDownY - clickSizeFactor};

				if( mouseDown ) {
					if( isLastMouseButtonRight() ) {
						setDrawColor( 1, 0, 1, clickFade );
					}
					else {
						setDrawColor( 1, 0, 0, clickFade );
					}
				}
				else {
					if( mouseRightDown ) {
						setDrawColor( 1, 1, 0, clickFade );
					}
					else {
						setDrawColor( 0, 1, 0, clickFade );
					}
				}

				drawQuads( 1, clickVerts );

				// draw pin-point at center of click
				double clickCenterVerts[8] =
						{lastMouseDownX - centerSize,
						 lastMouseDownY - centerSize,
						 lastMouseDownX - centerSize,
						 lastMouseDownY + centerSize,
						 lastMouseDownX + centerSize,
						 lastMouseDownY + centerSize,
						 lastMouseDownX + centerSize,
						 lastMouseDownY - centerSize};

				drawQuads( 1, clickCenterVerts );
			}


			// finally, darker black center over whole thing
			double centerVerts[8] =
					{lastMouseX - centerSize, lastMouseY - centerSize,
					 lastMouseX - centerSize, lastMouseY + centerSize,
					 lastMouseX + centerSize, lastMouseY + centerSize,
					 lastMouseX + centerSize, lastMouseY - centerSize};

			setDrawColor( 0, 0, 0, 0.5 );
			drawQuads( 1, centerVerts );

		}
	}

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

void GameSceneHandler::keyPressed(
		unsigned char inKey, int inX, int inY ) {

	if( writeFailed || loadingFailedFlag ) {
		exit( 0 );
	}

	if( measureFrameRate && measureRecorded ) {
		if( inKey == 'y' || inKey == 'Y' ) {
			saveFrameRateSettings();
			screen->startRecordingOrPlayback();
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


	if( inKey == 9 && isCommandKeyDown() &&
		screen->isPlayingBack() ) {

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
			mScreen->setMaxFrameRate( 2 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == '&' ) {
			// half
			mScreen->setMaxFrameRate( targetFrameRate / 2 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == '*' ) {
			// normal
			mScreen->setMaxFrameRate( targetFrameRate );

			if( countingOnVsync ) {
				mScreen->useFrameSleep( false );
			}
			else {
				mScreen->useFrameSleep( true );
			}
		}
		if( inKey == '(' ) {
			// fast forward
			mScreen->setMaxFrameRate( targetFrameRate * 2 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == ')' ) {
			// fast fast forward
			mScreen->setMaxFrameRate( targetFrameRate * 4 );
			mScreen->useFrameSleep( true );
		}
		if( inKey == '-' ) {
			// fast fast fast forward
			mScreen->setMaxFrameRate( targetFrameRate * 8 );
			mScreen->useFrameSleep( true );
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
