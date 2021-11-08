//
// Created by olivier on 29/10/2021.
//

#include "application.h"

#include <SDL/SDL.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include "minorGems/graphics/openGL/SingleTextureGL.h"
#include "minorGems/graphics/openGL/glInclude.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"
#include "minorGems/system/Thread.h"
#include "minorGems/crypto/hashes/sha1.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"
#include "OneLife/gameSource/dataTypes/messages/keyboard.h"
#include "OneLife/gameSource/dataTypes/ui.h"
#include "OneLife/gameSource/components/keyboard.h"
#include "OneLife/gameSource/components/engines/deviceListener.h" //TODO: rename to gameScreenDeviceListener
#include "OneLife/gameSource/components/engines/screenRenderer.h"
#include "OneLife/gameSource/components/GamePage.h"

#ifdef __mac__
#include "minorGems/game/platforms/SDL/mac/SDLMain_Ext.h"
#endif

#ifdef RASPBIAN
#include "RaspbianGLSurface.cpp"
#endif

extern unsigned char keyMap[256];
extern char keyMapOn;
extern GameSceneHandler *sceneHandler;
extern GamePage *currentGamePage;
#include "OneLife/gameSource/components/pages/LivingLifePage.h"
extern LivingLifePage *livingLifePage;

//!globals screen selection
extern char demoMode;
extern char writeFailed;
extern SimpleVector<int> possibleFrameRates;
extern int idealTargetFrameRate;
extern int targetFrameRate;
extern char countingOnVsync;
extern char loadingFailedFlag;
extern char *loadingFailedMessage;
extern char frameDrawerInited;
extern int gameWidth;
extern int gameHeight;
extern int pixelZoomFactor;
extern int pauseOnMinimize;
extern int cursorMode;
extern double emulatedCursorScale;
extern char recordAudio;
extern int soundSampleRate;
extern char bufferSizeHinted;
extern double *soundSpriteMixingBufferL;
extern double *soundSpriteMixingBufferR;
extern int mouseDownSteps;
extern int screenWidth;
extern int screenHeight;
extern char measureFrameRate;

OneLife::game::Application *currentScreenGL;
long timeSinceLastFrameMS = 0;// FOVMOD NOTE:  Change 1/3 - Take these lines during the merge process
char screenGLStencilBufferSupported = false;
char measureRecorded = false;
char loadingMessageShown = false;

static int numFramesSkippedBeforeMeasure = 0;
static int numFramesToSkipBeforeMeasure = 30;
static char startMeasureTimeRecorded = false;
static double startMeasureTime = 0;
static int numFramesMeasured = 0;
static double noWarningSecondsToMeasure = 1;
static double secondsToMeasure = noWarningSecondsToMeasure;
static double warningSecondsToMeasure = 3;
static char mouseDown = false;//TODO: get mouse position
static char mouseRightDown = false;
static int lastMouseX = 0;
static int lastMouseY = 0;
static int lastMouseDownX = 0;
static int lastMouseDownY = 0;
static char mPaused = false;

OneLife::game::Application::Application(
		int inWide, int inHigh, char inFullScreen,
		char inDoNotChangeNativeResolution,
		unsigned int inMaxFrameRate,
		char inRecordEvents,
		const char *inCustomRecordedGameData,
		const char *inHashSalt,
		const char *inWindowName,
		KeyboardHandlerGL *inKeyHandler,
		MouseHandlerGL *inMouseHandler,
		SceneHandlerGL *inSceneHandler):

		mWide( inWide ), mHigh( inHigh ),
		mForceAspectRatio( false ),
		mForceSpecifiedDimensions( false ),
		mDoNotChangeNativeResolution( inDoNotChangeNativeResolution ),
		mImageWide( inWide ), mImageHigh( inHigh ),
		mFullScreen( inFullScreen ),
		mMaxFrameRate( inMaxFrameRate ),
		mUseFrameSleep( true ),
		mFullFrameRate( inMaxFrameRate ),
		m2DMode( false ),
		mViewPosition( new Vector3D( 0, 0, 0 ) ),
		mViewOrientation( new Angle3D( 0, 0, 0 ) ),
		mMouseHandlerVector( new SimpleVector<MouseHandlerGL*>() ),
		mKeyboardHandlerVector( new SimpleVector<KeyboardHandlerGL*>() ),
		mRedrawListenerVector( new SimpleVector<RedrawListenerGL*>() )
{
	this->connection = nullptr;
	this->screenRenderer = new OneLife::game::ScreenRenderer();
	this->quit = false;
	this->virtualKeyboard = new Onelife::dataType::message::Keyboard();

	mWantToMimimize = false;
	mMinimized = false;
	mWasFullScreenBeforeMinimize = false;

	mCustomRecordedGameData = stringDuplicate( inCustomRecordedGameData );

	mHashSalt = stringDuplicate( inHashSalt );


	mLastReadWebEventHandle = -1;
	mCurrentWebEventHandle = 0;
	mNextUnusedWebEventHandle = 0;

	mLastAsyncFileHandleDone = -1;

	mLastMinimizedStatus = false;

	mAllowSlowdownKeysDuringPlayback = false;

	// add handlers if NULL (the default) was not passed in for them
	if( inMouseHandler != NULL ) {
		addMouseHandler( inMouseHandler );
	}
	if( inKeyHandler != NULL ) {
		addKeyboardHandler( inKeyHandler );
	}


	mRandSeed = (unsigned int)fmod( Time::timeSec(), UINT_MAX );

	mLastTimeValue = Time::timeSec();
	mLastRecordedTimeValue = 0;

	mLastCurrentTimeValue = Time::getCurrentTime();
	mLastRecordedCurrentTimeValue = 0;

	mLastActualFrameRate = inMaxFrameRate;

	mTimeValuePlayedBack = false;
	mFramesSinceLastTimeTick = 0;

	mShouldShowPlaybackDisplay = true;

	int hidePlaybackDisplayFlag =
			SettingsManager::getIntSetting( "hidePlaybackDisplay", 0 );

	if( hidePlaybackDisplayFlag == 1 ) {
		mShouldShowPlaybackDisplay = false;
	}


	mRecordingOrPlaybackStarted = false;

	mRecordingEvents = inRecordEvents;
	mPlaybackEvents = false;
	mEventFile = NULL;
	mEventFileNumBatches = 0;
	mNumBatchesPlayed = 0;

	mObscureRecordedNumericTyping = false;

	// playback overrides recording, check for it first
	// do this before setting up surface

	File playbackDir( NULL, "playbackGame" );

	if( !playbackDir.exists() ) {
		playbackDir.makeDirectory();
	}

	int numChildren;
	File **childFiles = playbackDir.getChildFiles( &numChildren );

	if( numChildren > 0 ) {

		// take first
		char *fullFileName = childFiles[0]->getFullFileName();
		char *partialFileName = childFiles[0]->getFileName();

		// skip hidden files
		int i = 0;
		while( partialFileName != NULL &&
			   partialFileName[i] == '.' ) {

			delete [] fullFileName;
			fullFileName = NULL;

			delete [] partialFileName;
			partialFileName = NULL;

			i++;
			if( i < numChildren ) {
				fullFileName = childFiles[i]->getFullFileName();
				partialFileName = childFiles[i]->getFileName();
			}
		}


		if( fullFileName != NULL ) {
			delete [] partialFileName;



			mEventFile = fopen( fullFileName, "r" );

			if( mEventFile == NULL ) {
				AppLog::error( "Failed to open event playback file" );
			}
			else {

				// count number of newlines in file (close to the number
				// of batches in the file)
				char *fileContents = childFiles[i]->readFileContents();

				int fileLength = strlen( fileContents );

				for( int j=0; j<fileLength; j++ ) {
					if( fileContents[j] == '\n' ) {
						mEventFileNumBatches ++;
					}
				}
				delete [] fileContents;


				AppLog::getLog()->logPrintf(
						Log::INFO_LEVEL,
						"Playing back game from file %s", fullFileName );


				// first, determine max possible length of custom data
				int maxCustomLength = 0;

				int readChar = fgetc( mEventFile );

				while( readChar != EOF && readChar != '\n' ) {
					maxCustomLength++;
					readChar = fgetc( mEventFile );
				}

				// back to start
				rewind( mEventFile );

				char *readCustomGameData = new char[ maxCustomLength ];

				char hashString[41];



				int fullScreenFlag;
				unsigned int readRandSeed;
				unsigned int readMaxFrameRate;
				int readWide;
				int readHigh;

				int numScanned =
						fscanf(
								mEventFile,
								"%u seed, %u fps, %dx%d, fullScreen=%d, %s %40s\n",
								&readRandSeed,
								&readMaxFrameRate,
								&readWide, &readHigh, &fullScreenFlag,
								readCustomGameData,
								hashString );

				if( numScanned == 7 ) {

					char *stringToHash = autoSprintf( "%s%s",
							readCustomGameData,
							mHashSalt );

					char *correctHash = computeSHA1Digest( stringToHash );

					delete [] stringToHash;

					int difference = strcmp( correctHash, hashString );

					delete [] correctHash;

					if( difference == 0 ) {

						mRecordingEvents = false;
						mPlaybackEvents = true;

						mRandSeed = readRandSeed;
						mMaxFrameRate = readMaxFrameRate;
						mWide = readWide;
						mHigh = readHigh,

								mFullFrameRate = mMaxFrameRate;

						mImageWide = mWide;
						mImageHigh = mHigh;

						AppLog::info(
								"Forcing dimensions specified in playback file" );
						mForceSpecifiedDimensions = true;


						if( fullScreenFlag ) {
							mFullScreen = true;
						}
						else {
							mFullScreen = false;
						}

						delete [] mCustomRecordedGameData;
						mCustomRecordedGameData =
								stringDuplicate( readCustomGameData );
					}
					else {
						AppLog::error(
								"Hash check failed for custom data in playback file" );
					}
				}
				else {
					AppLog::error(
							"Failed to parse playback header data" );

				}
				delete [] readCustomGameData;
			}
			delete [] fullFileName;
		}


		for( int i=0; i<numChildren; i++ ) {
			delete childFiles[i];
		}
	}
	delete [] childFiles;





	mStartedFullScreen = mFullScreen;

	setupSurface();


	SDL_WM_SetCaption( inWindowName, NULL );


	// turn off repeat
	SDL_EnableKeyRepeat( 0, 0 );

	SDL_EnableUNICODE( true );

	for( int i=0; i<256; i++ ) {
		keyMap[i] = (unsigned char)i;
	}
}

OneLife::game::Application::~Application()
{
	/*
	this->screenGL->~ScreenGL();
	 */
	delete mViewPosition;
	delete mViewOrientation;
	delete mRedrawListenerVector;
	delete mMouseHandlerVector;
	delete mKeyboardHandlerVector;
	//delete mSceneHandlerVector;

	if( mRecordingEvents && mRecordingOrPlaybackStarted ) {
		writeEventBatchToFile();
	}

	if( mEventFile != NULL ) {
		fclose( mEventFile );
		mEventFile = NULL;
	}


	delete [] mCustomRecordedGameData;
	delete [] mHashSalt;


	for( int i=0; i<mPendingWebEvents.size(); i++ ) {
		WebEvent *e = mPendingWebEvents.getElement( i );

		if( e->bodyText != NULL ) {

			delete [] e->bodyText;

			e->bodyText = NULL;
		}

	}
	for( int i=0; i<mPendingSocketEvents.size(); i++ ) {
		SocketEvent *e = mPendingSocketEvents.getElement( i );

		if( e->bodyBytesHex != NULL ) {

			delete [] e->bodyBytesHex;

			e->bodyBytesHex = NULL;
		}

	}
}

void OneLife::game::Application::setConnection()
{
	/*TODO: uncomment when modification is finished
	this->connection = new OneLife::game::component::Socket(
			&serverSocketBuffer,
			&bytesInCount);
	*/
}

OneLife::game::component::Socket* OneLife::game::Application::getConnection()
{
	return this->connection;
}

/**
* Starts the GLUT main loop.
*
* Note that this function call never returns.
*/
void OneLife::game::Application::start()
{
	currentScreenGL = this;

	// call our resize callback (GLUT used to do this for us when the
	// window was created)
	callbackResize( mWide, mHigh );


	// oversleep on last loop (discount it from next sleep)
	// can be negative (add to next sleep)
	int oversleepMSec = 0;


	// FOVMOD NOTE:  Change 2/3 - Take these lines during the merge process
	timeSec_t frameStartSec;
	unsigned long frameStartMSec;
	unsigned long oldFrameStart;

	Time::getCurrentTime( &frameStartSec, &frameStartMSec );

	OneLife::dataType::ui::Screen dataScreen;


	// main loop
	while( !this->quit )
	{
		oldFrameStart = frameStartMSec;

		Time::getCurrentTime( &frameStartSec, &frameStartMSec );

		timeSinceLastFrameMS = frameStartMSec - oldFrameStart;
		if( timeSinceLastFrameMS < 0 ) {
			timeSinceLastFrameMS = 0;
		}

		// pre-display first, this might involve a sleep for frame timing
		// purposes
		callbackPreDisplay();


		this->readDevicesStatus();

		// now all events handled, actually draw the screen
		this->selectScreen();

		if( ! currentScreenGL->m2DMode ) {
			// apply our view transform
			currentScreenGL->applyViewTransform();
		}

		this->update(&dataScreen);

		this->render(&dataScreen);

		for( int r=0; r<currentScreenGL->mRedrawListenerVector->size(); r++ )
		{
			RedrawListenerGL *listener = *( currentScreenGL->mRedrawListenerVector->getElement( r ) );
			listener->postRedraw();
		}

#ifdef RASPBIAN
		raspbianSwapBuffers();
#else
		SDL_GL_SwapBuffers();
#endif

		// thanks to Andrew McClure for the idea of doing this AFTER
		// the next redraw (for pretty minimization)
		if( currentScreenGL->mWantToMimimize ) {
			currentScreenGL->mWantToMimimize = false;
			SDL_WM_IconifyWindow();
			currentScreenGL->mMinimized = true;
		}



		// record them?
		// do this down here, AFTER display, since some events might be
		// triggered by the drawing code (example:  web requests and results)
		if( mRecordingEvents && mRecordingOrPlaybackStarted ) {
			writeEventBatchToFile();
		}


		int frameTime = Time::getMillisecondsSince( frameStartSec, frameStartMSec );


		// frame time should never be negative
		// BUT it can be if system time changes while game is running
		// (example:  automatic daylight savings time adjustment)
		if( frameTime < 0 ) {
			frameTime = 0;
		}


		if( mUseFrameSleep ) {
			// lock down to mMaxFrameRate frames per second
			int minFrameTime = 1000 / mMaxFrameRate;
			if( ( frameTime + oversleepMSec ) < minFrameTime ) {
				int timeToSleep =
						minFrameTime - ( frameTime + oversleepMSec );

				//SDL_Delay( timeToSleep );
				timeSec_t sleepStartSec;
				unsigned long sleepStartMSec;
				Time::getCurrentTime( &sleepStartSec, &sleepStartMSec );

				Thread::staticSleep( timeToSleep );

				int actualSleepTime =
						Time::getMillisecondsSince( sleepStartSec, sleepStartMSec );

				oversleepMSec = actualSleepTime - timeToSleep;
			}
			else {
				oversleepMSec = 0;
			}
		}


	}
}

/**********************************************************************************************************************/
//!private

void OneLife::game::Application::readDevicesStatus()
{
	this->virtualKeyboard->reset();

	/*
	this->deviceListener->listen();
	for(int i=0; i<this->deviceListener->getEvent()->size(); i++)
	{

	}
 	*/



	// now handle pending events BEFORE actually drawing the screen.
	// Thus, screen reflects all the latest events (not just those
	// that happened before any sleep called during the pre-display).
	// This makes controls much more responsive.

	SDL_Event event;

	while( !( mPlaybackEvents && mRecordingOrPlaybackStarted ) && SDL_PollEvent( &event ) ) {

		SDLMod mods = SDL_GetModState();

		// alt-enter, toggle fullscreen (but only if we started there,
		// to prevent window content centering issues due to mWidth and
		// mHeight changes mid-game)
		if( mStartedFullScreen &&
			event.type == SDL_KEYDOWN &&
			event.key.keysym.sym == SDLK_RETURN &&
			( ( mods & KMOD_META ) || ( mods & KMOD_ALT ) ) ) {
			printf( "Toggling fullscreen\n" );

			mFullScreen = !mFullScreen;

			setupSurface();

			callbackResize( mWide, mHigh );

			// reload all textures into OpenGL
			SingleTextureGL::contextChanged();
		}
			// alt-tab when not in fullscreen mode
		else if( ! mFullScreen &&
				 ! mMinimized &&
				 event.type == SDL_KEYDOWN &&
				 event.key.keysym.sym == SDLK_TAB &&
				 ( ( mods & KMOD_META ) || ( mods & KMOD_ALT ) ) ) {

			mWantToMimimize = true;
			mWasFullScreenBeforeMinimize = false;

			if( SDL_WM_GrabInput( SDL_GRAB_QUERY ) == SDL_GRAB_ON ) {
				mWasInputGrabbedBeforeMinimize = true;
			}
			else {
				mWasInputGrabbedBeforeMinimize = false;
			}
			SDL_WM_GrabInput( SDL_GRAB_OFF );

			// record TAB keystroke so that it's properly
			// played back
			if( mRecordingEvents &&
				mRecordingOrPlaybackStarted ) {

				int mouseX, mouseY;
				SDL_GetMouseState( &mouseX, &mouseY );
				char *eventString = autoSprintf( "kd %d %d %d",
						9, mouseX, mouseY );

				mUserEventBatch.push_back( eventString );
			}
		}
			// handle alt-tab to minimize out of full-screen mode
		else if( mFullScreen &&
				 ! mMinimized &&
				 event.type == SDL_KEYDOWN &&
				 event.key.keysym.sym == SDLK_TAB &&
				 ( ( mods & KMOD_META ) || ( mods & KMOD_ALT ) ) ) {

			printf( "Minimizing from fullscreen on Alt-tab\n" );

			mFullScreen = false;

			setupSurface();

			callbackResize( mWide, mHigh );

			// reload all textures into OpenGL
			SingleTextureGL::contextChanged();

			mWantToMimimize = true;
			mWasFullScreenBeforeMinimize = true;

			if( SDL_WM_GrabInput( SDL_GRAB_QUERY ) == SDL_GRAB_ON ) {
				mWasInputGrabbedBeforeMinimize = true;
			}
			else {
				mWasInputGrabbedBeforeMinimize = false;
			}
			SDL_WM_GrabInput( SDL_GRAB_OFF );

			// record TAB keystroke so that it's properly
			// played back
			if( mRecordingEvents &&
				mRecordingOrPlaybackStarted ) {

				int mouseX, mouseY;
				SDL_GetMouseState( &mouseX, &mouseY );
				char *eventString = autoSprintf( "kd %d %d %d",
						9, mouseX, mouseY );

				mUserEventBatch.push_back( eventString );
			}
		}
			// active event after minimizing from windowed mode
		else if( mMinimized &&
				 ! mWasFullScreenBeforeMinimize &&
				 event.type == SDL_ACTIVEEVENT &&
				 event.active.gain &&
				 event.active.state == SDL_APPACTIVE ) {
			// window becoming active out of minimization, needs
			// to return to full-screen mode

			printf( "Restoring to window after Alt-tab\n" );

			mWantToMimimize = false;
			mWasFullScreenBeforeMinimize = false;
			mMinimized = false;

			if( mWasInputGrabbedBeforeMinimize ) {
				SDL_WM_GrabInput( SDL_GRAB_ON );
			}
		}
			// active event after minimizing from fullscreen mode
		else if( mMinimized &&
				 mWasFullScreenBeforeMinimize &&
				 event.type == SDL_ACTIVEEVENT &&
				 event.active.gain &&
				 event.active.state == SDL_APPACTIVE ) {
			// window becoming active out of minimization, needs
			// to return to full-screen mode

			printf( "Restoring to fullscreen after Alt-tab\n" );

			mFullScreen = true;

			setupSurface();

			callbackResize( mWide, mHigh );

			// reload all textures into OpenGL
			SingleTextureGL::contextChanged();

			mWantToMimimize = false;
			mWasFullScreenBeforeMinimize = false;
			mMinimized = false;

			if( mWasInputGrabbedBeforeMinimize ) {
				SDL_WM_GrabInput( SDL_GRAB_ON );
			}
		}
			// map CTRL-q to ESC
			// 17 is "DC1" which is ctrl-q on some platforms
		else if( event.type == SDL_KEYDOWN &&
				 ( ( event.key.keysym.sym == SDLK_q
					 &&
					 ( ( mods & KMOD_META ) || ( mods & KMOD_ALT )
					   || ( mods & KMOD_CTRL ) ) )
				   ||
				   ( ( event.key.keysym.unicode & 0xFF ) == 17 ) ) ) {

			// map to 27, escape
			int mouseX, mouseY;
			SDL_GetMouseState( &mouseX, &mouseY );

			callbackKeyboard( 27, mouseX, mouseY );
		}
		else {


			switch( event.type ) {
				case SDL_QUIT: {
					// map to 27, escape
					int mouseX, mouseY;
					SDL_GetMouseState( &mouseX, &mouseY );

					callbackKeyboard( 27, mouseX, mouseY );
				}
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP: {
					int mouseX, mouseY;
					SDL_GetMouseState( &mouseX, &mouseY );


					// check if special key
					int mgKey = mapSDLSpecialKeyToMG( event.key.keysym.sym );

					if( mgKey != 0 ) {
						if( event.type == SDL_KEYDOWN ) {
							callbackSpecialKeyboard( mgKey, mouseX, mouseY );
						}
						else {
							callbackSpecialKeyboardUp( mgKey, mouseX, mouseY );
						}
					}
					else {
						unsigned char asciiKey;

						// try unicode first, if 8-bit clean (extended ASCII)
						if( ( event.key.keysym.unicode & 0xFF00 ) == 0 &&
							( event.key.keysym.unicode & 0x00FF ) != 0 ) {
							asciiKey = event.key.keysym.unicode & 0xFF;
						}
						else {
							// else unicode-to-ascii failed

							// fall back
							asciiKey =
									mapSDLKeyToASCII( event.key.keysym.sym );
						}


						if( asciiKey != 0 ) {
							// shift and caps cancel each other
							if( ( ( event.key.keysym.mod & KMOD_SHIFT )
								  &&
								  !( event.key.keysym.mod & KMOD_CAPS ) )
								||
								( !( event.key.keysym.mod & KMOD_SHIFT )
								  &&
								  ( event.key.keysym.mod & KMOD_CAPS ) ) ) {

								asciiKey = toupper( asciiKey );
							}

							if( event.type == SDL_KEYDOWN ) {
								callbackKeyboard( asciiKey, mouseX, mouseY );
							}
							else {
								callbackKeyboardUp( asciiKey, mouseX, mouseY );
							}
						}
					}
				}
					break;
				case SDL_MOUSEMOTION:
					if( event.motion.state & SDL_BUTTON( 1 )
						||
						event.motion.state & SDL_BUTTON( 2 )
						||
						event.motion.state & SDL_BUTTON( 3 ) ) {

						callbackMotion( event.motion.x, event.motion.y );
					}
					else {
						callbackPassiveMotion( event.motion.x,
								event.motion.y );
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					callbackMouse( event.button.button,
							event.button.state,
							event.button.x,
							event.button.y );
					break;
			}
		}

	}

	if( mPlaybackEvents && mRecordingOrPlaybackStarted &&
		mEventFile != NULL ) {


		if( !mTimeValuePlayedBack ) {

			// so far, no time values have been played back yet.
			// (as a fix for earlier release that did not
			// record time), fix time() values to go along with specified
			// frame rate in recording file (so that a game played on a
			// machine fast enough for 60fps will behave close to the
			// same, time()-wise, on a machine that can't play the game
			// back at 60fps).

			mFramesSinceLastTimeTick ++;

			if( mFramesSinceLastTimeTick >= mFullFrameRate ) {
				mFramesSinceLastTimeTick = 0;
				mLastTimeValue ++;
				mLastCurrentTimeValue += 1.0;
			}
		}


		// this may overwrite the mLastTimeValue that we're emulating
		// if this recorded frame involved a recorded time() call.
		playNextEventBatch();


		// dump events, but responde to ESC to stop playback
		// let player take over from that point
		while( SDL_PollEvent( &event ) ) {
			SDLMod mods = SDL_GetModState();
			// map CTRL-q to ESC
			// 17 is "DC1" which is ctrl-q on some platforms
			if( event.type == SDL_KEYDOWN &&
				( ( event.key.keysym.sym == SDLK_q
					&&
					( ( mods & KMOD_META ) || ( mods & KMOD_ALT )
					  || ( mods & KMOD_CTRL ) ) )
				  ||
				  ( ( event.key.keysym.unicode & 0xFF ) == 17 ) ) ) {

				// map to 27, escape
				int mouseX, mouseY;
				SDL_GetMouseState( &mouseX, &mouseY );

				printf( "User terminated recorded event playback "
						"with ESC\n" );

				// stop playback
				mPlaybackEvents = false;
			}
			else {
				switch( event.type ) {
					case SDL_QUIT: {
						// map to 27, escape
						int mouseX, mouseY;
						SDL_GetMouseState( &mouseX, &mouseY );

						// actual quit event, still pass through
						// as ESC to signal a full quit
						callbackKeyboard( 27, mouseX, mouseY );
					}
						break;
					case SDL_KEYDOWN: {

						unsigned char asciiKey;

						// try unicode first,
						// if 8-bit clean (extended ASCII)
						if( ( event.key.keysym.unicode & 0xFF00 ) == 0 &&
							( event.key.keysym.unicode & 0x00FF ) != 0 ) {
							asciiKey = event.key.keysym.unicode & 0xFF;
						}
						else {
							// else unicode-to-ascii failed

							// fall back
							asciiKey = mapSDLKeyToASCII( event.key.keysym.sym );
						}

						if( asciiKey == 27 ) {
							// pass ESC through
							// map to 27, escape
							int mouseX, mouseY;
							SDL_GetMouseState( &mouseX, &mouseY );

							printf("User terminated recorded event playback " "with ESC\n" );

							// stop playback
							mPlaybackEvents = false;
						}
						else if( asciiKey == '%' ) {
							mShouldShowPlaybackDisplay =
									! mShouldShowPlaybackDisplay;
						}
						if( mAllowSlowdownKeysDuringPlayback ) {

							if( asciiKey == '^' ) {
								setMaxFrameRate( 2 );
							}
							else if( asciiKey == '&' ) {
								setMaxFrameRate( mFullFrameRate );
							}
							else if( asciiKey == '*' ) {
								// fast forward
								setMaxFrameRate( mFullFrameRate * 2 );
							}
							else if( asciiKey == '(' ) {
								// fast fast forward
								setMaxFrameRate( mFullFrameRate * 4 );
							}
							else if( asciiKey == ')' ) {
								// fast fast fast forward
								setMaxFrameRate( mFullFrameRate * 8 );
							}
						}
					}
				}
			}

		}

		if( !mPlaybackEvents ) {
			// playback ended
			// send through full spectrum of release events
			// so no presses linger after playback end

			int mouseX, mouseY;
			SDL_GetMouseState( &mouseX, &mouseY );
			callbackMouse( SDL_BUTTON_LEFT,
					SDL_RELEASED, mouseX, mouseY );

			callbackMouse( SDL_BUTTON_MIDDLE,
					SDL_RELEASED, mouseX, mouseY );

			callbackMouse( SDL_BUTTON_RIGHT,
					SDL_RELEASED, mouseX, mouseY );

			callbackMouse( SDL_BUTTON_WHEELUP,
					SDL_RELEASED, mouseX, mouseY );

			callbackMouse( SDL_BUTTON_WHEELDOWN,
					SDL_RELEASED, mouseX, mouseY );

			for( int i=0; i<255; i++ ) {
				callbackKeyboardUp( i, mouseX, mouseY );
			}
			for( int i=MG_KEY_FIRST_CODE; i<=MG_KEY_LAST_CODE; i++ ) {
				callbackSpecialKeyboardUp( i, mouseX, mouseY );
			}
		}


	}
}

void OneLife::game::Application::readServerMessage()
{
	//printf("\n==========>read server message");
}

void OneLife::game::Application::selectScreen()
{
	//printf("\n==========>select screen");

	//!SEARCH LEG000001 for legacy place
	if( demoMode )
	{
		if( ! isDemoCodePanelShowing() )
		{
			demoMode = false;// stop demo mode when panel done
			//mScreen->addMouseHandler( this );
			//mScreen->addKeyboardHandler( this );
			//screen->startRecordingOrPlayback();
		}
	}
	else if( writeFailed )
	{
		drawString( translate( "writeFailed" ), true );
	}
	else if( !this->isPlayingBack() && measureFrameRate ) {
		if( !measureRecorded )
		{
			this->useFrameSleep( false );
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

							this->useFrameSleep( true );
							countingOnVsync = false;
						}
						else {
							AppLog::infoF(
									"Vsync seems to be enforcing an allowed frame "
									"rate of %d fps.\n", closestTargetFrameRate );

							targetFrameRate = closestTargetFrameRate;

							this->useFrameSleep( false );
							countingOnVsync = true;
						}
					}
					else {
						// half frame rate must be set

						AppLog::infoF(
								"User has halfFrameRate set, so we're going "
								"to manually sleep to enforce a target "
								"frame rate of %d fps.\n", targetFrameRate );
						this->useFrameSleep( true );
						countingOnVsync = false;
					}


					this->setFullFrameRate( targetFrameRate );
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
					this->startRecordingOrPlayback();
					measureFrameRate = false;
				}
			}
		}
		//return;
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
				this->getCustomRecordedGameData(),
				this->isPlayingBack() );//!currentGamePage initialized inside

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
	else if( !writeFailed && !loadingFailedFlag  )// demo mode done or was never enabled
	{
		if( pauseOnMinimize && this->isMinimized() ) mPaused = true;// auto-pause when minimized
		char update = !mPaused;//TODO: paused is triggered in gameSceneHandler => change this! // don't update while paused

		//printf("\n==========>render scene !!!!");
		drawFrame( update );//TODO: screenSelection separation <========================================================
		/**************************************************************************************************************/
		if( cursorMode > 0 ) {
			int lastMouseX = 0;//TODO: get last mouse position
			int lastMouseY = 0;//TODO: get last mouse position
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

		if( this->isPlayingBack() && this->shouldShowPlaybackDisplay() )
		{

			char *progressString = autoSprintf(
					"%s %.1f\n%s\n%s",
					translate( "playbackTag" ),
					this->getPlaybackDoneFraction() * 100,
					translate( "playbackToggleMessage" ),
					translate( "playbackEndMessage" ) );

			drawString( progressString );

			delete [] progressString;

		}

		if( this->isPlayingBack() && this->shouldShowPlaybackDisplay() && showMouseDuringPlayback() )
		{
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
		/**************************************************************************************************************/
	}

	sceneHandler->drawScene();//TODO: screenHandler/this->handle/update(screen)->readMessage(message);
}

void OneLife::game::Application::update(OneLife::dataType::ui::Screen* dataScreen)
{
	if(!currentGamePage) return;
	currentGamePage->handle(dataScreen);
	//printf("\n==========>update %s", dataScreen->label);
}

void OneLife::game::Application::render(OneLife::dataType::ui::Screen* dataScreen)
{
	this->screenRenderer->render();
}

void OneLife::game::Application::sendClientMessage() {}


/**
* Gets data read from a recorded game file.
*
* Not destroyed by caller.
*/
const char* OneLife::game::Application::getCustomRecordedGameData()
{
	//return this->screenGL->getCustomRecordedGameData();
	return mCustomRecordedGameData;
}

/**
* True if currently in playback mode.
*/
char OneLife::game::Application::isPlayingBack()
{
	//return this->screenGL->isPlayingBack();
	return mPlaybackEvents;
}

/**
* Returns an estimate of playback fraction complete.
*/
float OneLife::game::Application::getPlaybackDoneFraction()
{
	//return this->screenGL->getPlaybackDoneFraction();
	if( mEventFileNumBatches == 0 || mEventFile == NULL ) {
		return 0;
	}

	return mNumBatchesPlayed / (float)mEventFileNumBatches;
}

/**
* Returns whether playback display is on or off.
*/
char OneLife::game::Application::shouldShowPlaybackDisplay()
{
	//return this->screenGL->shouldShowPlaybackDisplay();
	return mShouldShowPlaybackDisplay;
}

/**
* True if minimized.
*/
char OneLife::game::Application::isMinimized()
{
	//return this->screenGL->isMinimized();
	// we use mMinimized internally to keep track of whether we ever
	// minimized ourself and were properly restored from that minimization
	// (to do the things that we need to do when we come out of minimization)

	// HOWEVER, on some platforms, we don't receive proper events when we
	// come out of minimization, so we can't count on mMinimized to be
	// correct.

	// Actually test for real here.



	if( mPlaybackEvents && mRecordingOrPlaybackStarted &&
		mEventFile != NULL ) {


		return mLastMinimizedStatus;
	}


	// non-playback behavior
	char isMin = ( ( SDL_GetAppState() & SDL_APPACTIVE ) == 0 );

	if( isMin &&
		mRecordingEvents &&
		mRecordingOrPlaybackStarted ) {

		// record it

		char *eventString = stringDuplicate( "v" );

		mEventBatch.push_back( eventString );
	}


	return isMin;
}

// can avoid recording/playback during certain "front matter"
// activities like menu navigation
// (has no effect if no recording or playback pending)
void OneLife::game::Application::startRecordingOrPlayback()
{
	//this->screenGL->startRecordingOrPlayback();

	if( mRecordingEvents ) {
		File recordedGameDir( NULL, "recordedGames" );

		if( !recordedGameDir.exists() ) {
			recordedGameDir.makeDirectory();
		}


		// find next event recording file
		int fileNumber = 0;

		// find max existing file number
		int numFiles = 0;
		File **childFiles = recordedGameDir.getChildFiles( &numFiles );

		for( int f=0; f<numFiles; f++ ) {
			char *fileName = childFiles[f]->getFileName();

			int n = -1;
			sscanf( fileName, "recordedGame%d.txt", &n );

			if( n > fileNumber ) {
				fileNumber = n;
			}
			delete childFiles[f];

			delete [] fileName;
		}
		delete [] childFiles;

		// next file number in sequence, after max found
		fileNumber++;

		char *fileName = autoSprintf( "recordedGame%06d.txt",
				fileNumber );
		File *file = recordedGameDir.getChildFile( fileName );

		delete [] fileName;

		char *fullFileName = file->getFullFileName();

		mEventFile = fopen( fullFileName, "w" );

		if( mEventFile == NULL ) {
			AppLog::error( "Failed to open event recording file" );
		}
		else {
			AppLog::getLog()->logPrintf(
					Log::INFO_LEVEL,
					"Recording game into file %s", fullFileName );

			int fullScreenFlag = 0;
			if( mFullScreen ) {
				fullScreenFlag = 1;
			}

			char *stringToHash = autoSprintf( "%s%s",
					mCustomRecordedGameData,
					mHashSalt );

			char *correctHash = computeSHA1Digest( stringToHash );

			delete [] stringToHash;


			fprintf( mEventFile,
					"%u seed, %u fps, %dx%d, fullScreen=%d, %s %s\n",
					mRandSeed,
					mMaxFrameRate, mWide, mHigh, fullScreenFlag,
					mCustomRecordedGameData,
					correctHash );

			delete [] correctHash;


			delete [] fullFileName;
		}
		delete file;



		int keepNumber = SettingsManager::getIntSetting( "keepPastRecordings",
				-1 );
		if( keepNumber >= 0 ) {

			AppLog::getLog()->logPrintf(
					Log::INFO_LEVEL,
					"Only keeping %d past recordings", keepNumber );

			int cutOffNumber = fileNumber - keepNumber;

			int numRemoved = 0;

			for( int f=1; f<cutOffNumber; f++ ) {
				// handle removing both old 5-digit format and new 6-digit
				// format
				char *fileName = autoSprintf( "recordedGame%05d.txt", f );
				File *file = recordedGameDir.getChildFile( fileName );

				delete [] fileName;

				if( file->exists() ) {
					file->remove();
					numRemoved++;
				}
				delete file;

				fileName = autoSprintf( "recordedGame%06d.txt", f );
				file = recordedGameDir.getChildFile( fileName );

				delete [] fileName;

				if( file->exists() ) {
					file->remove();
					numRemoved++;
				}
				delete file;
			}
			AppLog::getLog()->logPrintf(
					Log::INFO_LEVEL,
					"Removed %d stale recordings", numRemoved );
		}
	}



	mRecordingOrPlaybackStarted = true;
}

// our current max
// can be changed with speed control keys, if enabled
void OneLife::game::Application::setMaxFrameRate( unsigned int inMaxFrameRate )
{
	//this->screenGL->setMaxFrameRate(inMaxFrameRate);
	mMaxFrameRate = inMaxFrameRate;
}

// our absolute max
// speed control key adjustments are relative to this full rate
void OneLife::game::Application::setFullFrameRate( unsigned int inFullFrameRate )
{
	//this->screenGL->setFullFrameRate(inFullFrameRate);
	mFullFrameRate = inFullFrameRate;
	mMaxFrameRate = mFullFrameRate;
}


unsigned int OneLife::game::Application::getMaxFramerate()
{
	//return this->screenGL->getMaxFramerate();
	return mMaxFrameRate;
}

// set to true to enable frame sleeping to enforce max frame rate
// (sleeping is default)
// Otherwise, if false, no max is enforced (though we may be able
// to depend on vsync to enforce it for us)
void OneLife::game::Application::useFrameSleep( char inUse )
{
	//this->screenGL->useFrameSleep(inUse);
	mUseFrameSleep = inUse;
}



// should ^ and % be allowed to slowdown and resume normal speed
// during event playback?
// setMaxFrameRate also can be used to create a slowdown, but
// ^ and % keys are not passed through during playback
void OneLife::game::Application::allowSlowdownKeysDuringPlayback( char inAllow )
{
	//this->screenGL->allowSlowdownKeysDuringPlayback(inAllow);
	mAllowSlowdownKeysDuringPlayback = inAllow;
}


// enables rand seed to be recorded and played back with
// event playback
unsigned int OneLife::game::Application::getRandSeed()
{
	//return this->screenGL->getRandSeed();
	return mRandSeed;
}


// enables Time::timeSec() values to be recorded and played back
timeSec_t OneLife::game::Application::getTimeSec()
{
	//return this->screenGL->getTimeSec();
	if( mPlaybackEvents && mRecordingOrPlaybackStarted &&
		mEventFile != NULL ) {

		if( mLastTimeValueStack.size() > 0 ) {
			timeSec_t t = mLastTimeValueStack.getElementDirect( 0 );
			mLastTimeValueStack.deleteElement( 0 );
			return t;
		}
		else {
			return mLastTimeValue;
		}
	}


	// else just normal behavior (platform's time() output)
	timeSec_t currentTime = Time::timeSec();

	if( mRecordingEvents &&
		mRecordingOrPlaybackStarted ) {

		// record it if it is different from the last value that we recorded
		// thus, 't' events are sparse in our event file (and not repeated
		// for every frame, or multiple times per frame, even if get_time()
		// is called a lot).

		if( currentTime != mLastRecordedTimeValue ) {

			char *eventString = autoSprintf( "t %.f", currentTime );

			mEventBatch.push_back( eventString );

			mLastRecordedTimeValue = currentTime;
		}
		else {
			// repeat, record short string to indicate this
			char *eventString = stringDuplicate( "r" );

			mEventBatch.push_back( eventString );
		}
	}


	return currentTime;
}

double OneLife::game::Application::getCurrentTime()
{
	//return this->screenGL->getCurrentTime();
	if( mPlaybackEvents && mRecordingOrPlaybackStarted &&
		mEventFile != NULL ) {

		if( mLastCurrentTimeValueStack.size() > 0 ) {
			double t = mLastCurrentTimeValueStack.getElementDirect( 0 );
			mLastCurrentTimeValueStack.deleteElement( 0 );
			return t;
		}
		else {
			return mLastCurrentTimeValue;
		}
	}


	// else just normal behavior (platform's output)
	double currentTime = Time::getCurrentTime();

	if( mRecordingEvents &&
		mRecordingOrPlaybackStarted ) {

		// record it if it is different from the last value that we recorded
		// thus, 't' events are sparse in our event file (and not repeated
		// for every frame, or multiple times per frame, even if get_time()
		// is called a lot).

		if( currentTime != mLastRecordedCurrentTimeValue ) {

			char *eventString = autoSprintf( "T %f", currentTime );

			mEventBatch.push_back( eventString );

			mLastRecordedCurrentTimeValue = currentTime;
		}
		else {
			// repeat, record short string to indicate this
			char *eventString = stringDuplicate( "R" );

			mEventBatch.push_back( eventString );
		}
	}


	return currentTime;
}


// to save FPS in recording files
void OneLife::game::Application::registerActualFrameRate( double inFrameRate )
{
	//this->screenGL->registerActualFrameRate(inFrameRate);
	if( mRecordingEvents &&
		mRecordingOrPlaybackStarted ) {

		char *eventString = autoSprintf( "F %lf", inFrameRate );

		mEventBatch.push_back( eventString );
	}
}

// these can be played back
double OneLife::game::Application::getRecordedFrameRate()
{
	//return this->screenGL->getRecordedFrameRate();
	return mLastActualFrameRate;
}

// sets mapping so that when inFromKey is pressed, an
// event for inToKey is generated instead
void OneLife::game::Application::setKeyMapping( unsigned char inFromKey,
					unsigned char inToKey )
{
	//this->screenGL->setKeyMapping(setKeyMapping, inToKey);
	keyMap[ inFromKey ] = inToKey;

	AppLog::getLog()->logPrintf(
			Log::INFO_LEVEL,
			"Mapping key '%c' to '%c'", inFromKey, inToKey );
}


// turns key mapping on (default) or off
void OneLife::game::Application::toggleKeyMapping( char inMappingOn )
{
	//this->screenGL->toggleKeyMapping(inMappingOn);
	// keyMapOn = inMappingOn;
}



// passes in a web event to be (possibly) added to the current
// game recording
// type encodes :
//    0 = normal request step completed (NULL body)
//    1 = request done step completed (NULL body)
//   -1 = error-returning request step completed (NULL body)
//    2 = result-fetch step completed (non-NULL body)
//   >2 = web progress event, showing number of bytes fetched
//        (NULL body)
void OneLife::game::Application::registerWebEvent( int inHandle,
					   int inType,
					   const char *inBodyString,
		// -1 means use string length
		// provide length for binary bodies
					   int inBodyLength)
{
	//this->screenGL->registerWebEvent(inHandle, inType, inBodyString, inBodyLength);
	if( ! currentScreenGL->mRecordingEvents ||
		! currentScreenGL->mRecordingOrPlaybackStarted ) {

		// not recording!
		return;
	}


	char *eventString;

	// only event type 2 has a body text payload
	if( inType == 2 ) {
		if( inBodyLength == -1 ) {

			eventString = autoSprintf( "wb %u %d %u %s", inHandle,
					inType,
					strlen( inBodyString ), inBodyString );
		}
		else {

			char *bodyHex = hexEncode( (unsigned char*)inBodyString,
					inBodyLength );

			eventString = autoSprintf( "wx %u %d %u %s", inHandle,
					inType,
					strlen( bodyHex ), bodyHex );

			delete [] bodyHex;
		}
	}
	else {
		eventString = autoSprintf( "wb %u %d", inHandle, inType );
	}


	mEventBatch.push_back( eventString );
}


// gets the type of the next pending web event (from playback)
// if the event has no result body, this call removes the event
// from the list.
// If it has a result body, getWebEventResultBody must be called.
int OneLife::game::Application::getWebEventType( int inHandle )
{
	//return this->screenGL->getWebEventType(inHandle);
	for( int i=0; i<mPendingWebEvents.size(); i++ ) {
		WebEvent *e = mPendingWebEvents.getElement( i );

		if( e->handle == inHandle ) {

			int type = e->type;

			if( type != 2 ) {
				mPendingWebEvents.deleteElement( i );
			}

			return type;
		}
	}

	// no event record present?
	// assume a normal, non-finished step occurred
	return 0;
}

// gets a recorded event body text from the current frame matching
//   inHandle
// result NULL if there is no event matching inHandle
// result destroyed by caller.
//
// This call removes the web event from the list of pending events.
char* OneLife::game::Application::getWebEventResultBody( int inHandle,
							 int *outSize)
{
	//return this->screenGL->getWebEventResultBody(inHandle, outSize);
	for( int i=0; i<mPendingWebEvents.size(); i++ ) {
		WebEvent *e = mPendingWebEvents.getElement( i );

		if( e->handle == inHandle ) {

			char *returnValue = e->bodyText;

			if( outSize != NULL ) {
				*outSize = e->bodyLength;
			}

			mPendingWebEvents.deleteElement( i );

			return returnValue;
		}
	}

	return NULL;
}

// passes in a socket event to be (possibly) added to the current
// game recording
// type encodes :
//    0 = send with number sent (NULL body)
//    1 = send resulting in error (NULL body)
//    2 = read with number read (non-NULL body if inNumBodyBytes != 0)
//    3 = read resulting in error (NULL body)
void OneLife::game::Application::registerSocketEvent( int inHandle,
						  int inType,
						  int inNumBodyBytes,
						  unsigned char *inBodyBytes)
{
	//this->screenGL->registerSocketEvent(inHandle, inType, inNumBodyBytes, inBodyBytes);
	if( ! currentScreenGL->mRecordingEvents ||
		! currentScreenGL->mRecordingOrPlaybackStarted ) {

		// not recording!
		return;
	}


	char *eventString;

	// only event type 2 has a body byte payload
	if( inType == 2 && inNumBodyBytes != 0 ) {

		char *bodyHex = hexEncode( inBodyBytes, inNumBodyBytes );

		eventString = autoSprintf( "xs %u %d %u %s", inHandle,
				inType,
				inNumBodyBytes, bodyHex );
		delete [] bodyHex;
	}
	else {
		eventString = autoSprintf( "xs %u %d %d", inHandle, inType,
				inNumBodyBytes );
	}


	mEventBatch.push_back( eventString );
}


// gets the type of the next pending socket event (from playback)
// if the event has no body bytes
// (type 0, 1, or 3, OR type 2 with 0-length body),
// this call removes the event from the list.
//
// In case of type 2 with outNumBodyBytes > 0,
// getWebEventResultBody must be called.
//
//
void OneLife::game::Application::getSocketEventTypeAndSize( int inHandle,
								int *outType,
								int *outNumBodyBytes )
{
	for( int i=0; i<mPendingSocketEvents.size(); i++ ) {
		SocketEvent *e = mPendingSocketEvents.getElement( i );

		if( e->handle == inHandle ) {

			*outType = e->type;
			*outNumBodyBytes = e->numBodyBytes;

			if( *outType != 2 || *outNumBodyBytes == 0 ) {
				mPendingSocketEvents.deleteElement( i );
			}

			return;
		}
	}

	// no event record present?
	*outType = -1;
	*outNumBodyBytes = 0;
	return;
}

// gets a recorded socket event body bytes from the current
// frame matching inHandle
// result NULL if there is no event matching inHandle
// result destroyed by caller.
//
// return value has length as set by last getSocketEventTypeAndSize
// call
//
// This call removes the socket event from the list of pending events.
unsigned char* OneLife::game::Application::getSocketEventBodyBytes( int inHandle )
{
	//return this->screenGL->getSocketEventBodyBytes(inHandle);
	for( int i=0; i<mPendingSocketEvents.size(); i++ ) {
		SocketEvent *e = mPendingSocketEvents.getElement( i );

		if( e->handle == inHandle ) {


			unsigned char *returnValue = NULL;

			if( e->bodyBytesHex != NULL ) {
				returnValue = hexDecode( e->bodyBytesHex );

				delete [] e->bodyBytesHex;
			}

			mPendingSocketEvents.deleteElement( i );

			return returnValue;
		}
	}

	return NULL;
}

void OneLife::game::Application::registerAsyncFileDone( int inHandle )
{
	//this->screenGL->registerAsyncFileDone(inHandle);
	if( ! currentScreenGL->mRecordingEvents ||
		! currentScreenGL->mRecordingOrPlaybackStarted ) {

		// not recording!
		return;
	}


	char *eventString = autoSprintf( "af %d", inHandle );

	mEventBatch.push_back( eventString );
}

char OneLife::game::Application::getAsyncFileDone( int inHandle )
{
	//return this->screenGL->getAsyncFileDone(inHandle);
	return ( mLastAsyncFileHandleDone >= inHandle );
}

void OneLife::game::Application::writeEventBatchToFile() {
	int num = mEventBatch.size() + mUserEventBatch.size();

	fprintf( mEventFile, "%d ", num );

	writeEventBatchToFile( &mEventBatch );

	fprintf( mEventFile, " " );

	writeEventBatchToFile( &mUserEventBatch );

	fprintf( mEventFile, "\n" );

	fflush( mEventFile );
}

void OneLife::game::Application::writeEventBatchToFile( SimpleVector<char*> *inBatch ) {
	int numInBatch = inBatch->size();

	if( mEventFile != NULL ) {
		if( numInBatch > 0 ) {

			char **allEvents = inBatch->getElementArray();
			char *eventString = join( allEvents, numInBatch, " " );


			int eventStringLength = strlen( eventString );

			int numWritten = fwrite( eventString, 1, eventStringLength,
					mEventFile );

			if( numWritten != eventStringLength ) {
				printf( "Failed to write %d-event batch of length %d "
						"to recording file\n", numInBatch, eventStringLength );
			}

			delete [] allEvents;
			delete [] eventString;
		}

	}

	for( int i=0; i<numInBatch; i++ ) {
		delete [] *( inBatch->getElement( i ) );
	}
	inBatch->deleteAll();
}

void OneLife::game::Application::playNextEventBatch() {
	// we get a minimized event every frame that we're minimized
	mLastMinimizedStatus = false;

	mLastTimeValueStack.deleteAll();
	mLastCurrentTimeValueStack.deleteAll();


	// read and playback next batch
	int batchSize = 0;
	int numRead = fscanf( mEventFile, "%d", &batchSize );

	if( numRead == 0 || numRead == EOF ) {
		printf( "Reached end of recorded event file during playback\n" );
		// stop playback
		mPlaybackEvents = false;
	}


	for( int i=0; i<batchSize; i++ ) {

		char code[3];
		code[0] = '\0';

		fscanf( mEventFile, "%2s", code );

		switch( code[0] ) {
			case 'm':
				switch( code[1] ) {
					case 'm': {
						int x, y;
						fscanf( mEventFile, "%d %d", &x, &y );

						callbackPassiveMotion( x, y );
					}
						break;
					case 'd': {
						int x, y;
						fscanf( mEventFile, "%d %d", &x, &y );

						callbackMotion( x, y );
					}
						break;
					case 'b': {
						int button, state, x, y;
						fscanf( mEventFile, "%d %d %d %d",
								&button, &state, &x, &y );

						if( state == 1 ) {
							state = SDL_PRESSED;
						}
						else {
							state = SDL_RELEASED;
						}

						callbackMouse( button, state, x, y );
					}
						break;
				}
				break;
			case 'k': {
				int c, x, y;
				fscanf( mEventFile, "%d %d %d", &c, &x, &y );

				switch( code[1] ) {
					case 'd':
						callbackKeyboard( c, x, y );
						break;
					case 'u':
						callbackKeyboardUp( c, x, y );
						break;
				}
			}
				break;
			case 's': {
				int c, x, y;
				fscanf( mEventFile, "%d %d %d", &c, &x, &y );

				switch( code[1] ) {
					case 'd':
						callbackSpecialKeyboard( c, x, y );
						break;
					case 'u':
						callbackSpecialKeyboardUp( c, x, y );
						break;
				}
			}
				break;
			case 't': {
				fscanf( mEventFile, "%lf", &mLastTimeValue );
				mLastTimeValueStack.push_back( mLastTimeValue );
				mTimeValuePlayedBack = true;
			}
				break;
			case 'r': {
				// repeat last time value
				mLastTimeValueStack.push_back( mLastTimeValue );
				mTimeValuePlayedBack = true;
			}
				break;
			case 'T': {
				double t;
				fscanf( mEventFile, "%lf", &t );
				mLastCurrentTimeValue = t;
				mLastCurrentTimeValueStack.push_back( mLastCurrentTimeValue );
				mTimeValuePlayedBack = true;
			}
				break;
			case 'R': {
				// repeat last time value
				mLastCurrentTimeValueStack.push_back( mLastCurrentTimeValue );
				mTimeValuePlayedBack = true;
			}
				break;
			case 'F': {
				double fps;
				fscanf( mEventFile, "%lf", &fps );
				mLastActualFrameRate = fps;
			}
				break;
			case 'v': {
				mLastMinimizedStatus = true;
			}
				break;
			case 'w': {
				// special case:  incoming web event
				// (simulating response from a web server during playback)

				WebEvent e;
				fscanf( mEventFile, "%d %d", &( e.handle ), &( e.type ) );

				if( e.handle > mLastReadWebEventHandle ) {
					mLastReadWebEventHandle = e.handle;
					e.handle = mNextUnusedWebEventHandle;
					mCurrentWebEventHandle = e.handle;

					mNextUnusedWebEventHandle++;
				}
				else {
					e.handle = mCurrentWebEventHandle;
				}

				e.bodyText = NULL;
				e.bodyLength = 0;

				if( e.type == 2 ) {
					// includes a body payload

					unsigned int length;

					fscanf( mEventFile, "%u", &length );

					// skip the space after length
					fgetc( mEventFile );


					if( code[1] == 'b' ) {
						// plain text body
						e.bodyLength = length;
						e.bodyText = new char[ length + 1 ];

						unsigned int numRead =
								fread( e.bodyText, 1, length, mEventFile );

						e.bodyText[ length ] = '\0';

						if( numRead != length ) {
							AppLog::error(
									"Failed to read web event body text from "
									"playback file" );
							delete [] e.bodyText;
							e.bodyText = NULL;
							e.bodyLength = 0;
						}
					}
					else if( code[1] == 'x' ) {
						// hex-encoded body
						char *bodyHex = new char[ length + 1 ];

						unsigned int numRead =
								fread( bodyHex, 1, length, mEventFile );

						bodyHex[ length ] = '\0';

						if( numRead != length ) {
							AppLog::error(
									"Failed to read web event body hex from "
									"playback file" );
							e.bodyText = NULL;
							e.bodyLength = 0;
						}
						else {
							unsigned char *bodyDecoded
									= hexDecode( bodyHex );

							e.bodyLength = length / 2;
							e.bodyText = new char[ e.bodyLength + 1 ];

							memcpy( e.bodyText, bodyDecoded, e.bodyLength );

							e.bodyText[ e.bodyLength ] = '\0';

							delete [] bodyDecoded;
						}
						delete [] bodyHex;
					}
				}

				mPendingWebEvents.push_back( e );
				break;
			}
			case 'x': {
				// special case:  incoming socket event
				// (simulating response from a socket server during playback)

				SocketEvent e;
				fscanf( mEventFile, "%d %d %d",
						&( e.handle ), &( e.type ), &( e.numBodyBytes ) );

				e.bodyBytesHex = NULL;

				if( e.type == 2 && e.numBodyBytes != 0 ) {
					// includes a body payload

					// skip the space after numBodyBytes
					fgetc( mEventFile );

					unsigned int hexLength = e.numBodyBytes * 2;


					e.bodyBytesHex = new char[ hexLength + 1 ];

					unsigned int numRead =
							fread( e.bodyBytesHex, 1, hexLength, mEventFile );

					e.bodyBytesHex[ hexLength ] = '\0';

					if( numRead != hexLength ) {
						AppLog::error(
								"Failed to read socket event body hex from "
								"playback file" );
						delete [] e.bodyBytesHex;
						e.bodyBytesHex = NULL;
					}
				}

				mPendingSocketEvents.push_back( e );
				break;
			}
			case 'a': {
				int nextHandle;
				fscanf( mEventFile, "%d", &nextHandle );

				if( nextHandle > mLastAsyncFileHandleDone ) {
					// track the largest handle seen done so far
					// (async files are ready in handle order)
					mLastAsyncFileHandleDone = nextHandle;
				}
			}
				break;
			default:
				AppLog::getLog()->logPrintf(
						Log::ERROR_LEVEL,
						"Unknown code '%s' in playback file\n",
						code );
		}

	}


	mNumBatchesPlayed++;
}


/**
 * Switches to 2D mode, where no view transforms are applied
 *
 * Must be called before start();
 */
void OneLife::game::Application::switchTo2DMode()
{
	//this->screenGL->switchTo2DMode();
	m2DMode = true;
}



/**
 * Moves the view position.
 *
 * @param inPositionChange directional vector describing motion.
 *   Must be destroyed by caller.
 */
void OneLife::game::Application::moveView( Vector3D *inPositionChange )
{
	mViewPosition->add( inPositionChange );
}


/**
 * Rotates the view.
 *
 * @param inOrientationChange angle to rotate view by.
 *   Must be destroyed by caller.
 */
void OneLife::game::Application::rotateView( Angle3D *inOrientationChange )
{
	mViewOrientation->add( inOrientationChange );
}


/**
 * Gets the angle of the current view direction.
 *
 * @return the angle of the current view direction.
 *   Not a copy, so shouldn't be modified or destroyed by caller.
 */
Angle3D *OneLife::game::Application::getViewOrientation()
{
	return mViewOrientation;
}


/**
 * Gets the current view position.
 *
 * @return the position of the current view.
 *  Must be destroyed by caller.
 */
Vector3D *OneLife::game::Application::getViewPosition()
{
	return new Vector3D( mViewPosition );
}


/**
 * Sets the current view position.
 *
 * @param inPosition the new position.
 *   Must be destroyed by caller.
 */
void OneLife::game::Application::setViewPosition( Vector3D *inPosition )
{
	delete mViewPosition;
	mViewPosition = new Vector3D( inPosition );
}


/**
 * Gets the width of the screen.
 *
 * @return the width of the screen, in pixels.
 */
int OneLife::game::Application::getWidth()
{
	return mWide;
}



/**
 * Gets the height of the screen.
 *
 * @return the height of the screen, in pixels.
 */
int OneLife::game::Application::getHeight()
{
	return mHigh;
}



/**
 * Switches into full screen mode.
 *
 * Use changeWindowSize to switch back out of full screen mode.
 */
void OneLife::game::Application::setFullScreen()
{
	//this->screenGL->setFullScreen();
	//glutFullScreen();
}



/**
 * Sets the size of the viewport image in the window.
 *
 * Defaults to window size.
 *
 * Must be called before screen is started.
 *
 * @param inWidth, inHeight the new dimensions, in pixels.
 */
void OneLife::game::Application::setImageSize( int inWidth, int inHeight )
{
	mImageSizeSet = true;

	mImageWide = inWidth;
	mImageHigh = inHeight;
}



/**
 * Gets the width of the viewport image.
 *
 * @return the width of the viewport, in pixels.
 */
int OneLife::game::Application::getImageWidth()
{
	return mImageWide;
}



/**
 * Gets the height of the viewport image.
 *
 * @return the height of the viewport, in pixels.
 */
int OneLife::game::Application::getImageHeight()
{
	return mImageHigh;
}



/**
 * Change the window size.
 *
 * @param inWidth, inHeight the new dimensions, in pixels.
 */
void OneLife::game::Application::changeWindowSize( int inWidth, int inHeight )
{
	//this->screenGL->changeWindowSize(inWidth, inHeight);
	//glutReshapeWindow( inWidth, inHeight );
}



/**
 * Adds a mouse handler.
 *
 * @param inHandler the handler to add  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::addMouseHandler( MouseHandlerGL *inListener )
{
	mMouseHandlerVector->push_back( inListener );
}



/**
 * Removes a mouse handler.
 *
 * @param inHandler the handler to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::removeMouseHandler( MouseHandlerGL *inListener )
{
	mMouseHandlerVector->deleteElementEqualTo( inListener );
}


/**
 * Adds a keyboard handler.
 *
 * @param inHandler the handler to add  Must
 *   be destroyed by caller.
 * @param inFirstHandler true to put this handler ahead of
 *   existing handlers in the list.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::addKeyboardHandler( KeyboardHandlerGL *inListener,
													 char inFirstHandler ) {
	if( !inFirstHandler ) {
		mKeyboardHandlerVector->push_back( inListener );
	}
	else {
		int numExisting= mKeyboardHandlerVector->size();
		KeyboardHandlerGL **oldHandlers =
				mKeyboardHandlerVector->getElementArray();

		mKeyboardHandlerVector->deleteAll();

		mKeyboardHandlerVector->push_back( inListener );
		mKeyboardHandlerVector->appendArray( oldHandlers, numExisting );
		delete [] oldHandlers;
	}
}



/**
 * Removes a keyboard handler.
 *
 * @param inHandler the handler to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::removeKeyboardHandler( KeyboardHandlerGL *inListener )
{
	mKeyboardHandlerVector->deleteElementEqualTo( inListener );
}

/**
 * Removes a scene handler.
 *
 * @param inHandler the handler to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
/*
void OneLife::game::Application::removeSceneHandler( SceneHandlerGL *inListener )
{
	//mSceneHandlerVector->deleteElementEqualTo( inListener );
}
*/


/**
 * Adds a redraw listener.
 *
 * @param inListener the listener to add.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::addRedrawListener( RedrawListenerGL *inListener )
{
	mRedrawListenerVector->push_back( inListener );
}


/**
 * Removes a redraw listener.
 *
 * @param inListener the listener to remove.  Must
 *   be destroyed by caller.
 *
 * Must not be called after calling start().
 */
void OneLife::game::Application::removeRedrawListener( RedrawListenerGL *inListener )
{
	mRedrawListenerVector->deleteElementEqualTo( inListener );
}



/**
 * Applies the current view matrix transformation
 * to the matrix at the top of the GL_PROJECTION stack.
 */
void OneLife::game::Application::applyViewTransform()
{
	//return this->screenGL->applyViewTransform();
	// compute view angle

	// default angle is 90, but we want to force a 1:1 aspect ratio to avoid
	// distortion.
	// If our screen's width is different than its height, we need to decrease
	// the view angle so that the angle coresponds to the smaller dimension.
	// This keeps the zoom factor constant in the smaller dimension.

	// Of course, because of the way perspective works, only one Z-slice
	// will have a constant zoom factor
	// The zSlice variable sets the distance of this slice from the picture
	// plane
	double zSlice = .31;

	double maxDimension = mWide;
	if( mHigh > mWide ) {
		maxDimension = mHigh;
	}
	double aspectDifference = fabs( mWide / 2 - mHigh / 2 ) / maxDimension;
	// default angle is 90 degrees... half the angle is PI/4
	double angle = atan( tan( M_PI / 4 ) +
						 aspectDifference / zSlice );

	// double it to get the full angle
	angle *= 2;


	// convert to degrees
	angle = 360 * angle / ( 2 * M_PI );


	// set up the projection matrix
	glMatrixMode( GL_PROJECTION );

	glLoadIdentity();

	//gluPerspective( 90, mWide / mHigh, 1, 9999 );
	gluPerspective( angle,
			1,
			1, 9999 );


	// set up the model view matrix
	glMatrixMode( GL_MODELVIEW );

	glLoadIdentity();

	// create default view and up vectors,
	// then rotate them by orientation angle
	Vector3D *viewDirection = new Vector3D( 0, 0, 1 );
	Vector3D *upDirection = new Vector3D( 0, 1, 0 );

	viewDirection->rotate( mViewOrientation );
	upDirection->rotate( mViewOrientation );

	// get a point out in front of us in the view direction
	viewDirection->add( mViewPosition );

	// look at takes a viewer position,
	// a point to look at, and an up direction
	gluLookAt( mViewPosition->mX,
			mViewPosition->mY,
			mViewPosition->mZ,
			viewDirection->mX,
			viewDirection->mY,
			viewDirection->mZ,
			upDirection->mX,
			upDirection->mY,
			upDirection->mZ );

	delete viewDirection;
	delete upDirection;
}



void OneLife::game::Application::obscureRecordedNumericTyping( char inObscure,
								   char inCharToRecordInstead ) {
	mObscureRecordedNumericTyping = inObscure;
	mCharToRecordInstead = inCharToRecordInstead;
}


/**
 * Access the various handlers.
 */
//KeyboardHandlerGL *getKeyHandler();
//MouseHandlerGL *getMouseHandler();
//SceneHandlerGL *getSceneHandler();


char OneLife::game::Application::isLastMouseButtonRight() {
	return mLastMouseButtonRight;
}

// FOVMOD NOTE:  Change 2/3 - Take these lines during the merge process
int OneLife::game::Application::getLastMouseButton() {
	return mLastMouseButton;
}

char OneLife::game::Application::isKeyboardHandlerFocused() {
	for( int h=0; h<mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( mKeyboardHandlerVector->getElement( h ) );
		if( handler->isFocused() ) {
			return true;
		}
	}

	// else none were focused
	return false;
}

/**********************************************************************************************************************/

//!private

/**
 *
 */
void OneLife::game::Application::setupSurface() {
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	int flags = 0;

#ifndef RASPBIAN
	// don't have SDL create the GL surface
	// (the SDL one is a software surface on Raspbian)
	flags = SDL_OPENGL;
#endif

	// NOTE:  flags are also adjusted below if fullscreen resolution not
	// available
	int borderless = 0;

	if( mFullScreen ) {
#ifdef __mac__
		borderless = 1;
        NSMenu_setMenuBarVisible(0);
#else
		borderless = SettingsManager::getIntSetting( "borderless", 0 );
#endif

		if( borderless ) {
			AppLog::info( "Setting borderless mode for fullscreen" );
			SDL_putenv( (char*)"SDL_VIDEO_WINDOW_POS=0,0" );

			flags = flags | SDL_NOFRAME;
		}
		else {
			AppLog::info( "Setting real (not borderless) fullscreen mode" );
			flags = flags | SDL_FULLSCREEN;
		}
	}

	const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

	int currentW = currentScreenInfo->current_w;
	int currentH = currentScreenInfo->current_h;

	// aspect ratio
	int currentAspectRatio = computeAspectRatio( currentW, currentH );

	AppLog::getLog()->logPrintf(
			Log::INFO_LEVEL,
			"Current screen configuration is %dx%d with aspect ratio %.2f",
			currentW, currentH, currentAspectRatio / 100.0f );



	// check for available modes
	SDL_Rect** modes;


	// Get available fullscreen/hardware modes
	modes = SDL_ListModes( NULL, flags);

	// Check if there are any modes available
	if( modes == NULL ) {
		AppLog::criticalError( "ERROR:  No video modes available");
		exit(-1);
	}

	// Check if our resolution is restricted
	if( modes == (SDL_Rect**)-1 ) {
		AppLog::info( "All resolutions available" );

		if( mFullScreen && mDoNotChangeNativeResolution ) {
			AppLog::info( "Sticking with user's current screen resolution" );

			int borderlessHeightAdjust =
					SettingsManager::getIntSetting( "borderlessHeightAdjust", 0 );

			mWide = currentW;
			mHigh = currentH;

			if( borderless && borderlessHeightAdjust != 0 ) {
				AppLog::getLog()->logPrintf(
						Log::INFO_LEVEL,
						"Adding %d to borderless window height of %d "
						"resulting in total height of %d",
						borderlessHeightAdjust, mHigh,
						mHigh + borderlessHeightAdjust );
				mHigh += borderlessHeightAdjust;
			}
		}
	}
	else if( mForceSpecifiedDimensions && mFullScreen ) {

		AppLog::info( "Requested video mode is forced (playback?)" );

		// check if specified dimension available in fullscreen

		char match = false;

		for( int i=0; modes[i] && ! match; ++i ) {
			if( mWide == modes[i]->w &&
				mHigh == modes[i]->h ) {
				match = true;
			}
		}

		if( !match ) {
			AppLog::getLog()->logPrintf(
					Log::WARNING_LEVEL,
					"  Could not find a full-screen match for the forced screen "
					"dimensions %d x %d\n", mWide, mHigh );
			AppLog::warning( "Reverting to windowed mode" );

			mFullScreen = false;

			flags = SDL_OPENGL;
#ifdef RASPBIAN
			flags = 0;
#endif
		}
	}
	else{

		if( mFullScreen && mDoNotChangeNativeResolution ) {
			AppLog::info( "Sticking with user's current screen resolution" );

			mWide = currentW;
			mHigh = currentH;
		}


		AppLog::getLog()->logPrintf(
				Log::INFO_LEVEL,
				"Checking if requested video mode (%dx%d) is available",
				mWide, mHigh );


		// Print valid modes

		// only count a match of BOTH resolution and aspect ratio
		char match = false;

		AppLog::info( "Available video modes:" );
		for( int i=0; modes[i]; ++i ) {
			AppLog::getLog()->logPrintf( Log::DETAIL_LEVEL,
					"   %d x %d\n",
					modes[i]->w,
					modes[i]->h );

			int thisAspectRatio = computeAspectRatio( modes[i]->w,
					modes[i]->h );

			if( !mForceAspectRatio && thisAspectRatio == currentAspectRatio ) {
				AppLog::info( "   ^^^^ this mode matches current "
							  "aspect ratio" );
			}

			if( mWide == modes[i]->w && mHigh == modes[i]->h ) {
				AppLog::info( "   ^^^^ this mode matches requested mode" );

				if( ! mForceAspectRatio &&
					thisAspectRatio != currentAspectRatio ) {
					AppLog::info( "        but it doesn't match current "
								  "aspect ratio" );
				}
				else {
					match = true;
				}
			}
		}

		if( !match ) {
			AppLog::warning( "Warning:  No match for requested video mode" );
			AppLog::info( "Trying to find the closest match" );

			int bestDistance = 99999999;

			int bestIndex = -1;

			for( int i=0; modes[i]; ++i ) {
				// don't even consider modes that are SMALLER than our
				// requested mode in either dimension
				if( modes[i]->w >= mWide &&
					modes[i]->h >= mHigh ) {

					int distance = (int)(
							fabs( modes[i]->w - mWide ) +
							fabs( modes[i]->h - mHigh ) );

					int thisAspectRatio = computeAspectRatio( modes[i]->w,
							modes[i]->h );

					if( ( mForceAspectRatio ||
						  thisAspectRatio == currentAspectRatio )
						&&
						distance < bestDistance ) {

						bestIndex = i;
						bestDistance = distance;
					}
				}

			}


			if( bestIndex != -1 ) {

				if( mForceAspectRatio ) {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Picking closest available large-enough resolution:  "
							"%d x %d\n",
							modes[bestIndex]->w,
							modes[bestIndex]->h );
				}
				else {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Picking closest available large-enough resolution "
							"that matches current aspect ratio:  %d x %d\n",
							modes[bestIndex]->w,
							modes[bestIndex]->h );
				}
			}
			else {
				// search again, ignoring aspect match

				if( !mForceAspectRatio ) {

					AppLog::warning(
							"Warning:  No match for current aspect ratio" );
					AppLog::info(
							"Trying to find the closest off-ratio match" );


					for( int i=0; modes[i]; ++i ) {
						// don't even consider modes that are SMALLER than our
						// requested mode in either dimension
						if( modes[i]->w >= mWide &&
							modes[i]->h >= mHigh ) {

							int distance = (int)(
									fabs( modes[i]->w - mWide ) +
									fabs( modes[i]->h - mHigh ) );

							if( distance < bestDistance ) {
								bestIndex = i;
								bestDistance = distance;
							}
						}
					}
				}


				if( bestIndex != -1 ) {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Picking closest available large-enough resolution:  "
							"%d x %d\n",
							modes[bestIndex]->w,
							modes[bestIndex]->h );
				}
				else {
					AppLog::warning(
							"Warning:  No sufficiently sized resolution found" );
					AppLog::info(
							"Considering closest-match smaller resolution" );

					for( int i=0; modes[i]; ++i ) {
						int distance = (int)(
								fabs( modes[i]->w - mWide ) +
								fabs( modes[i]->h - mHigh ) );

						if( distance < bestDistance ) {
							bestIndex = i;
							bestDistance = distance;
						}
					}

					if( bestIndex != -1 ) {
						AppLog::getLog()->logPrintf(
								Log::INFO_LEVEL,
								"Picking closest available resolution:  "
								"%d x %d\n",
								modes[bestIndex]->w,
								modes[bestIndex]->h );
					}
					else {
						AppLog::criticalError(
								"ERROR:  No video modes available");
						exit(-1);
					}
				}

			}


			mWide = modes[bestIndex]->w;
			mHigh = modes[bestIndex]->h;
		}

	}


	// 1-bit stencil buffer
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );

	// vsync to avoid tearing
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 );

	// current color depth
	SDL_Surface *screen = SDL_SetVideoMode( mWide, mHigh, 0, flags);

#ifdef RASPBIAN
	raspbianCreateSurface();
#endif


	if ( screen == NULL ) {
		printf( "Couldn't set %dx%d GL video mode: %s\n",
				mWide,
				mHigh,
				SDL_GetError() );
	}

#ifdef RASPBIAN
	screenGLStencilBufferSupported = true;
#else
	int setStencilSize;
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &setStencilSize );
	if( setStencilSize > 0 ) {
		// we have a stencil buffer
		screenGLStencilBufferSupported = true;
	}
#endif


	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glCullFace( GL_BACK );
	glFrontFace( GL_CCW );
}

/**********************************************************************************************************************/

//!callback create for glut now use in SDL context
void callbackResize( int inW, int inH ) {
	OneLife::game::Application *s = currentScreenGL;
	s->mWide = inW;
	s->mHigh = inH;


	int bigDimension = s->mImageWide;
	if( bigDimension < s->mImageHigh ) {
		bigDimension = s->mImageHigh;
	}

	int excessW = s->mWide - bigDimension;
	int excessH = s->mHigh - bigDimension;

	// viewport is square of biggest image dimension, centered on screen
	glViewport( excessW / 2,
			excessH / 2,
			bigDimension,
			bigDimension );
}

void callbackKeyboard( unsigned char inKey, int inX, int inY ) {
	// all playback events are already mapped
	if( ! ( currentScreenGL->mPlaybackEvents &&
			currentScreenGL->mRecordingOrPlaybackStarted )
		&& keyMapOn ) {

		inKey = keyMap[inKey];
	}

	if( currentScreenGL->mRecordingEvents &&
		currentScreenGL->mRecordingOrPlaybackStarted ) {

		unsigned char keyToRecord = inKey;

		if( currentScreenGL->mObscureRecordedNumericTyping &&
			inKey >= '0' && inKey <= '9' ) {
			keyToRecord = currentScreenGL->mCharToRecordInstead;
		}

		char *eventString = autoSprintf( "kd %d %d %d",
				keyToRecord, inX, inY );

		currentScreenGL->mUserEventBatch.push_back( eventString );
	}


	char someFocused = currentScreenGL->isKeyboardHandlerFocused();

	int h;
	// flag those that exist right now
	// because handlers might remove themselves or add new handlers,
	// and we don't want to fire to those that weren't present when
	// callback was called

	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler = *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = true;
	}

	// fire to all handlers, stop if eaten
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ )
	{
		KeyboardHandlerGL *handler = *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );

		if( handler->mHandlerFlagged ) {
			// if some are focused, only fire to this handler if it is one
			// of the focused handlers
			if( !someFocused || handler->isFocused() ) {
				handler->keyPressed( inKey, inX, inY );
				if( handler->mEatEvent ) {
					handler->mEatEvent = false;
					goto down_eaten;
				}
			}
		}
	}

	down_eaten:

	// deflag for next time
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ )
	{
		KeyboardHandlerGL *handler = *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = false;
	}
}

void callbackKeyboardUp( unsigned char inKey, int inX, int inY ) {
	// all playback events are already mapped
	if( ! ( currentScreenGL->mPlaybackEvents &&
			currentScreenGL->mRecordingOrPlaybackStarted )
		&& keyMapOn ) {

		inKey = keyMap[inKey];
	}

	if( currentScreenGL->mRecordingEvents &&
		currentScreenGL->mRecordingOrPlaybackStarted ) {

		unsigned char keyToRecord = inKey;

		if( currentScreenGL->mObscureRecordedNumericTyping  &&
			inKey >= '0' && inKey <= '9' ) {
			keyToRecord = currentScreenGL->mCharToRecordInstead;
		}

		char *eventString = autoSprintf( "ku %d %d %d",
				keyToRecord, inX, inY );

		currentScreenGL->mUserEventBatch.push_back( eventString );
	}

	char someFocused = currentScreenGL->isKeyboardHandlerFocused();

	int h;
	// flag those that exist right now
	// because handlers might remove themselves or add new handlers,
	// and we don't want to fire to those that weren't present when
	// callback was called

	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = true;
	}

	// fire to all handlers, stop if eaten
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );

		if( handler->mHandlerFlagged ) {

			// if some are focused, only fire to this handler if it is one
			// of the focused handlers
			if( !someFocused || handler->isFocused() ) {
				handler->keyReleased( inKey, inX, inY );
				if( handler->mEatEvent ) {
					handler->mEatEvent = false;
					goto up_eaten;
				}
			}
		}
	}

	up_eaten:


	// deflag for next time
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = false;
	}

}

void callbackSpecialKeyboard( int inKey, int inX, int inY ) {
	if( currentScreenGL->mRecordingEvents &&
		currentScreenGL->mRecordingOrPlaybackStarted ) {

		char *eventString = autoSprintf( "sd %d %d %d", inKey, inX, inY );

		currentScreenGL->mUserEventBatch.push_back( eventString );
	}


	char someFocused = currentScreenGL->isKeyboardHandlerFocused();

	int h;
	// flag those that exist right now
	// because handlers might remove themselves or add new handlers,
	// and we don't want to fire to those that weren't present when
	// callback was called

	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = true;
	}


	// fire to all handlers, stop if eaten
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );

		if( handler->mHandlerFlagged ) {

			// if some are focused, only fire to this handler if it is one
			// of the focused handlers
			if( !someFocused || handler->isFocused() ) {
				handler->specialKeyPressed( inKey, inX, inY );
				if( handler->mEatEvent ) {
					handler->mEatEvent = false;
					goto special_down_eaten;
				}
			}
		}
	}

	special_down_eaten:

	// deflag for next time
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = false;
	}
}

void callbackSpecialKeyboardUp( int inKey, int inX, int inY ) {
	if( currentScreenGL->mRecordingEvents &&
		currentScreenGL->mRecordingOrPlaybackStarted ) {

		char *eventString = autoSprintf( "su %d %d %d", inKey, inX, inY );

		currentScreenGL->mUserEventBatch.push_back( eventString );
	}


	char someFocused = currentScreenGL->isKeyboardHandlerFocused();

	int h;
	// flag those that exist right now
	// because handlers might remove themselves or add new handlers,
	// and we don't want to fire to those that weren't present when
	// callback was called

	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = true;
	}

	// fire to all handlers, stop if eaten
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );

		if( handler->mHandlerFlagged ) {

			// if some are focused, only fire to this handler if it is one
			// of the focused handlers
			if( !someFocused || handler->isFocused() ) {
				handler->specialKeyReleased( inKey, inX, inY );
				if( handler->mEatEvent ) {
					handler->mEatEvent = false;
					goto special_up_eaten;
				}
			}
		}
	}

	special_up_eaten:

	// deflag for next time
	for( h=0; h<currentScreenGL->mKeyboardHandlerVector->size(); h++ ) {
		KeyboardHandlerGL *handler
				= *( currentScreenGL->mKeyboardHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = false;
	}

}

void callbackMotion( int inX, int inY ) {
	if( currentScreenGL->mRecordingEvents &&
		currentScreenGL->mRecordingOrPlaybackStarted ) {

		char *eventString = autoSprintf( "md %d %d", inX, inY );

		currentScreenGL->mUserEventBatch.push_back( eventString );
	}

	// fire to all handlers
	int h;
	// flag those that exist right now
	// because handlers might remove themselves or add new handlers,
	// and we don't want to fire to those that weren't present when
	// callback was called

	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = true;
	}

	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );
		if( handler->mHandlerFlagged ) {
			handler->mouseDragged( inX, inY );
		}
	}

	// deflag for next time
	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = false;
	}
}

void callbackPassiveMotion( int inX, int inY ) {

	if( currentScreenGL->mRecordingEvents &&
		currentScreenGL->mRecordingOrPlaybackStarted ) {

		char *eventString = autoSprintf( "mm %d %d", inX, inY );

		currentScreenGL->mUserEventBatch.push_back( eventString );
	}

	// fire to all handlers
	int h;
	// flag those that exist right now
	// because handlers might remove themselves or add new handlers,
	// and we don't want to fire to those that weren't present when
	// callback was called

	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = true;
	}

	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );

		if( handler->mHandlerFlagged ) {
			handler->mouseMoved( inX, inY );
		}
	}

	// deflag for next time
	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = false;
	}
}

void callbackMouse( int inButton, int inState, int inX, int inY ) {

	// FOVMOD NOTE:  Change 3/3 - Take these lines during the merge process
	currentScreenGL->mLastMouseButtonRight = false;

	switch( inButton ) {
		case SDL_BUTTON_LEFT:
			currentScreenGL->mLastMouseButton = MouseButton::LEFT;
			break;
		case SDL_BUTTON_MIDDLE:
			currentScreenGL->mLastMouseButton = MouseButton::MIDDLE;
			break;
		case SDL_BUTTON_RIGHT:
			currentScreenGL->mLastMouseButtonRight = true;
			currentScreenGL->mLastMouseButton = MouseButton::RIGHT;
			break;
		case SDL_BUTTON_WHEELUP:
			currentScreenGL->mLastMouseButton = MouseButton::WHEELUP;
			break;
		case SDL_BUTTON_WHEELDOWN:
			currentScreenGL->mLastMouseButton = MouseButton::WHEELDOWN;
			break;
		default:
			currentScreenGL->mLastMouseButton = MouseButton::NONE;
			break;
	}


	if( currentScreenGL->mRecordingEvents &&
		currentScreenGL->mRecordingOrPlaybackStarted ) {

		int stateEncoding = 0;
		if( inState == SDL_PRESSED ) {
			stateEncoding = 1;
		}

		char *eventString = autoSprintf( "mb %d %d %d %d",
				inButton, stateEncoding, inX, inY );

		currentScreenGL->mUserEventBatch.push_back( eventString );
	}


	// fire to all handlers
	int h;

	// flag those that exist right now
	// because handlers might remove themselves or add new handlers,
	// and we don't want to fire to those that weren't present when
	// callbackMouse was called

	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = true;
	}

	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );

		if( handler->mHandlerFlagged ) {

			handler->mouseMoved( inX, inY );
			if( inState == SDL_PRESSED ) {
				handler->mousePressed( inX, inY );
			}
			else if( inState == SDL_RELEASED ) {
				handler->mouseReleased( inX, inY );
			}
			else {
				printf( "Error:  Unknown mouse state received from SDL\n" );
			}
		}
	}

	// deflag for next time
	for( h=0; h<currentScreenGL->mMouseHandlerVector->size(); h++ ) {
		MouseHandlerGL *handler
				= *( currentScreenGL->mMouseHandlerVector->getElement( h ) );
		handler->mHandlerFlagged = false;
	}

}

void callbackPreDisplay() {
	OneLife::game::Application *s = currentScreenGL;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


	// fire to all redraw listeners
	// do this first so that they can update our view transform
	// this makes control much more responsive
	for( int r=0; r<s->mRedrawListenerVector->size(); r++ ) {
		RedrawListenerGL *listener
				= *( s->mRedrawListenerVector->getElement( r ) );
		listener->fireRedraw();
	}
}

void callbackIdle() {
	//glutPostRedisplay();
}

//!screen
// aspect ratio rounded to nearest 1/100
// (16:9  is 177)
// Some screen resolutions, like 854x480, are not exact matches to their
// aspect ratio
int computeAspectRatio( int inW, int inH ) {

	int intRatio = (100 * inW ) / inH;

	return intRatio;
}