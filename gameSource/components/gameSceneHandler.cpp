//
// Created by olivier on 24/10/2021.
//

#include "gameSceneHandler.h"

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
#include "minorGems/game/game.h"//Uint8
#include "minorGems/graphics/Image.h"//bytes
#include "OneLife/gameSource/GamePage.h"
#include "OneLife/gameSource/procedures/graphics/drawString.h"
#include "OneLife/gameSource/dataTypes/web.h"
#include "OneLife/gameSource/musicPlayer.h"
#include "OneLife/gameSource/dataTypes/sound.h"
#include "OneLife/gameSource/objectBank.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/categoryBank.h"
#include "OneLife/gameSource/spriteBank.h"
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
#include "OneLife/gameSource/ServerActionPage.h"
#include "OneLife/gameSource/FinalMessagePage.h"
#include "OneLife/gameSource/LoadingPage.h"
#include "OneLife/gameSource/AutoUpdatePage.h"
#include "OneLife/gameSource/LivingLifePage.h"
#include "OneLife/gameSource/ExistingAccountPage.h"
#include "OneLife/gameSource/ExtendedMessagePage.h"
#include "OneLife/gameSource/RebirthChoicePage.h"
#include "OneLife/gameSource/SettingsPage.h"
#include "OneLife/gameSource/ReviewPage.h"
#include "OneLife/gameSource/TwinPage.h"
#include "OneLife/gameSource/PollPage.h"
#include "OneLife/gameSource/GeneticHistoryPage.h"


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
extern OneLife::game::Application *screen;
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
extern int idealTargetFrameRate;
extern int targetFrameRate;
extern char countingOnVsync;
extern int pixelZoomFactor;
extern int gameWidth;
extern int gameHeight;
extern int pauseOnMinimize;
extern int cursorMode;
extern double emulatedCursorScale;
extern int soundSampleRate;
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
char loadingMessageShown = false;
char loadingFailedFlag = false;
char *shutdownMessage = NULL;
int numPixelsDrawn = 0;

// show measure screen longer if there's a vsync warning
static double noWarningSecondsToMeasure = 1;
static double secondsToMeasure = noWarningSecondsToMeasure;
static float viewCenterX = 0;
static float viewCenterY = 0;
char mouseWorldCoordinates = true;
static float viewSize = 2;// default -1 to +1
int screenWidth = 640;
int screenHeight = 480;
float visibleWidth = -1;
float visibleHeight = -1;
static char ignoreNextMouseEvent = false;
static int mouseDownSteps = 1000;
static char mouseRightDown = false;
static char mouseDown = false;
static int lastMouseX = 0;
static int lastMouseY = 0;
static int lastMouseDownX = 0;
static int lastMouseDownY = 0;
static char measureRecorded = false;
static int numFramesSkippedBeforeMeasure = 0;
static int numFramesToSkipBeforeMeasure = 30;
static double startMeasureTime = 0;
static double warningSecondsToMeasure = 3;
static int numFramesMeasured = 0;
static char startMeasureTimeRecorded = false;
char frameDrawerInited = false;
static int xCoordToIgnore, yCoordToIgnore;// start with last click expired
static int nextShotNumber = -1;
static char shotDirExists = false;
static int outputFrameCount = 0;
static unsigned int frameNumber = 0;


GameSceneHandler::GameSceneHandler( OneLife::game::Application *inScreen )
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


	if( demoMode ) {

		if( ! isDemoCodePanelShowing() ) {

			// stop demo mode when panel done
			demoMode = false;

			mScreen->addMouseHandler( this );
			mScreen->addKeyboardHandler( this );

			screen->startRecordingOrPlayback();
		}
	}
	else if( writeFailed ) {
		drawString( translate( "writeFailed" ), true );
	}
	else if( !screen->isPlayingBack() && measureFrameRate ) {
		if( !measureRecorded ) {
			screen->useFrameSleep( false );
		}


		if( numFramesSkippedBeforeMeasure < numFramesToSkipBeforeMeasure ) {
			numFramesSkippedBeforeMeasure++;

			drawString( translate( "measuringFPS" ), true );
		}
		else if( ! startMeasureTimeRecorded ) {
			startMeasureTime = Time::getCurrentTime();
			startMeasureTimeRecorded = true;

			drawString( translate( "measuringFPS" ), true );
		}
		else {

			numFramesMeasured++;

			double totalTime = Time::getCurrentTime() - startMeasureTime;

			double timePerFrame = totalTime / ( numFramesMeasured );

			double frameRate = 1 / timePerFrame;


			int closestTargetFrameRate = 0;
			double closestFPSDiff = 9999999;

			for( int i=0; i<possibleFrameRates.size(); i++ ) {

				int v = possibleFrameRates.getElementDirect( i );

				double diff = fabs( frameRate - v );

				if( diff < closestFPSDiff ) {
					closestTargetFrameRate = v;
					closestFPSDiff = diff;
				}
			}

			double overAllowFactor = 1.05;



			if( numFramesMeasured > 10 &&
				frameRate > overAllowFactor * closestTargetFrameRate ) {

				secondsToMeasure = warningSecondsToMeasure;
			}
			else {
				secondsToMeasure = noWarningSecondsToMeasure;
			}

			if( totalTime <= secondsToMeasure ) {
				char *message = autoSprintf( "%s\n%0.2f\nFPS",
											 translate( "measuringFPS" ),
											 frameRate );


				drawString( message, true );

				delete [] message;
			}

			if( totalTime > secondsToMeasure ) {

				if( ! measureRecorded ) {

					if( targetFrameRate == idealTargetFrameRate ) {
						// not invoking halfFrameRate

						AppLog::infoF( "Measured frame rate = %f fps\n",
									   frameRate );
						AppLog::infoF(
								"Closest possible frame rate = %d fps\n",
								closestTargetFrameRate );

						if( frameRate >
							overAllowFactor * closestTargetFrameRate ) {

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
						else {
							AppLog::infoF(
									"Vsync seems to be enforcing an allowed frame "
									"rate of %d fps.\n", closestTargetFrameRate );

							targetFrameRate = closestTargetFrameRate;

							screen->useFrameSleep( false );
							countingOnVsync = true;
						}
					}
					else {
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

				if( !countingOnVsync ) {
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
				else {
					// auto-save it now
					saveFrameRateSettings();
					screen->startRecordingOrPlayback();
					measureFrameRate = false;
				}
			}
		}

		return;
	}
	else if( !loadingMessageShown ) {
		drawString( translate( "loading" ), true );

		loadingMessageShown = true;
	}
	else if( loadingFailedFlag ) {
		drawString( loadingFailedMessage, true );
	}
	else if( !writeFailed && !loadingFailedFlag && !frameDrawerInited ) {
		drawString( translate( "loading" ), true );

		initFrameDrawer( pixelZoomFactor * gameWidth,
						 pixelZoomFactor * gameHeight,
						 targetFrameRate,
						 screen->getCustomRecordedGameData(),
						 screen->isPlayingBack() );

		int readCursorMode = SettingsManager::getIntSetting( "cursorMode", -1 );


		if( readCursorMode < 0 ) {
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
		if( pauseOnMinimize && screen->isMinimized() ) {
			mPaused = true;
		}


		// don't update while paused
		char update = !mPaused;

		drawFrame( update );

		if( cursorMode > 0 ) {
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


		if( recordAudio ) {
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


		if( screen->isPlayingBack() && screen->shouldShowPlaybackDisplay() ) {

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
			showMouseDuringPlayback() ) {


			// draw mouse position info

			if( mouseDown ) {
				if( isLastMouseButtonRight() ) {
					mouseRightDown = true;
					setDrawColor( 1, 0, 1, 0.5 );
				}
				else {
					mouseRightDown = false;
					setDrawColor( 1, 0, 0, 0.5 );
				}
			}
			else {
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

			if( mouseDownSteps < mouseClickDisplayDuration ) {

				float mouseClickProgress =
						mouseDownSteps / (float)mouseClickDisplayDuration;

				clickSizeFactor *= 5 * mouseClickProgress;
				showClick = true;

				clickFade *= 1.0f - mouseClickProgress;
			}


			// mouse coordinates in screen space
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


	if( visibleWidth != -1 && visibleHeight != -1 ) {
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


	if( shouldTakeScreenshot ) {
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

static unsigned char lastKeyPressed = '\0';


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
	delete screen;


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

	if( screen->isPlayingBack() ) {
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


