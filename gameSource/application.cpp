//
// Created by olivier on 29/10/2021.
//

#include "application.h"

#include <SDL/SDL.h>
#include <cmath>
#include <climits>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include "minorGems/util/SimpleVector.h"
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
#include "OneLife/gameSource/misc_plateformspecific.h"
#include "OneLife/gameSource/dataTypes/ui.h"
#include "OneLife/gameSource/components/keyboard.h"
#include "OneLife/gameSource/components/engines/GameSceneHandler.h" //TODO: rename to gameScreenDeviceListener
#include "OneLife/gameSource/components/engines/screenRenderer.h"
#include "OneLife/gameSource/components/GamePage.h"
#include "OneLife/gameSource/components/messageChannel.h"
#include "OneLife/gameSource/dataTypes/signals.h"
#include "OneLife/gameSource/dataTypes/exception/exception.h"
#include "OneLife/gameSource/procedures/graphics/drawFrame.h"
#include "OneLife/gameSource/components/trace.h"
#include "OneLife/gameSource/dataTypes/feature.h"
#include "OneLife/gameSource/debug.h"

using signal = OneLife::dataType::Signal;

#ifdef __mac__
#include "minorGems/game/platforms/SDL/mac/SDLMain_Ext.h"
#endif

#ifdef RASPBIAN
#include "RaspbianGLSurface.cpp"
#endif

using namespace OneLife::dataType::hardware::keyboard;

extern unsigned char keyMap[256];
extern char keyMapOn;
extern GameSceneHandler *sceneHandler;
#include "OneLife/gameSource/controllers/LivingLifePage.h"
extern GamePage *currentGamePage;
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
//char screenGLStencilBufferSupported = false; //TODO: check if not used somewhere =>delete
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
char firstDrawFrameCalled = false;
char upKey = 'w';
char leftKey = 'a';
char downKey = 's';
char rightKey = 'd';

/**********************************************************************************************************************/
#include "minorGems/game/doublePair.h"
#include "OneLife/gameSource/procedures/graphics/modalObjects/drawPausePanel.h"
#include "OneLife/gameSource/controllers/AutoUpdatePage.h"
#include "OneLife/gameSource/controllers/ExistingAccountPage.h"
#include "OneLife/gameSource/controllers/ExtendedMessagePage.h"
#include "OneLife/gameSource/controllers/FinalMessagePage.h"
#include "OneLife/gameSource/controllers/GeneticHistoryPage.h"
#include "OneLife/gameSource/controllers/LivingLifePage.h"
#include "OneLife/gameSource/controllers/LoadingPage.h"
#include "OneLife/gameSource/controllers/PollPage.h"
#include "OneLife/gameSource/controllers/RebirthChoicePage.h"
#include "OneLife/gameSource/controllers/ReviewPage.h"
#include "OneLife/gameSource/controllers/ServerActionPage.h"
#include "OneLife/gameSource/controllers/SettingsPage.h"
#include "OneLife/gameSource/controllers/TwinPage.h"
#include "OneLife/gameSource/components/socket.h"
#include "OneLife/gameSource/musicPlayer.h"
#include "OneLife/gameSource/components/banks/spriteBank.h"
#include "OneLife/gameSource/soundBank.h"
#include "OneLife/gameSource/photos.h"
#include "OneLife/gameSource/categoryBank.h"
#include "OneLife/gameSource/liveObjectSet.h"
#include "OneLife/gameSource/lifeTokens.h"
#include "OneLife/gameSource/fitnessScore.h"
#include "OneLife/gameSource/groundSprites.h"

extern FinalMessagePage *finalMessagePage;
extern ServerActionPage *getServerAddressPage;
extern LoadingPage *loadingPage;
extern AutoUpdatePage *autoUpdatePage;
extern ExistingAccountPage *existingAccountPage;
extern ExtendedMessagePage *extendedMessagePage;
extern RebirthChoicePage *rebirthChoicePage;
extern SettingsPage *settingsPage;
extern ReviewPage *reviewPage;
extern TwinPage *twinPage;
extern PollPage *pollPage;
extern GeneticHistoryPage *geneticHistoryPage;
extern float pauseScreenFade;
extern char userReconnect;
extern doublePair lastScreenViewCenter;
extern int versionNumber;
extern int serverPort;
extern char *serverIP;
extern char *userTwinCode;
extern char autoLogIn;
extern char mapPullMode;
extern char *accountKey;
extern char *userEmail;
extern float musicLoudness;
extern int holdDeleteKeySteps;
extern int stepsBetweenDeleteRepeat;
extern double frameRateFactor;
extern char *currentUserTypedMessage;
extern int loadingStepBatchSize;
extern int numLoadingSteps;
extern char loginEditOverride;

int loadingPhase = 0;
double loadingPhaseStartTime;
/**********************************************************************************************************************/

OneLife::game::Application::Application(
		OneLife::game::Settings currentGame,
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
	this->idScreen = 0;
	this->isNewSystemEnable = false;

	//!gameScreens declaration
	this->controller.initializationScreen = nullptr;

	//!init SDL context ...
	Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
	if(currentGame.useSound) flags |= SDL_INIT_AUDIO;
	int sdlResult = SDL_Init( flags );
	if(sdlResult < 0);//TODO throw exception

	//!init current screen ...
	this->currentScreen.status.fullScreen = inFullScreen;
	this->currentScreen.settings.grabInput = false;//take value from mWasFullScreenBeforeMinimize = false; from initFrameDrawer(...)

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

	int hidePlaybackDisplayFlag = SettingsManager::getIntSetting( "hidePlaybackDisplay", 0 );

	if( hidePlaybackDisplayFlag == 1 )
	{
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

	if( !playbackDir.exists() )
	{
		playbackDir.makeDirectory();
	}

	int numChildren;
	File **childFiles = playbackDir.getChildFiles( &numChildren );

	if( numChildren > 0 )
	{
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

					if( difference == 0 )
					{

						mRecordingEvents = false;
						mPlaybackEvents = true;

						mRandSeed = readRandSeed;
						mMaxFrameRate = readMaxFrameRate;
						mWide = readWide;
						mHigh = readHigh,

								mFullFrameRate = mMaxFrameRate;

						mImageWide = mWide;
						mImageHigh = mHigh;

						AppLog::info("Forcing dimensions specified in playback file");
						mForceSpecifiedDimensions = true;


						if( fullScreenFlag ) {
							this->currentScreen.status.fullScreen = true;
						}
						else {
							this->currentScreen.status.fullScreen = false;
						}

						delete [] mCustomRecordedGameData;
						mCustomRecordedGameData =
								stringDuplicate( readCustomGameData );
					}
					else
					{
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

		for( int i=0; i<numChildren; i++ )
		{
			delete childFiles[i];
		}
	}

	delete [] childFiles;
	mStartedFullScreen = this->currentScreen.status.fullScreen;
	setupSurface();
	SDL_WM_SetCaption( inWindowName, NULL );
	SDL_EnableKeyRepeat( 0, 0 );// turn off repeat
	SDL_EnableUNICODE( true );

	for( int i=0; i<256; i++ )
	{
		keyMap[i] = (unsigned char)i;
	}

	/******************************************************************************************************************/
	//!set status
	this->status.connectedMode = false;

	//!set data
	memset(&(this->data.socket.ip), 0, sizeof(this->data.socket.ip));
	this->data.socket.port = 0;

	//!set components
	this->connection = nullptr;

	//!set controllers
	this->controller.gameSceneController = nullptr;

	//!
	this->lastSignalValue = signal::NONE;
	this->currentController = nullptr;
	this->useCustomServer = false;
	this->serverMessage = nullptr;

	this->deviceListener = new OneLife::game::DeviceListener();

	this->messageChannel = new OneLife::game::component::MessageChannel();
	GamePage::setMessageChannel(this->messageChannel);

	this->screenRenderer = new OneLife::game::ScreenRenderer(this->currentScreen);
	this->screenRenderer->setDefault(
			mWide,
			mHigh,
			mForceAspectRatio,
			mDoNotChangeNativeResolution,
			this->currentScreen.status.fullScreen,
			mForceSpecifiedDimensions
	);

	this->isControllerRecentlySet = false;
	this->player = nullptr;

	this->quit = false;
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
	/******************************************************************************************************************/
	if(this->serverMessage) free(this->serverMessage);
}

void OneLife::game::Application::init(OneLife::game::Settings settings)
{
	unsigned long bufferSize = sizeof(char)*1024*5;
	this->serverMessage = (char*)malloc(bufferSize);
	memset(this->serverMessage, 0, bufferSize);
}

void OneLife::game::Application::setConnection(const char* ip, int port)
{
	OneLife::game::Debug::writeMethodInfo("OneLife::game::Application::setConnection(\"%s\", %i)", ip, port);
	strcpy(this->data.socket.ip, ip);
	this->data.socket.port = port;
}

OneLife::game::component::Socket* OneLife::game::Application::getConnection()
{
	return this->connection;
}

void OneLife::game::Application::setUseCustomServerStatus(bool status)
{
	this->useCustomServer = status;
}

bool OneLife::game::Application::isUsingCustomServer()
{
	return this->useCustomServer;
}

void OneLife::game::Application::addFeature(void* feature)
{
	this->registeredFeature.push_back((OneLife::game::Feature*) feature);
}

/**********************************************************************************************************************/

/**
* Starts the GLUT main loop.
*
* Note that this function call never returns.
*/
void OneLife::game::Application::start()
{
	currentScreenGL = this;

	callbackResize( mWide, mHigh );// call our resize callback (GLUT used to do this for us when the window was created)

	initFrameDrawer( pixelZoomFactor * 320,
			pixelZoomFactor * 200,
			targetFrameRate,
			this->getCustomRecordedGameData(),
			this->isPlayingBack() );//!this->currentController initialized inside

	//!
	if(!this->controller.initializationScreen) this->controller.initializationScreen = new OneLife::game::InitializationScreen();
	this->controller.initializationScreen->setServerSocketAddress(this->data.socket);
	this->controller.initializationScreen->handle(this->controller.gameSceneController);
	this->setController(this->controller.initializationScreen);


	unsigned long oldFrameStart;
	timeSec_t frameStartSec;// FOVMOD NOTE:  Change 2/3 - Take these lines during the merge process
	unsigned long frameStartMSec;
	Time::getCurrentTime( &frameStartSec, &frameStartMSec );
	int oversleepMSec = 0;// oversleep on last loop (discount it from next sleep) can be negative (add to next sleep)

	// main loop
	OneLife::dataType::UiComponent dataScreen = {nullptr, nullptr, nullptr};
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
		this->readMessages();

		if(this->quit) break;

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

		this->sendClientMessage();

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
	if(this->isNewSystemEnable)
	{
		this->deviceListener->listen();
		if(this->deviceListener->receiveQuitSignal())
		{
			this->quit = true;
		}
		else//for(int i=0; i<this->deviceListener->getEvent()->size(); i++)
		{
			char* keyboard = (char*)this->deviceListener->getEvent().at(0).keyboard;

			if(keyboard[KEY::ALT]&&keyboard[KEY::TAB])
			{
				if(keyboard[KEY::ALT_LEFT])printf("\n===>type : ALT(left)+TAB");
				if(keyboard[KEY::ALT_RIGHT])printf("\n===>type : ALT(right)+TAB");
			}
			else if(keyboard[KEY::TAB])
			{
				printf("\n===>type : TAB");
			}
		}
	}
	else
	{
		this->_oldReadDevicesStatus();
	}
}

void OneLife::game::Application::_oldReadDevicesStatus()
{
	// now handle pending events BEFORE actually drawing the screen.
	// Thus, screen reflects all the latest events (not just those
	// that happened before any sleep called during the pre-display).
	// This makes controls much more responsive.

	SDL_Event event;

	while( !( mPlaybackEvents && mRecordingOrPlaybackStarted ) && SDL_PollEvent( &event ) )
	{

		SDLMod mods = SDL_GetModState();

		// alt-enter, toggle fullscreen (but only if we started there,
		// to prevent window content centering issues due to mWidth and
		// mHeight changes mid-game)
		if( mStartedFullScreen && event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN && ( ( mods & KMOD_META ) || ( mods & KMOD_ALT ) ) )
		{
			printf( "Toggling fullscreen\n" );

			this->currentScreen.status.fullScreen = !this->currentScreen.status.fullScreen;

			setupSurface();

			callbackResize( mWide, mHigh );

			// reload all textures into OpenGL
			SingleTextureGL::contextChanged();
		}
			// alt-tab when not in fullscreen mode
		else if( ! this->currentScreen.status.fullScreen &&
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
		else if( this->currentScreen.status.fullScreen &&
				 ! mMinimized &&
				 event.type == SDL_KEYDOWN &&
				 event.key.keysym.sym == SDLK_TAB &&
				 ( ( mods & KMOD_META ) || ( mods & KMOD_ALT ) ) ) {

			printf( "Minimizing from fullscreen on Alt-tab\n" );

			this->currentScreen.status.fullScreen = false;

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
			if( mRecordingEvents && mRecordingOrPlaybackStarted )
			{

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

			this->currentScreen.status.fullScreen = true;

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

	if( mPlaybackEvents && mRecordingOrPlaybackStarted && mEventFile != NULL )
	{
		if( !mTimeValuePlayedBack )
		{

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
		while( SDL_PollEvent( &event ) )
		{
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

void OneLife::game::Application::readMessages()
{
	//!message from game
	this->lastSignalValue = this->messageChannel->getLastSignal();
	this->messageChannel->setLastSignal(signal::NONE);

	//!message from server
	char *message = nullptr;
	while(message = getNextServerMessage())
	{
		OneLife::game::Trace::log(message);
		printf("\n===>receive message length %d message\n%s\n", (int)strlen( message ), message );

		messageType type = getMessageType( message );
		printf("\n===>messageType : %i", type);

		for(auto & feature : this->registeredFeature)
		{
			feature->readSocketMessage(message);
			OneLife::dataType::Message gameMessage = feature->getGameMessage();
			if(gameMessage.type != OneLife::dataType::message::TYPE::NONE)
			{
				printf("\n===>insert message in gameMessageQueue");
			}
			/*
			switch(feature->getType())
			{
				case OneLife::dataType::feature::Type::BASE:
					break;
			}
			 */
		}

/**********************************************************************************************************************

		if( mapPullMode && type != MAP_CHUNK ) {
// ignore it---map is a frozen snapshot in time
// or as close as we can get to it
			type = UNKNOWN;
		}
		if( type == SHUTDOWN  || type == FORCED_SHUTDOWN )
		{
			this->socket->close();

			setWaiting( false );
			setSignal( "serverShutdown" );

			delete [] message;
			return;
		}
		else if( type == SERVER_FULL )
		{
			this->socket->close();

			setWaiting( false );
			setSignal( "serverFull" );

			delete [] message;
			return;
		}
		else if( type == GLOBAL_MESSAGE ) {
			if( mTutorialNumber <= 0 ) {
// not in tutorial
// display this message

				char messageFromServer[200];
				sscanf( message, "MS\n%199s", messageFromServer );

				char *upper = stringToUpperCase( messageFromServer );

				char found;

				char *lines = replaceAll( upper, "**", "##", &found );
				delete [] upper;

				char *spaces = replaceAll( lines, "_", " ", &found );

				delete [] lines;


				mGlobalMessageShowing = true;
				mGlobalMessageStartTime = game_getCurrentTime();

				if( mLiveTutorialSheetIndex >= 0 ) {
					mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
							mTutorialHideOffset[ mLiveTutorialSheetIndex ];
				}
				mLiveTutorialSheetIndex ++;

				if( mLiveTutorialSheetIndex >= NUM_HINT_SHEETS ) {
					mLiveTutorialSheetIndex -= NUM_HINT_SHEETS;
				}
				mTutorialMessage[ mLiveTutorialSheetIndex ] =
						stringDuplicate( spaces );

// other tutorial messages don't need to be destroyed
				mGlobalMessagesToDestroy.push_back(
						(char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );

				mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
						mTutorialHideOffset[ mLiveTutorialSheetIndex ];

				mTutorialTargetOffset[ mLiveTutorialSheetIndex ].y -= 100;

				double longestLine = getLongestLine(
						(char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );

				mTutorialExtraOffset[ mLiveTutorialSheetIndex ].x = longestLine;


				delete [] spaces;
			}
		}
		else if( type == FLIP ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
				delete [] lines[0];
			}

			for( int i=1; i<numLines; i++ ) {
				int id = 0;
				int facingLeft = 0;

				int numRead =
						sscanf( lines[i], "%d %d", &id, &facingLeft );

				if( numRead == 2 ) {
					LiveObject *o = getLiveObject( id );

					if( o != NULL && ! o->inMotion ) {
						char flip = false;

						if( facingLeft && ! o->holdingFlip ) {
							o->holdingFlip = true;
							flip = true;
						}
						else if( ! facingLeft && o->holdingFlip ) {
							o->holdingFlip = false;
							flip = true;
						}
						if( flip ) {
							o->lastAnim = moving;
							o->curAnim = ground2;
							o->lastAnimFade = 1;

							o->lastHeldAnim = moving;
							o->curHeldAnim = held;
							o->lastHeldAnimFade = 1;
						}
					}
				}
				delete [] lines[i];
			}

			delete [] lines;
		}
		else if( type == CRAVING ) {
			int foodID = -1;
			int bonus = 0;

			int numRead =
					sscanf( message, "CR\n%d %d", &foodID, &bonus );

			if( numRead == 2 ) {
				setNewCraving( foodID, bonus );
			}
		}
		else if( type == SEQUENCE_NUMBER ) {
// need to respond with LOGIN message

			char challengeString[200];

// we don't use these for anything in client
			int currentPlayers = 0;
			int maxPlayers = 0;
			mRequiredVersion = versionNumber;

			sscanf( message,
					"SN\n"
					"%d/%d\n"
					"%199s\n"
					"%d\n", &currentPlayers, &maxPlayers, challengeString,
					&mRequiredVersion );


			if( mRequiredVersion > versionNumber ||
				( mRequiredVersion < versionNumber &&
				  mRequiredVersion < dataVersionNumber ) ) {

// if server is using a newer version than us, we must upgrade
// our client

// if server is using an older version, check that
// their version is not behind our data version at least
				this->socket->close();

				setWaiting( false );

				if( ! usingCustomServer &&
					mRequiredVersion < dataVersionNumber ) {
// we have a newer data version than the server
// the servers must be in the process of updating, and
// we connected at just the wrong time
// Don't display a confusing version mismatch message here.
					setSignal( "serverUpdate" );
				}
				else {
					setSignal( "versionMismatch" );
				}

				delete [] message;
				return;
			}

			char *pureKey = getPureAccountKey();

			char *password =
					SettingsManager::getStringSetting( "serverPassword" );

			if( password == NULL ) {
				password = stringDuplicate( "x" );
			}


			char *pwHash = hmac_sha1( password, challengeString );

			char *keyHash = hmac_sha1( pureKey, challengeString );

			delete [] pureKey;
			delete [] password;


// we record the number of characters sent for playback
// if playback is using a different email.ini setting, this
// will fail.
// So pad the email with up to 80 space characters
// Thus, the login message is always this same length

			char *twinExtra;

			if( userTwinCode != NULL ) {
				char *hash = computeSHA1Digest( userTwinCode );
				twinExtra = autoSprintf( " %s %d", hash, userTwinCount );
				delete [] hash;
			}
			else {
				twinExtra = stringDuplicate( "" );
			}


			char *outMessage;

			char *tempEmail;

			if( strlen( userEmail ) > 0 ) {
				std::string seededEmail = std::string( userEmail );

// If user doesn't have a seed in their email field
				if( seededEmail.find('|') == std::string::npos ) {
					std::string seedList = SettingsManager::getSettingContents( "spawnSeed", "" );
					std::string seed = "";
					if( seedList == "" ) {
						seed = "";
					} else if( seedList.find('\n') == std::string::npos ) {
						seed = seedList;
					} else if( seedList.find('\n') != std::string::npos ) {
						seed = seedList.substr( 0, seedList.find('\n') );
					}

// And if the user has a seed set in settings
					if( seed != "" ) {
// Add seed delim and then seed
						seededEmail += '|';
						seededEmail += seed;
					}
				}

				tempEmail = stringDuplicate( seededEmail.c_str() );
			}
			else {
// a blank email
// this will cause LOGIN message to have one less token

// stick a place-holder in there instead
				tempEmail = stringDuplicate( "blank_email" );
			}


			if( strlen( tempEmail ) <= 80 ) {
				outMessage = autoSprintf( "LOGIN %-80s %s %s %d%s#",
						tempEmail, pwHash, keyHash,
						mTutorialNumber, twinExtra );
			}
			else {
// their email is too long for this trick
// don't cut it off.
// but note that the playback will fail if email.ini
// doesn't match on the playback machine
				outMessage = autoSprintf( "LOGIN %s %s %s %d%s#",
						tempEmail, pwHash, keyHash,
						mTutorialNumber, twinExtra );
			}

			delete [] tempEmail;
			delete [] twinExtra;
			delete [] pwHash;
			delete [] keyHash;

			sendToServerSocket( outMessage );

			delete [] outMessage;

			delete [] message;
			return;
		}
		else if( type == ACCEPTED ) {
// logged in successfully, wait for next message

// subsequent messages should all be part of FRAME batches
			waitForFrameMessages = true;

			SettingsManager::setSetting( "loginSuccess", 1 );

			delete [] message;
			return;
		}
		else if( type == REJECTED )
		{
			this->socket->close();

			setWaiting( false );
			setSignal( "loginFailed" );

			delete [] message;
			return;
		}
		else if( type == NO_LIFE_TOKENS )
		{
			this->socket->close();

			setWaiting( false );
			setSignal( "noLifeTokens" );

			delete [] message;
			return;
		}
		else if( type == APOCALYPSE ) {
			apocalypseDisplayProgress = 0;
			apocalypseInProgress = true;
		}
		else if( type == APOCALYPSE_DONE ) {
			apocalypseDisplayProgress = 0;
			apocalypseInProgress = false;
			homePosStack.deleteAll();

// cancel all emots
			for( int i=0; i<gameObjects.size(); i++ ) {
				LiveObject *p = gameObjects.getElement( i );
				p->currentEmot = NULL;
			}
		}
		else if( type == MONUMENT_CALL ) {
			int posX, posY, monumentID;

			int numRead = sscanf( message, "MN\n%d %d %d",
					&posX, &posY, &monumentID );
			if( numRead == 3 ) {
				applyReceiveOffset( &posX, &posY );

				doublePair pos;
				pos.x = posX;
				pos.y = posY;

				LiveObject *ourLiveObject = getOurLiveObject();

				if( ourLiveObject != NULL ) {
					double d = distance( pos, ourLiveObject->currentPos );

					if( d > 32 ) {
						addAncientHomeLocation( posX, posY );

// play sound in distance
						ObjectRecord *monObj = getObject( monumentID );

						if( monObj != NULL &&
							monObj->creationSound.numSubSounds > 0 ) {

							doublePair realVector =
									getVectorFromCamera( lrint( posX ),
											lrint( posY ) );
// position off camera in that direction
// but fake distance
							realVector = mult( normalize( realVector ), 4 );

							playSound( monObj->creationSound, realVector );
						}
					}
				}
			}
		}
		else if( type == GRAVE ) {
			int posX, posY, playerID;

			int numRead = sscanf( message, "GV\n%d %d %d",
					&posX, &posY, &playerID );
			if( numRead == 3 ) {
				applyReceiveOffset( &posX, &posY );

				LiveObject *gravePerson = getLiveObject( playerID );

				if( gravePerson != NULL &&
					( gravePerson->relationName ||
					  gravePerson->name != NULL ) ) {

					GraveInfo g;
					g.worldPos.x = posX;
					g.worldPos.y = posY;
					g.creationTime = game_getCurrentTime();
					g.creationTimeUnknown = false;
					g.lastMouseOverYears = -1;
					g.lastMouseOverTime = g.creationTime;

					char *des = gravePerson->relationName;
					char *desToDelete = NULL;

					if( des == NULL ) {
						des = (char*)translate( "unrelated" );

						if( gravePerson->name == NULL ) {
// call them nameless instead
							des = (char*)translate( "namelessPerson" );
						}
					}
					if( gravePerson->name != NULL ) {
						des = autoSprintf( "%s - %s",
								gravePerson->name, des );
						desToDelete = des;
					}

					g.relationName = stringDuplicate( des );

					if( desToDelete != NULL ) {
						delete [] desToDelete;
					}

// this grave replaces any in same location
					for( int i=0; i< mGraveInfo.size(); i++ ) {
						GraveInfo *otherG = mGraveInfo.getElement( i );

						if( otherG->worldPos.x == posX &&
							otherG->worldPos.y == posY ) {

							delete [] otherG->relationName;
							mGraveInfo.deleteElement( i );
							i--;
						}
					}


					mGraveInfo.push_back( g );
				}
			}
		}
		else if( type == GRAVE_MOVE ) {
			int posX, posY, posXNew, posYNew;

			int swapDest = 0;

			int numRead = sscanf( message, "GM\n%d %d %d %d %d",
					&posX, &posY, &posXNew, &posYNew,
					&swapDest );
			if( numRead == 4 || numRead == 5 ) {
				applyReceiveOffset( &posX, &posY );
				applyReceiveOffset( &posXNew, &posYNew );


// handle case where two graves are "in the air"
// at the same time, and one gets put down where the
// other was picked up from before the other gets put down.
// (graves swapping position)

// When showing label to player, these are walked through
// in order until first match found

// So, we should walk through in reverse order until
// LAST match found, and then move that one to the top
// of the list.

// it will "cover up" the label of the still-matching
// grave further down on the list, which we will find
// and fix later when it fininall finishes moving.
				char found = false;
				for( int i=mGraveInfo.size() - 1; i >= 0; i-- ) {
					GraveInfo *g = mGraveInfo.getElement( i );

					if( g->worldPos.x == posX &&
						g->worldPos.y == posY ) {

						g->worldPos.x = posXNew;
						g->worldPos.y = posYNew;

						GraveInfo gStruct = *g;
						mGraveInfo.deleteElement( i );
						mGraveInfo.push_front( gStruct );
						found = true;
						break;
					}
				}

				if( found && ! swapDest ) {
// do NOT need to keep any extra ones around
// this fixes cases where old grave info is left
// behind, due to decay
					for( int i=1; i < mGraveInfo.size(); i++ ) {
						GraveInfo *g = mGraveInfo.getElement( i );

						if( g->worldPos.x == posXNew &&
							g->worldPos.y == posYNew ) {

// a stale match
							mGraveInfo.deleteElement( i );
							i--;
						}
					}
				}
			}
		}
		else if( type == GRAVE_OLD ) {
			int posX, posY, playerID, displayID;
			double age;

			int eveID = -1;


			char nameBuffer[200];

			nameBuffer[0] = '\0';

			int numRead = sscanf( message, "GO\n%d %d %d %d %lf %199s",
					&posX, &posY, &playerID, &displayID,
					&age, nameBuffer );
			if( numRead == 6 ) {
				applyReceiveOffset( &posX, &posY );

				GridPos thisPos = { posX, posY };

				for( int i=0; i<graveRequestPos.size(); i++ ) {
					if( equal( graveRequestPos.getElementDirect( i ),
							thisPos ) ) {
						graveRequestPos.deleteElement( i );
						break;
					}
				}

				int nameLen = strlen( nameBuffer );
				for( int i=0; i<nameLen; i++ ) {
					if( nameBuffer[i] == '_' ) {
						nameBuffer[i] = ' ';
					}
				}


				SimpleVector<int> otherLin;

				int numLines;

				char **lines = split( message, "\n", &numLines );

				if( numLines > 1 ) {
					SimpleVector<char *> *tokens =
							tokenizeString( lines[1] );

					int numNormalTokens = tokens->size();

					if( tokens->size() > 6 ) {
						char *lastToken =
								tokens->getElementDirect(
										tokens->size() - 1 );

						if( strstr( lastToken, "eve=" ) ) {
// eve tag at end
							numNormalTokens--;

							sscanf( lastToken, "eve=%d", &( eveID ) );
						}
					}

					for( int t=6; t<numNormalTokens; t++ ) {
						char *tok = tokens->getElementDirect( t );

						int mID = 0;
						sscanf( tok, "%d", &mID );

						if( mID != 0 ) {
							otherLin.push_back( mID );
						}
					}
					tokens->deallocateStringElements();
					delete tokens;
				}


				for( int i=0; i<numLines; i++ ) {
					delete [] lines[i];
				}
				delete [] lines;

				LiveObject *ourLiveObject = getOurLiveObject();

				char *relationName = getRelationName(
						&( ourLiveObject->lineage ),
						&otherLin,
						ourID,
						playerID,
						ourLiveObject->displayID,
						displayID,
						ourLiveObject->age,
						age,
						ourLiveObject->lineageEveID,
						eveID );

				GraveInfo g;
				g.worldPos.x = posX;
				g.worldPos.y = posY;

				char *des = relationName;
				char *desToDelete = NULL;

				if( des == NULL ) {
					des = (char*)translate( "unrelated" );

					if( strcmp( nameBuffer, "" ) == 0 ||
						strcmp( nameBuffer, "~" ) == 0 ) {
// call them nameless instead
						des = (char*)translate( "namelessPerson" );

						if( playerID == 0 ) {
// call them forgotten instead
							des = (char*)translate( "forgottenPerson" );
						}
					}
				}
				if( strcmp( nameBuffer, "" ) != 0 &&
					strcmp( nameBuffer, "~" ) != 0 ) {
					des = autoSprintf( "%s - %s",
							nameBuffer, des );
					desToDelete = des;
				}

				g.relationName = stringDuplicate( des );

				if( desToDelete != NULL ) {
					delete [] desToDelete;
				}

				if( relationName != NULL ) {
					delete [] relationName;
				}

				g.creationTime =
						game_getCurrentTime() - age / ourLiveObject->ageRate;

				if( age == -1 ) {
					g.creationTime = 0;
					g.creationTimeUnknown = true;
				}
				else {
					g.creationTimeUnknown = false;
				}

				g.lastMouseOverYears = -1;
				g.lastMouseOverTime = g.creationTime;

				mGraveInfo.push_back( g );
			}
		}
		else if( type == OWNER ) {
			SimpleVector<char*> *tokens = tokenizeString( message );

			if( tokens->size() >= 3 ) {
				int x = 0;
				int y = 0;

				sscanf( tokens->getElementDirect( 1 ), "%d", &x );
				sscanf( tokens->getElementDirect( 2 ), "%d", &y );

				GridPos thisPos = { x, y };

				for( int i=0; i<ownerRequestPos.size(); i++ ) {
					if( equal( ownerRequestPos.getElementDirect( i ),
							thisPos ) ) {
						ownerRequestPos.deleteElement( i );
						break;
					}
				}

				OwnerInfo *o = NULL;

				for( int i=0; i<mOwnerInfo.size(); i++ ) {
					OwnerInfo *thisInfo = mOwnerInfo.getElement( i );
					if( thisInfo->worldPos.x == x &&
						thisInfo->worldPos.y == y ) {

						o = thisInfo;
						break;
					}
				}
				if( o == NULL ) {
// not found, create new
					GridPos p = { x, y };

					OwnerInfo newO = { p, new SimpleVector<int>() };

					mOwnerInfo.push_back( newO );
					o = mOwnerInfo.getElement( mOwnerInfo.size() - 1 );
				}

				o->ownerList->deleteAll();
				for( int t=3; t < tokens->size(); t++ ) {
					int ownerID = 0;
					sscanf( tokens->getElementDirect( t ), "%d", &ownerID );
					if( ownerID > 0 ) {
						o->ownerList->push_back( ownerID );
					}
				}
			}
			tokens->deallocateStringElements();
			delete tokens;
		}
		else if( type == VALLEY_SPACING ) {
			sscanf( message, "VS\n%d %d",
					&valleySpacing, &valleyOffset );
		}
		else if( type == FLIGHT_DEST ) {
			int posX, posY, playerID;

			int numRead = sscanf( message, "FD\n%d %d %d",
					&playerID, &posX, &posY );
			if( numRead == 3 ) {
				applyReceiveOffset( &posX, &posY );

				LiveObject *flyingPerson = getLiveObject( playerID );

				if( flyingPerson != NULL ) {
// move them there instantly
					flyingPerson->xd = posX;
					flyingPerson->yd = posY;

					flyingPerson->xServer = posX;
					flyingPerson->yServer = posY;

					flyingPerson->currentPos.x = posX;
					flyingPerson->currentPos.y = posY;

					flyingPerson->currentSpeed = 0;
					flyingPerson->currentGridSpeed = 0;
					flyingPerson->destTruncated = false;

					flyingPerson->currentMoveDirection.x = 0;
					flyingPerson->currentMoveDirection.y = 0;

					if( flyingPerson->pathToDest != NULL ) {
						delete [] flyingPerson->pathToDest;
						flyingPerson->pathToDest = NULL;
					}

					flyingPerson->inMotion = false;


					if( flyingPerson->id == ourID )
					{
// special case for self

// jump camera there instantly
						lastScreenViewCenter.x = posX * CELL_D;
						lastScreenViewCenter.y = posY * CELL_D;
						setViewCenterPosition( lastScreenViewCenter.x,
								lastScreenViewCenter.y );

// show loading screen again
						mStartedLoadingFirstObjectSet = false;
						mDoneLoadingFirstObjectSet = false;
						mFirstObjectSetLoadingProgress = 0;
						mPlayerInFlight = true;
						this->sendTripMessage("Flying yo destination ...");

// home markers invalid now
						homePosStack.deleteAll();
					}
				}
			}
		}
		else if( type == VOG_UPDATE ) {
			int posX, posY;

			int numRead = sscanf( message, "VU\n%d %d",
					&posX, &posY );
			if( numRead == 2 ) {
				vogPos.x = posX;
				vogPos.y = posY;

				mObjectPicker.setPosition( vogPos.x * CELL_D + 510,
						vogPos.y * CELL_D + 90 );

// jump camp instantly
				lastScreenViewCenter.x = posX * CELL_D;
				lastScreenViewCenter.y = posY * CELL_D;
				setViewCenterPosition( lastScreenViewCenter.x,
						lastScreenViewCenter.y );

				mCurMouseOverCellFade = 1.0;

				mCurMouseOverCell.x = vogPos.x - mMapOffsetX + mMapD / 2;
				mCurMouseOverCell.y = vogPos.y - mMapOffsetY + mMapD / 2;
			}
		}
		else if( type == PHOTO_SIGNATURE ) {
			int posX, posY;

			char sig[100];

			int numRead = sscanf( message, "PH\n%d %d %99s",
					&posX, &posY, sig );
			if( numRead == 3 ) {
				photoSig = stringDuplicate( sig );
			}
			else {
				photoSig = stringDuplicate( "NO_SIG" );
			}
		}
		else if( type == MAP_CHUNK ) {

			int sizeX = 0;
			int sizeY = 0;
			int x = 0;
			int y = 0;

			int binarySize = 0;
			int compressedSize = 0;

			sscanf( message, "MC\n%d %d %d %d\n%d %d\n",
					&sizeX, &sizeY, &x, &y, &binarySize, &compressedSize );

			printf( "Got map chunk with bin size %d, compressed size %d\n",
					binarySize, compressedSize );

			if( ! mMapGlobalOffsetSet ) {

// we need 7 fraction bits to represent 128 pixels per tile
// 32-bit float has 23 significant bits, not counting sign
// that leaves 16 bits for tile coordinate, or 65,536
// Give two extra bits of wiggle room
				int maxOK = 16384;

				if( x < maxOK &&
					x > -maxOK &&
					y < maxOK &&
					y > -maxOK ) {
					printf( "First chunk isn't too far from center, using "
							"0,0 as our global offset\n" );

					mMapGlobalOffset.x = 0;
					mMapGlobalOffset.y = 0;
					mMapGlobalOffsetSet = true;
				}
				else {
					printf(
							"Using this first chunk center as our global offset:  "
							"%d, %d\n", x, y );
					mMapGlobalOffset.x = x;
					mMapGlobalOffset.y = y;
					mMapGlobalOffsetSet = true;
				}
			}

			applyReceiveOffset( &x, &y );

// recenter our in-ram sub-map around this new chunk
			int newMapOffsetX = x + sizeX/2;
			int newMapOffsetY = y + sizeY/2;

// move old cached map cells over to line up with new center

			int xMove = mMapOffsetX - newMapOffsetX;
			int yMove = mMapOffsetY - newMapOffsetY;

			int *newMap = new int[ mMapD * mMapD ];
			int *newMapBiomes = new int[ mMapD * mMapD ];
			int *newMapFloors = new int[ mMapD * mMapD ];


			double *newMapAnimationFrameCount = new double[ mMapD * mMapD ];
			double *newMapAnimationLastFrameCount = new double[ mMapD * mMapD ];

			double *newMapAnimationFrozenRotFameCount =
					new double[ mMapD * mMapD ];

			char *newMapAnimationFrozenRotFameCountUsed =
					new char[ mMapD * mMapD ];

			int *newMapFloorAnimationFrameCount = new int[ mMapD * mMapD ];

			AnimType *newMapCurAnimType = new AnimType[ mMapD * mMapD ];
			AnimType *newMapLastAnimType = new AnimType[ mMapD * mMapD ];
			double *newMapLastAnimFade = new double[ mMapD * mMapD ];

			doublePair *newMapDropOffsets = new doublePair[ mMapD * mMapD ];
			double *newMapDropRot = new double[ mMapD * mMapD ];
			SoundUsage *newMapDropSounds = new SoundUsage[ mMapD * mMapD ];

			doublePair *newMapMoveOffsets = new doublePair[ mMapD * mMapD ];
			double *newMapMoveSpeeds = new double[ mMapD * mMapD ];


			char *newMapTileFlips= new char[ mMapD * mMapD ];

			SimpleVector<int> *newMapContainedStacks =
					new SimpleVector<int>[ mMapD * mMapD ];

			SimpleVector< SimpleVector<int> > *newMapSubContainedStacks =
					new SimpleVector< SimpleVector<int> >[ mMapD * mMapD ];

			char *newMapPlayerPlacedFlags = new char[ mMapD * mMapD ];


			for( int i=0; i<mMapD *mMapD; i++ ) {
// starts uknown, not empty
				newMap[i] = -1;
				newMapBiomes[i] = -1;
				newMapFloors[i] = -1;

				int newX = i % mMapD;
				int newY = i / mMapD;

				int worldX = newX + mMapOffsetX - mMapD / 2;
				int worldY = newY + mMapOffsetY - mMapD / 2;

// each cell is different, but always the same
				newMapAnimationFrameCount[i] =
						lrint( getXYRandom( worldX, worldY ) * 10000 );
				newMapAnimationLastFrameCount[i] =
						newMapAnimationFrameCount[i];

				newMapAnimationFrozenRotFameCount[i] = 0;
				newMapAnimationFrozenRotFameCountUsed[i] = false;

				newMapFloorAnimationFrameCount[i] =
						lrint( getXYRandom( worldX, worldY ) * 13853 );



				newMapCurAnimType[i] = ground;
				newMapLastAnimType[i] = ground;
				newMapLastAnimFade[i] = 0;
				newMapDropOffsets[i].x = 0;
				newMapDropOffsets[i].y = 0;
				newMapDropRot[i] = 0;

				newMapDropSounds[i] = blankSoundUsage;

				newMapMoveOffsets[i].x = 0;
				newMapMoveOffsets[i].y = 0;
				newMapMoveSpeeds[i] = 0;

				newMapTileFlips[i] = false;
				newMapPlayerPlacedFlags[i] = false;



				int oldX = newX - xMove;
				int oldY = newY - yMove;

				if( oldX >= 0 && oldX < mMapD
					&&
					oldY >= 0 && oldY < mMapD ) {

					int oI = oldY * mMapD + oldX;

					newMap[i] = mMap[oI];
					newMapBiomes[i] = mMapBiomes[oI];
					newMapFloors[i] = mMapFloors[oI];

					newMapAnimationFrameCount[i] = mMapAnimationFrameCount[oI];
					newMapAnimationLastFrameCount[i] =
							mMapAnimationLastFrameCount[oI];

					newMapAnimationFrozenRotFameCount[i] =
							mMapAnimationFrozenRotFrameCount[oI];

					newMapAnimationFrozenRotFameCountUsed[i] =
							mMapAnimationFrozenRotFrameCountUsed[oI];

					newMapFloorAnimationFrameCount[i] =
							mMapFloorAnimationFrameCount[oI];

					newMapCurAnimType[i] = mMapCurAnimType[oI];
					newMapLastAnimType[i] = mMapLastAnimType[oI];
					newMapLastAnimFade[i] = mMapLastAnimFade[oI];
					newMapDropOffsets[i] = mMapDropOffsets[oI];
					newMapDropRot[i] = mMapDropRot[oI];
					newMapDropSounds[i] = mMapDropSounds[oI];

					newMapMoveOffsets[i] = mMapMoveOffsets[oI];
					newMapMoveSpeeds[i] = mMapMoveSpeeds[oI];

					newMapTileFlips[i] = mMapTileFlips[oI];


					newMapContainedStacks[i] = mMapContainedStacks[oI];
					newMapSubContainedStacks[i] = mMapSubContainedStacks[oI];

					newMapPlayerPlacedFlags[i] =
							mMapPlayerPlacedFlags[oI];
				}
			}

			memcpy( mMap, newMap, mMapD * mMapD * sizeof( int ) );
			memcpy( mMapBiomes, newMapBiomes, mMapD * mMapD * sizeof( int ) );
			memcpy( mMapFloors, newMapFloors, mMapD * mMapD * sizeof( int ) );

			memcpy( mMapAnimationFrameCount, newMapAnimationFrameCount,
					mMapD * mMapD * sizeof( double ) );
			memcpy( mMapAnimationLastFrameCount,
					newMapAnimationLastFrameCount,
					mMapD * mMapD * sizeof( double ) );

			memcpy( mMapAnimationFrozenRotFrameCount,
					newMapAnimationFrozenRotFameCount,
					mMapD * mMapD * sizeof( double ) );

			memcpy( mMapAnimationFrozenRotFrameCountUsed,
					newMapAnimationFrozenRotFameCountUsed,
					mMapD * mMapD * sizeof( char ) );


			memcpy( mMapFloorAnimationFrameCount,
					newMapFloorAnimationFrameCount,
					mMapD * mMapD * sizeof( int ) );


			memcpy( mMapCurAnimType, newMapCurAnimType,
					mMapD * mMapD * sizeof( AnimType ) );
			memcpy( mMapLastAnimType, newMapLastAnimType,
					mMapD * mMapD * sizeof( AnimType ) );
			memcpy( mMapLastAnimFade, newMapLastAnimFade,
					mMapD * mMapD * sizeof( double ) );
			memcpy( mMapDropOffsets, newMapDropOffsets,
					mMapD * mMapD * sizeof( doublePair ) );
			memcpy( mMapDropRot, newMapDropRot,
					mMapD * mMapD * sizeof( double ) );
			memcpy( mMapDropSounds, newMapDropSounds,
					mMapD * mMapD * sizeof( SoundUsage ) );

			memcpy( mMapMoveOffsets, newMapMoveOffsets,
					mMapD * mMapD * sizeof( doublePair ) );
			memcpy( mMapMoveSpeeds, newMapMoveSpeeds,
					mMapD * mMapD * sizeof( double ) );


			memcpy( mMapTileFlips, newMapTileFlips,
					mMapD * mMapD * sizeof( char ) );

// can't memcpy vectors
// need to assign them so copy constructors are invoked
			for( int i=0; i<mMapD *mMapD; i++ ) {
				mMapContainedStacks[i] = newMapContainedStacks[i];
				mMapSubContainedStacks[i] = newMapSubContainedStacks[i];
			}


			memcpy( mMapPlayerPlacedFlags, newMapPlayerPlacedFlags,
					mMapD * mMapD * sizeof( char ) );

			delete [] newMap;
			delete [] newMapBiomes;
			delete [] newMapFloors;
			delete [] newMapAnimationFrameCount;
			delete [] newMapAnimationLastFrameCount;
			delete [] newMapAnimationFrozenRotFameCount;
			delete [] newMapAnimationFrozenRotFameCountUsed;
			delete [] newMapFloorAnimationFrameCount;

			delete [] newMapCurAnimType;
			delete [] newMapLastAnimType;
			delete [] newMapLastAnimFade;
			delete [] newMapDropOffsets;
			delete [] newMapDropRot;
			delete [] newMapDropSounds;

			delete [] newMapMoveOffsets;
			delete [] newMapMoveSpeeds;

			delete [] newMapTileFlips;
			delete [] newMapContainedStacks;
			delete [] newMapSubContainedStacks;

			delete [] newMapPlayerPlacedFlags;



			mMapOffsetX = newMapOffsetX;
			mMapOffsetY = newMapOffsetY;


			unsigned char *compressedChunk =
					new unsigned char[ compressedSize ];


			for( int i=0; i<compressedSize; i++ ) {
				compressedChunk[i] = serverSocketBuffer.getElementDirect( i );
			}
			serverSocketBuffer.deleteStartElements( compressedSize );


			unsigned char *decompressedChunk =
					zipDecompress( compressedChunk,
							compressedSize,
							binarySize );

			delete [] compressedChunk;

			if( decompressedChunk == NULL ) {
				printf( "Decompressing chunk failed\n" );
			}
			else {

				unsigned char *binaryChunk =
						new unsigned char[ binarySize + 1 ];

				memcpy( binaryChunk, decompressedChunk, binarySize );

				delete [] decompressedChunk;


// for now, binary chunk is actually just ASCII
				binaryChunk[ binarySize ] = '\0';


				SimpleVector<char *> *tokens =
						tokenizeString( (char*)binaryChunk );

				delete [] binaryChunk;


				int numCells = sizeX * sizeY;

				if( tokens->size() == numCells ) {

					for( int i=0; i<tokens->size(); i++ ) {
						int cX = i % sizeX;
						int cY = i / sizeX;

						int mapX = cX + x - mMapOffsetX + mMapD / 2;
						int mapY = cY + y - mMapOffsetY + mMapD / 2;

						if( mapX >= 0 && mapX < mMapD
							&&
							mapY >= 0 && mapY < mMapD ) {


							int mapI = mapY * mMapD + mapX;
							int oldMapID = mMap[mapI];

							sscanf( tokens->getElementDirect(i),
									"%d:%d:%d",
									&( mMapBiomes[mapI] ),
									&( mMapFloors[mapI] ),
									&( mMap[mapI] ) );

							if( mMap[mapI] != oldMapID ) {
// our placement status cleared
								mMapPlayerPlacedFlags[mapI] = false;
							}

							mMapContainedStacks[mapI].deleteAll();
							mMapSubContainedStacks[mapI].deleteAll();

							if( strstr( tokens->getElementDirect(i), "," )
								!= NULL ) {

								int numInts;
								char **ints =
										split( tokens->getElementDirect(i),
												",", &numInts );

								delete [] ints[0];

								int numContained = numInts - 1;

								for( int c=0; c<numContained; c++ ) {
									SimpleVector<int> newSubStack;

									mMapSubContainedStacks[mapI].push_back(
											newSubStack );

									int contained = atoi( ints[ c + 1 ] );
									mMapContainedStacks[mapI].push_back(
											contained );

									if( strstr( ints[c + 1], ":" ) != NULL ) {
// sub-container items

										int numSubInts;
										char **subInts =
												split( ints[c + 1],
														":", &numSubInts );

										delete [] subInts[0];
										int numSubCont = numSubInts - 1;

										SimpleVector<int> *subStack =
												mMapSubContainedStacks[mapI].
														getElement(c);

										for( int s=0; s<numSubCont; s++ ) {
											subStack->push_back(
													atoi( subInts[ s + 1 ] ) );
											delete [] subInts[ s + 1 ];
										}

										delete [] subInts;
									}

									delete [] ints[ c + 1 ];
								}
								delete [] ints;
							}
						}
					}
				}

				tokens->deallocateStringElements();
				delete tokens;
			}

			if( mapPullMode ) {

				if( x == mapPullCurrentX - sizeX/2 &&
					y == mapPullCurrentY - sizeY/2 ) {

					lastScreenViewCenter.x = mapPullCurrentX * CELL_D;
					lastScreenViewCenter.y = mapPullCurrentY * CELL_D;
					setViewCenterPosition( lastScreenViewCenter.x,
							lastScreenViewCenter.y );

					mapPullCurrentX += 10;

					if( mapPullCurrentX > mapPullEndX ) {
						mapPullCurrentX = mapPullStartX;
						mapPullCurrentY += 5;

						if( mapPullCurrentY > mapPullEndY ) {
							mapPullModeFinalImage = true;
						}
					}
					mapPullCurrentSaved = false;
					mapPullCurrentSent = false;
				}
			}
		}
		else if( type == MAP_CHANGE ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip fist
				delete [] lines[0];
			}

			char idBuffer[500];

			for( int i=1; i<numLines; i++ ) {

				int x, y, floorID, responsiblePlayerID;
				int oldX, oldY;
				float speed = 0;


				int numRead = sscanf( lines[i], "%d %d %d %499s %d %d %d %f",
						&x, &y, &floorID,
						idBuffer, &responsiblePlayerID,
						&oldX, &oldY, &speed );
				if( numRead == 5 || numRead == 8) {

					applyReceiveOffset( &x, &y );

					int mapX = x - mMapOffsetX + mMapD / 2;
					int mapY = y - mMapOffsetY + mMapD / 2;

					if( mapX >= 0 && mapX < mMapD
						&&
						mapY >= 0 && mapY < mMapD ) {

						int mapI = mapY * mMapD + mapX;

						int oldFloor = mMapFloors[ mapI ];

						mMapFloors[ mapI ] = floorID;


						int old = mMap[mapI];

						int newID = -1;

						int oldContainedCount =
								mMapContainedStacks[mapI].size();

						if( responsiblePlayerID != -1 ) {
							int rID = responsiblePlayerID;
							if( rID < -1 ) {
								rID = -rID;
							}
							LiveObject *rObj = getLiveObject( rID );

							if( rObj != NULL &&
								rObj->pendingReceivedMessages.size() > 0 ) {


								printf( "Holding MX message caused by "
										"%d until later, "
										"%d other messages pending for them\n",
										rID,
										rObj->pendingReceivedMessages.size() );

								rObj->pendingReceivedMessages.push_back(
										autoSprintf( "MX\n%s\n#",
												lines[i] ) );

								delete [] lines[i];
								continue;
							}
						}


						if( strstr( idBuffer, "," ) != NULL ) {
							int numInts;
							char **ints =
									split( idBuffer, ",", &numInts );


							newID = atoi( ints[0] );

							mMap[mapI] = newID;

							delete [] ints[0];

							SimpleVector<int> oldContained;
// player triggered
// with no changed to container
// look for contained change
							if( speed == 0 &&
								old == newID &&
								responsiblePlayerID < 0 ) {

								oldContained.push_back_other(
										&( mMapContainedStacks[mapI] ) );
							}

							mMapContainedStacks[mapI].deleteAll();
							mMapSubContainedStacks[mapI].deleteAll();

							int numContained = numInts - 1;

							for( int c=0; c<numContained; c++ ) {

								SimpleVector<int> newSubStack;

								mMapSubContainedStacks[mapI].push_back(
										newSubStack );

								mMapContainedStacks[mapI].push_back(
										atoi( ints[ c + 1 ] ) );

								if( strstr( ints[c + 1], ":" ) != NULL ) {
// sub-container items

									int numSubInts;
									char **subInts =
											split( ints[c + 1], ":", &numSubInts );

									delete [] subInts[0];
									int numSubCont = numSubInts - 1;

									SimpleVector<int> *subStack =
											mMapSubContainedStacks[mapI].
													getElement(c);

									for( int s=0; s<numSubCont; s++ ) {
										subStack->push_back(
												atoi( subInts[ s + 1 ] ) );
										delete [] subInts[ s + 1 ];
									}

									delete [] subInts;
								}

								delete [] ints[ c + 1 ];
							}
							delete [] ints;

							if( speed == 0 &&
								old == newID &&
								responsiblePlayerID < 0
								&&
								oldContained.size() ==
								mMapContainedStacks[mapI].size() ) {
// no change in number of items
// count number that change
								int changeCount = 0;
								int changeIndex = -1;
								for( int i=0; i<oldContained.size(); i++ ) {
									if( oldContained.
											getElementDirect( i )
										!=
										mMapContainedStacks[mapI].
												getElementDirect( i ) ) {
										changeCount++;
										changeIndex = i;
									}
								}
								if( changeCount == 1 ) {
// single item changed
// play sound for it?

									int oldContID =
											oldContained.
													getElementDirect( changeIndex );
									int newContID =
											mMapContainedStacks[mapI].
													getElementDirect( changeIndex );


// watch out for swap case, with single
// item
// don't play sound then
									LiveObject *causingPlayer =
											getLiveObject( - responsiblePlayerID );

									if( causingPlayer != NULL &&
										causingPlayer->holdingID
										!= oldContID ) {


										ObjectRecord *newObj =
												getObject( newContID );

										if( shouldCreationSoundPlay(
												oldContID, newContID ) ) {
											if( newObj->
													creationSound.numSubSounds
												> 0 ) {

												playSound(
														newObj->creationSound,
														getVectorFromCamera(
																x, y ) );
											}
										}
										else if(
												causingPlayer != NULL &&
												causingPlayer->holdingID == 0 &&
												bothSameUseParent( newContID,
														oldContID ) &&
												newObj->
														usingSound.numSubSounds > 0 ) {

											ObjectRecord *oldObj =
													getObject( oldContID );

// only play sound if new is
// less used than old (filling back
// up sound)
											if( getObjectParent( oldContID ) ==
												newContID ||
												oldObj->thisUseDummyIndex <
												newObj->thisUseDummyIndex ) {

												playSound(
														newObj->usingSound,
														getVectorFromCamera(
																x, y ) );
											}
										}
									}
								}
							}
						}
						else {
// a single int
							newID = atoi( idBuffer );
							mMap[mapI] = newID;
							mMapContainedStacks[mapI].deleteAll();
							mMapSubContainedStacks[mapI].deleteAll();
						}

						if( speed > 0 ) {
// this cell moved from somewhere

							applyReceiveOffset( &oldX, &oldY );


							GridPos oldPos = { oldX, oldY };

// check if we have a move-to object "in the air"
// that is supposed to end up at this location
// if so, make it snap there
							for( int i=0;
								 i<mMapExtraMovingObjects.size(); i++ ) {

								if( equal(
										mMapExtraMovingObjectsDestWorldPos.
												getElementDirect( i ),
										oldPos ) ) {
									endExtraObjectMove( i );
									break;
								}
							}



							int oldMapI = getMapIndex( oldX, oldY );

							int sourceObjID = 0;
							if( oldMapI != -1 ) {
								sourceObjID = mMap[ oldMapI ];

// check what move-trans for sourceID
// produces.  If it produces something
// show that moving instead

								TransRecord *moveTrans =
										getTrans( -1, sourceObjID );

								if( moveTrans != NULL &&
									moveTrans->move > 0 ) {
									sourceObjID = moveTrans->newTarget;
								}
							}

							ExtraMapObject oldObj;

							if( old > 0 && sourceObjID != 0 &&
								getTrans( sourceObjID, old ) != NULL ) {

// save old object while we
// set up new object
								oldObj = copyFromMap( mapI );
								oldObj.objectID = old;
								if( old == -1 ) {
									oldObj.objectID = 0;
								}
							}

							mMapMoveSpeeds[mapI] = speed;



							doublePair oldOffset = { 0, 0 };

							char moveEastOrWest = false;

							if( oldMapI != -1 ) {
// location where we came from
								oldOffset = mMapMoveOffsets[ oldMapI ];

								mMapAnimationFrameCount[ mapI ] =
										mMapAnimationFrameCount[ oldMapI ];

								mMapAnimationLastFrameCount[ mapI ] =
										mMapAnimationLastFrameCount[ oldMapI ];


								mMapAnimationFrozenRotFrameCount[ mapI ] =
										mMapAnimationFrozenRotFrameCount[
												oldMapI ];

								mMapCurAnimType[ mapI ] =
										mMapCurAnimType[ oldMapI ];
								mMapLastAnimType[ mapI ] =
										mMapLastAnimType[ oldMapI ];

								mMapLastAnimFade[ mapI ] =
										mMapLastAnimFade[ oldMapI ];

								if( mMapLastAnimFade[ mapI ] == 0 &&
									mMapCurAnimType[ mapI ] != moving ) {
// not in the middle of a fade

// fade to moving
									mMapLastAnimType[ mapI ] =
											mMapCurAnimType[ mapI ];
									mMapCurAnimType[ mapI ] = moving;
									mMapLastAnimFade[ mapI ] = 1;

									if( mMapAnimationFrozenRotFrameCountUsed[
											mapI ] ) {
										mMapAnimationFrameCount[ mapI ] =
												mMapAnimationFrozenRotFrameCount[
														oldMapI ];
									}
								}

								int oldLocID = mMap[ oldMapI ];

								if( oldLocID > 0 ) {
									TransRecord *decayTrans =
											getTrans( -1, oldLocID );

									if( decayTrans != NULL ) {

										if( decayTrans->move == 6 ||
											decayTrans->move == 7 ) {
											moveEastOrWest = true;
										}
									}
								}
							}

							double oldTrueX = oldX + oldOffset.x;
							double oldTrueY = oldY + oldOffset.y;

							mMapMoveOffsets[mapI].x = oldTrueX - x;
							mMapMoveOffsets[mapI].y = oldTrueY - y;

							if( moveEastOrWest || x > oldTrueX ) {
								mMapTileFlips[mapI] = false;
							}
							else if( x < oldTrueX ) {
								mMapTileFlips[mapI] = true;
							}


							if( old > 0 && sourceObjID != 0 &&
								getTrans( sourceObjID, old ) != NULL ) {

// now that we've set up new object in dest
// copy it
								ExtraMapObject newObj = copyFromMap( mapI );
// leave source in place during move
								newObj.objectID = sourceObjID;

// save it as an extra obj
								mMapExtraMovingObjects.push_back( newObj );
								GridPos worldDestPos = { x, y };
								mMapExtraMovingObjectsDestWorldPos.push_back(
										worldDestPos );

								mMapExtraMovingObjectsDestObjectIDs.push_back(
										newID );

// put old object back in place for now
								putInMap( mapI, &oldObj );
							}


							if( old == 0 && sourceObjID != 0 ) {
// object moving into empty spot
// track where it came from as old
// so that we don't play initial
// creation sound by accident
								old = sourceObjID;
							}
						}
						else {
							mMapMoveSpeeds[mapI] = 0;
							mMapMoveOffsets[mapI].x = 0;
							mMapMoveOffsets[mapI].y = 0;
						}


						TransRecord *nextDecayTrans = getTrans( -1, newID );

						if( nextDecayTrans != NULL ) {

							if( nextDecayTrans->move == 6 ||
								nextDecayTrans->move == 7 ) {
// this object will move to left/right in future
// force no flip now
								mMapTileFlips[mapI] = false;
							}
						}


						if( oldFloor != floorID && floorID > 0 ) {
// floor changed

							ObjectRecord *obj = getObject( floorID );
							if( obj->creationSound.numSubSounds > 0 ) {

								playSound( obj->creationSound,
										getVectorFromCamera( x, y ) );
							}
						}


						LiveObject *responsiblePlayerObject = NULL;

						if( responsiblePlayerID > 0 ) {
							responsiblePlayerObject =
									getGameObject( responsiblePlayerID );
						}

						if( old > 0 &&
							newID > 0 &&
							old != newID &&
							responsiblePlayerID == - ourID ) {

// check for photo triggered
							if( strstr( getObject( newID )->description,
									"+photo" ) != NULL ) {

								takingPhotoGlobalPos.x = x;
								takingPhotoGlobalPos.y = y;
								takingPhotoFlip = mMapTileFlips[ mapI ];
								takingPhoto = true;
							}

						}


						if( old > 0 &&
							old == newID &&
							mMapContainedStacks[mapI].size() >
							oldContainedCount &&
							responsiblePlayerObject != NULL &&
							responsiblePlayerObject->holdingID == 0 ) {

// target is changed container and
// responsible player's hands now empty


// first, try and play the "using"
// sound for the container
							char soundPlayed = false;

							SoundUsage contSound =
									getObject( mMap[mapI] )->usingSound;

							if( contSound.numSubSounds > 0 ) {
								playSound( contSound,
										getVectorFromCamera( x, y ) );
								soundPlayed = true;
							}

							if( ! soundPlayed ) {
// no container using sound defined


// play player's using sound

								SoundUsage s = getObject(
										responsiblePlayerObject->displayID )->
										usingSound;

								if( s.numSubSounds > 0 ) {
									playSound( s, getVectorFromCamera( x, y ) );
								}
							}
						}



						if( responsiblePlayerID == -1 ) {
// no one dropped this

// our placement status cleared
							mMapPlayerPlacedFlags[mapI] = false;
						}


// Check if a home marker has been set or removed
						if( responsiblePlayerID != -1 &&
							( old != 0 ||
							  newID != 0 ) ) {
// player-triggered change

							int rID = responsiblePlayerID;
							if( rID < -1 ) {
								rID = -rID;
							}

							if( rID == ourID ) {
// local player triggered

								char addedOrRemoved = false;

								if( newID != 0 &&
									getObject( newID )->homeMarker ) {

									addHomeLocation( x, y );
									addedOrRemoved = true;
								}
								else if( old != 0 &&
										 getObject( old )->homeMarker ) {
									removeHomeLocation( x, y );
									addedOrRemoved = true;
								}

								if( addedOrRemoved ) {
// look in region for our home locations
// that may have been removed by others
// (other people pulling up our stake)
// and clear them.  Our player can
// no longer clear them, because they
// don't exist on the map anymore

									for( int ry=y-7; ry<=y+7; ry++ ) {
										for( int rx=x-7; rx<=x+7; rx++ ) {

											int mapRX =
													rx - mMapOffsetX + mMapD / 2;
											int mapRY =
													ry - mMapOffsetY + mMapD / 2;

											if( mapRX >= 0 && mapRX < mMapD
												&&
												mapRY >= 0 && mapRY < mMapD ) {

												int mapRI =
														mapRY * mMapD + mapRX;

												int cellID = mMap[ mapRI ];

												if( cellID == 0 ||
													( cellID > 0 &&
													  ! getObject( cellID )->
															  homeMarker ) ) {

													removeHomeLocation(
															rx, ry );
												}
											}
										}
									}
								}
							}
						}



						if( old != newID &&
							newID != 0 &&
							responsiblePlayerID <= -1 ) {
// ID change, and not just a player setting
// an object down

							ObjectRecord *newObj = getObject( newID );


							if( old > 0 ) {

								if( responsiblePlayerID == -1 ) {
// object auto-decayed from some other
// object

// play decay sound
									ObjectRecord *obj = getObject( old );
									if( obj->decaySound.numSubSounds > 0 ) {

										playSound(
												obj->decaySound,
												getVectorFromCamera( x, y ) );
									}
								}
								else if( responsiblePlayerID < -1 ) {
// player caused this object to change

// sound will be played elsewhere
								}
							}



							if( newObj->creationSound.numSubSounds > 0 ) {

								if( old == 0 && responsiblePlayerID < -1 ) {
// new placement, but not set-down
// we don't have information
// from on-ground objects to
// check for ancestor relationships

// check what the player is left
// holding instead

									LiveObject *responsiblePlayerObject =
											getGameObject( -responsiblePlayerID );
									if( responsiblePlayerObject != NULL ) {

										old =
												responsiblePlayerObject->holdingID;
										if( old < 0 ) {
											old = 0;
										}

										if( old > 0 ) {
											TransRecord *p =
													getTransProducing( old,
															newID );

											if( p != NULL &&
												p->actor > 0 ) {
												old = p->actor;
											}
										}
									}
								}


								if( shouldCreationSoundPlay( old, newID ) ) {
									playSound( newObj->creationSound,
											getVectorFromCamera( x, y ) );
								}
							}
						}



						if( mMap[mapI] != 0 ) {
							ObjectRecord *newObj = getObject( mMap[mapI] );

							if( newObj->permanent && newObj->blocksWalking ) {
// clear the locally-stored flip for this
// tile
								if( speed == 0 ) //allow blocking objects that move to flip e.g. beaver
									mMapTileFlips[mapI] = false;
							}
						}

						if( old != mMap[mapI] && mMap[mapI] != 0 ) {
// new placement

							printf( "New placement, responsible=%d\n",
									responsiblePlayerID );

							if( mMapMoveSpeeds[mapI] == 0 ) {

								if( old == 0 ) {
// set down into an empty spot
// reset frame count
									mMapAnimationFrameCount[mapI] = 0;
									mMapAnimationLastFrameCount[mapI] = 0;
								}
								else {
// else, leave existing frame count alone,
// since object has simply gone through a
// transition

// UNLESS it is a force-zero-start
// animation
									AnimationRecord *newAnim =
											getAnimation( mMap[mapI], ground );

									if( newAnim != NULL  &&
										newAnim->forceZeroStart ) {

										mMapAnimationFrameCount[mapI] = 0;
										mMapAnimationLastFrameCount[mapI] = 0;
									}
								}


								mMapCurAnimType[mapI] = ground;
								mMapLastAnimType[mapI] = ground;
								mMapLastAnimFade[mapI] = 0;
								mMapAnimationFrozenRotFrameCount[mapI] = 0;
							}
							else {
								mMapLastAnimType[mapI] = mMapCurAnimType[mapI];
								mMapCurAnimType[mapI] = moving;
								if( mMapLastAnimType[mapI] != moving ) {
									mMapLastAnimFade[mapI] = 1;
								}
							}


							LiveObject *responsiblePlayerObject = NULL;


							if( responsiblePlayerID > 0 ) {
								responsiblePlayerObject =
										getGameObject( responsiblePlayerID );

							}
							if( responsiblePlayerID != -1 &&
								responsiblePlayerID != 0 &&
								getObjectHeight( mMap[mapI] ) < CELL_D ) {

// try flagging objects as through-mousable
// for any change caused by a player,
// as long as object is small enough
								mMapPlayerPlacedFlags[mapI] = true;
							}
							else if( getObjectHeight( mMap[mapI] ) >= CELL_D ) {
// object that has become tall enough
// that through-highlights feel strange
								mMapPlayerPlacedFlags[mapI] = false;
							}
// don't forget placement flags for objects
// that someone placed but have naturally changed,
// but remain short



							if( responsiblePlayerObject == NULL ||
								!responsiblePlayerObject->onScreen ) {

// set it down instantly, no drop animation
// (player's held offset isn't valid)
								AnimationRecord *animR =
										getAnimation( mMap[mapI], ground );

								if( animR != NULL &&
									animR->randomStartPhase ) {
									mMapAnimationFrameCount[mapI] =
											randSource2.getRandomBoundedInt(
													0, 10000 );
									mMapAnimationLastFrameCount[i] =
											mMapAnimationFrameCount[mapI];
								}
								mMapDropOffsets[mapI].x = 0;
								mMapDropOffsets[mapI].y = 0;
								mMapDropRot[mapI] = 0;
								mMapDropSounds[mapI] = blankSoundUsage;

								if( responsiblePlayerObject != NULL ) {
// copy their flip, even if off-screen
									mMapTileFlips[mapI] =
											responsiblePlayerObject->holdingFlip;
								}
								else if( responsiblePlayerID < -1 &&
										 old == 0 &&
										 mMap[ mapI ] > 0 &&
										 ! getObject( mMap[ mapI ] )->
												 permanent ) {
// use-on-bare-ground
// with non-permanent result
// honor flip direction of player
									mMapTileFlips[mapI] =
											getLiveObject( -responsiblePlayerID )->
													holdingFlip;
								}
							}
							else {
// copy last frame count from last holder
// of this object (server tracks
// who was holding it and tell us about it)

								mMapLastAnimType[mapI] = held;
								mMapLastAnimFade[mapI] = 1;

								mMapAnimationFrozenRotFrameCount[mapI] =
										responsiblePlayerObject->
												heldFrozenRotFrameCount;


								if( responsiblePlayerObject->
										lastHeldAnimFade == 0 ) {

									mMapAnimationLastFrameCount[mapI] =
											responsiblePlayerObject->
													heldAnimationFrameCount;

									mMapLastAnimType[mapI] =
											responsiblePlayerObject->curHeldAnim;
								}
								else {
// dropped object was already
// in the middle of a fade
									mMapCurAnimType[mapI] =
											responsiblePlayerObject->curHeldAnim;

									mMapAnimationFrameCount[mapI] =
											responsiblePlayerObject->
													heldAnimationFrameCount;

									mMapLastAnimType[mapI] =
											responsiblePlayerObject->lastHeldAnim;

									mMapAnimationLastFrameCount[mapI] =
											responsiblePlayerObject->
													lastHeldAnimationFrameCount;


									mMapLastAnimFade[mapI] =
											responsiblePlayerObject->
													lastHeldAnimFade;
								}



								mMapDropOffsets[mapI].x =
										responsiblePlayerObject->
												heldObjectPos.x - x;

								mMapDropOffsets[mapI].y =
										responsiblePlayerObject->
												heldObjectPos.y - y;

								mMapDropRot[mapI] =
										responsiblePlayerObject->heldObjectRot;

								mMapDropSounds[mapI] =
										getObject(
												responsiblePlayerObject->displayID )->
												usingSound;

								mMapTileFlips[mapI] =
										responsiblePlayerObject->holdingFlip;

								if( responsiblePlayerObject->holdingID > 0 &&
									old == 0 ) {
// use on bare ground transition

// don't use drop offset
									mMapDropOffsets[mapI].x = 0;
									mMapDropOffsets[mapI].y = 0;

									mMapDropRot[mapI] = 0;
									mMapDropSounds[mapI] =
											blankSoundUsage;

									if( getObject( mMap[ mapI ] )->
											permanent ) {
// resulting in something
// permanent
// on ground.  Never flip it
										mMapTileFlips[mapI] = false;
									}
								}



								if( x >
									responsiblePlayerObject->xServer ) {
									responsiblePlayerObject->holdingFlip =
											false;
								}
								else if( x <
										 responsiblePlayerObject->xServer ) {
									responsiblePlayerObject->holdingFlip =
											true;
								}
							}
						}
					}
				}

				delete [] lines[i];
			}

			delete [] lines;
		}
		else if( type == PLAYER_UPDATE )
		{
		  //TODO search for message
#include "OneLife/gameSource/procedures/socket/player_update.cpp"
		}
		else if( type == PLAYER_MOVES_START ) {

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
		}
		else if( type == PLAYER_SAYS ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}


			for( int i=1; i<numLines; i++ ) {

				int id = -1;
				int curseFlag = 0;

				int numRead = 0;

				if( strstr( lines[i], "/" ) != NULL ) {
// new id/curse_flag format
					numRead = sscanf( lines[i], "%d/%d ", &id, &curseFlag );
				}
				else {
// old straight ID format
					numRead = sscanf( lines[i], "%d ", &id );
				}

				if( numRead >= 1 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == id ) {

							LiveObject *existing = gameObjects.getElement(j);

							if( existing->currentSpeech != NULL ) {
								delete [] existing->currentSpeech;
								existing->currentSpeech = NULL;
							}

							char *firstSpace = strstr( lines[i], " " );

							if( firstSpace != NULL ) {
								existing->currentSpeech =
										stringDuplicate( &( firstSpace[1] ) );

								existing->speechFade = 1.0;

								existing->speechIsSuccessfulCurse = curseFlag;

// longer time for longer speech
								existing->speechFadeETATime =
										game_getCurrentTime() + 3 +
										strlen( existing->currentSpeech ) / 5;

								if( existing->age < 1 &&
									existing->heldByAdultID == -1 ) {
// make 0-y-old unheld baby revert to
// crying age every time they speak
									existing->tempAgeOverrideSet = true;
									existing->tempAgeOverride = 0;
									existing->tempAgeOverrideSetTime =
											game_getCurrentTime();
								}

								if( curseFlag && mCurseSound != NULL ) {
									playSound(
											mCurseSound,
											0.5, // a little loud, tweak it
											getVectorFromCamera(
													existing->currentPos.x,
													existing->currentPos.y ) );
								}
							}

							break;
						}
					}

				}

				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == LOCATION_SAYS ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}

			for( int i=1; i<numLines; i++ ) {
				int x = 0;
				int y = 0;

				int numRead = sscanf( lines[i], "%d %d", &x, &y );


				if( numRead == 2 ) {

					char *firstSpace = strstr( lines[i], " " );

					if( firstSpace != NULL ) {
						char *secondSpace = strstr( &( firstSpace[1] ), " " );


						if( secondSpace != NULL ) {

							char *speech = &( secondSpace[1] );

							LocationSpeech ls;

							ls.pos.x = x * CELL_D;
							ls.pos.y = y * CELL_D;

							ls.speech = stringDuplicate( speech );

							ls.fade = 1.0;

// longer time for longer speech
							ls.fadeETATime =
									game_getCurrentTime() + 3 +
									strlen( ls.speech ) / 5;

							locationSpeech.push_back( ls );
						}
					}
				}

				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == PLAYER_EMOT ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}

			for( int i=1; i<numLines; i++ ) {
				int pid, emotIndex;
				int ttlSec = 0;

				int numRead = sscanf( lines[i], "%d %d %d",
						&pid, &emotIndex, &ttlSec );

				if( numRead >= 2 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == pid ) {

							LiveObject *existing = gameObjects.getElement(j);
							Emotion *newEmotPlaySound = NULL;

							if( ttlSec < 0 ) {
// new permanent emot layer
								newEmotPlaySound = getEmotion( emotIndex );

								if( newEmotPlaySound != NULL ) {
									if( existing->permanentEmots.
											getElementIndex(
											newEmotPlaySound ) == -1 ) {

										existing->permanentEmots.push_back(
												newEmotPlaySound );
									}
								}
								if( ttlSec == -2 ) {
// old emot that we're just learning about
// skip sound
									newEmotPlaySound = NULL;
								}
							}
							else {

								Emotion *oldEmot = existing->currentEmot;

								existing->currentEmot = getEmotion( emotIndex );

								if( numRead == 3 && ttlSec > 0 ) {
									existing->emotClearETATime =
											game_getCurrentTime() + ttlSec;
								}
								else {
// no ttl provided by server, use default
									existing->emotClearETATime =
											game_getCurrentTime() + emotDuration;
								}

								if( oldEmot != existing->currentEmot &&
									existing->currentEmot != NULL ) {
									newEmotPlaySound = existing->currentEmot;
								}
							}

							if( newEmotPlaySound != NULL ) {
								doublePair playerPos = existing->currentPos;

								for( int i=0;
									 i<getEmotionNumObjectSlots(); i++ ) {

									int id =
											getEmotionObjectByIndex(
													newEmotPlaySound, i );

									if( id > 0 ) {
										ObjectRecord *obj = getObject( id );

										if( obj->creationSound.numSubSounds
											> 0 ) {

											playSound(
													obj->creationSound,
													getVectorFromCamera(
															playerPos.x,
															playerPos.y ) );
// stop after first sound played
											break;
										}
									}
								}
							}
// found matching player, done
							break;
						}
					}
				}
				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == LINEAGE )
		{
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}


			for( int i=1; i<numLines; i++ ) {

				int id;
				int numRead = sscanf( lines[i], "%d ",
						&( id ) );

				if( numRead == 1 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == id ) {

							LiveObject *existing = gameObjects.getElement(j);

							existing->lineage.deleteAll();

							char *firstSpace = strstr( lines[i], " " );

							if( firstSpace != NULL ) {

								char *linStart = &( firstSpace[1] );

								SimpleVector<char *> *tokens =
										tokenizeString( linStart );

								int numNormalTokens = tokens->size();

								if( tokens->size() > 0 ) {
									char *lastToken =
											tokens->getElementDirect(
													tokens->size() - 1 );

									if( strstr( lastToken, "eve=" ) ) {
// eve tag at end
										numNormalTokens--;

										sscanf( lastToken, "eve=%d",
												&( existing->lineageEveID ) );
									}
								}

								for( int t=0; t<numNormalTokens; t++ ) {
									char *tok = tokens->getElementDirect( t );

									int mID = 0;
									sscanf( tok, "%d", &mID );

									if( mID != 0 ) {
										existing->lineage.push_back( mID );
									}
								}



								tokens->deallocateStringElements();
								delete tokens;
							}

							break;
						}
					}

				}

				delete [] lines[i];
			}
			delete [] lines;

// after a LINEAGE message, we should have lineage for all
// players

// now process each one and generate relation string
			LiveObject *ourObject = getOurLiveObject();

			for( int j=0; j<gameObjects.size(); j++ )
			{
				if( gameObjects.getElement(j)->id != ourID )
				{
					LiveObject *other = gameObjects.getElement(j);
					if( other->relationName == NULL )
					{
/-*
// test
ourObject->lineage.deleteAll();
other->lineage.deleteAll();

int cousinNum = 25;
int removeNum = 5;

for( int i=0; i<=cousinNum; i++ ) {
	ourObject->lineage.push_back( i );
	}

for( int i=0; i<=cousinNum - removeNum; i++ ) {
	other->lineage.push_back( 100 + i );
	}
other->lineage.push_back( cousinNum );
*-/

						other->relationName = getRelationName( ourObject, other );//TODO: uncomment
					}
				}
			}
		}
		else if( type == CURSED ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}


			for( int i=1; i<numLines; i++ ) {

				int id, level;
				int numRead = sscanf( lines[i], "%d %d",
						&id, &level );

				if( numRead == 2 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == id ) {

							LiveObject *existing = gameObjects.getElement(j);

							existing->curseLevel = level;
							break;
						}
					}

				}

				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == CURSE_TOKEN_CHANGE ) {
			LiveObject *ourLiveObject = getOurLiveObject();

			if( ourLiveObject != NULL ) {

				sscanf( message, "CX\n%d",
						&( ourLiveObject->curseTokenCount ) );
			}
		}
		else if( type == CURSE_SCORE ) {
			LiveObject *ourLiveObject = getOurLiveObject();

			if( ourLiveObject != NULL ) {

				sscanf( message, "CS\n%d",
						&( ourLiveObject->excessCursePoints ) );
			}
		}
		else if( type == PONG ) {
			sscanf( message, "PONG\n%d",
					&( lastPongReceived ) );
			if( lastPongReceived == lastPingSent ) {
				pongDeltaTime = game_getCurrentTime() - pingSentTime;
			}
		}
		else if( type == NAMES ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}


			for( int i=1; i<numLines; i++ ) {

				int id;
				int numRead = sscanf( lines[i], "%d ",
						&( id ) );

				if( numRead == 1 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == id ) {

							LiveObject *existing = gameObjects.getElement(j);

							if( existing->name != NULL ) {
								delete [] existing->name;
							}

							char *firstSpace = strstr( lines[i], " " );

							if( firstSpace != NULL ) {

								char *nameStart = &( firstSpace[1] );

								existing->name = stringDuplicate( nameStart );

								LiveObject *ourLiveObject = getOurLiveObject();
								if ( id == ourLiveObject->id &&
									 //Little hack here to not have the ding
									 //when we are just reconnected
									 //instead of a real name change
									 ourLiveObject->foodCapacity > 0 &&
									 mTutorialSound != NULL ) {
									playSound(
											mTutorialSound,
											0.18 * getSoundEffectsLoudness(),
											getVectorFromCamera(
													ourLiveObject->currentPos.x,
													ourLiveObject->currentPos.y ) );
								}
							}

							break;
						}
					}

				}

				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == DYING ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}


			for( int i=1; i<numLines; i++ ) {

				int id;
				int sickFlag = 0;

				int numRead = sscanf( lines[i], "%d %d",
						&( id ), &sickFlag );

				if( numRead >= 1 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == id ) {

							LiveObject *existing = gameObjects.getElement(j);

							existing->dying = true;
							if( sickFlag ) {
								existing->sick = true;
							}
							break;
						}
					}
				}
				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == HEALED ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}


			for( int i=1; i<numLines; i++ ) {

				int id;
				int numRead = sscanf( lines[i], "%d ",
						&( id ) );

				if( numRead == 1 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == id ) {

							LiveObject *existing = gameObjects.getElement(j);

							existing->dying = false;
							existing->sick = false;

// their wound will be gone after this
// play decay sound, if any, for their final
// wound state
							if( existing->holdingID > 0 ) {
								ObjectRecord *held =
										getObject( existing->holdingID );

								if( held->decaySound.numSubSounds > 0 ) {

									playSound(
											held->decaySound,
											getVectorFromCamera(
													existing->currentPos.x,
													existing->currentPos.y ) );
								}
							}
							break;
						}
					}
				}
				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == PLAYER_OUT_OF_RANGE ) {
			int numLines;
			char **lines = split( message, "\n", &numLines );

			if( numLines > 0 ) {
// skip first
				delete [] lines[0];
			}


			for( int i=1; i<numLines; i++ ) {

				int id;
				int numRead = sscanf( lines[i], "%d ",
						&( id ) );

				if( numRead == 1 ) {
					for( int j=0; j<gameObjects.size(); j++ ) {
						if( gameObjects.getElement(j)->id == id ) {

							LiveObject *existing = gameObjects.getElement(j);

							existing->outOfRange = true;

							if( existing->pendingReceivedMessages.size() > 0 ) {
// don't let pending messages for out-of-range
// players linger
								playPendingReceivedMessages( existing );
							}

							break;
						}
					}
				}
				delete [] lines[i];
			}
			delete [] lines;
		}
		else if( type == FOOD_CHANGE ) {

			LiveObject *ourLiveObject = getOurLiveObject();

			if( ourLiveObject != NULL ) {


				int lastAteID, lastAteFillMax;

				int responsiblePlayerID = -1;

				int foodStore;
				int foodCapacity;
				double lastSpeed;

				int oldYumBonus = mYumBonus;
				mYumBonus = 0;

				int oldYumMultiplier = mYumMultiplier;
				mYumMultiplier = 0;


				sscanf( message, "FX\n%d %d %d %d %lf %d %d %d",
						&( foodStore ),
						&( foodCapacity ),
						&( lastAteID ),
						&( lastAteFillMax ),
						&( lastSpeed ),
						&responsiblePlayerID,
						&mYumBonus, &mYumMultiplier );



				if( oldYumBonus != mYumBonus ) {
// pull out of old stack, if present
					for( int i=0; i<mOldYumBonus.size(); i++ ) {
						if( mOldYumBonus.getElementDirect( i ) == mYumBonus ) {
							mOldYumBonus.deleteElement( i );
							i--;
						}
					}

// fade existing
					for( int i=0; i<mOldYumBonus.size(); i++ ) {
						float fade =
								mOldYumBonusFades.getElementDirect( i );

						if( fade > 0.5 ) {
							fade -= 0.20;
						}
						else {
							fade -= 0.1;
						}

						*( mOldYumBonusFades.getElement( i ) ) = fade;
						if( fade <= 0 ) {
							mOldYumBonus.deleteElement( i );
							mOldYumBonusFades.deleteElement( i );
							i--;
						}
					}

					if( oldYumBonus != 0 ) {
// push on top of stack
						mOldYumBonus.push_back( oldYumBonus );
						mOldYumBonusFades.push_back( 1.0f );
					}
				}

				if( mYumMultiplier != oldYumMultiplier ) {
					int oldSlipIndex = -1;
					int newSlipIndex = 0;

					for( int i=0; i<2; i++ ) {
						if( mYumSlipNumberToShow[i] == oldYumMultiplier ) {
							oldSlipIndex = i;
							newSlipIndex = ( i + 1 ) % 2;
						}
					}
					if( oldSlipIndex != -1 ) {
						mYumSlipPosTargetOffset[ oldSlipIndex ] =
								mYumSlipHideOffset[ oldSlipIndex ];
					}

					mYumSlipPosTargetOffset[ newSlipIndex ] =
							mYumSlipHideOffset[ newSlipIndex ];

					if( mYumMultiplier > 0 ) {
						mYumSlipPosTargetOffset[ newSlipIndex ].y += 36;
					}
					mYumSlipNumberToShow[ newSlipIndex ] = mYumMultiplier;
				}




				if( responsiblePlayerID != -1 &&
					getLiveObject( responsiblePlayerID ) != NULL &&
					getLiveObject( responsiblePlayerID )->
							pendingReceivedMessages.size() > 0 ) {
// someone else fed us, and they're still in the
// middle of a local walk with updates pending after
// they finish

// defer this food update too

					LiveObject *rO = getLiveObject( responsiblePlayerID );

					printf( "Holding FX message caused by %d until later, "
							"%d other messages pending for them\n",
							responsiblePlayerID,
							rO->pendingReceivedMessages.size() );
					rO->pendingReceivedMessages.push_back(
							stringDuplicate( message ) );
				}
				else {
					char foodIncreased = false;
					int oldFoodStore = ourLiveObject->foodStore;

					if( foodCapacity == ourLiveObject->foodCapacity &&
						foodStore > ourLiveObject->foodStore ) {
						foodIncreased = true;
					}

					ourLiveObject->foodStore = foodStore;
					ourLiveObject->foodCapacity = foodCapacity;
					ourLiveObject->lastSpeed = lastSpeed;


					if( mCurrentLastAteString != NULL ) {


// one to add to erased list
// fade older ones first

						for( int i=0; i<mOldLastAteStrings.size(); i++ ) {
							float fade =
									mOldLastAteFades.getElementDirect( i );

							if( fade > 0.5 ) {
								fade -= 0.20;
							}
							else {
								fade -= 0.1;
							}

							*( mOldLastAteFades.getElement( i ) ) = fade;


// bar must fade slower (different blending mode)
							float barFade =
									mOldLastAteBarFades.getElementDirect( i );

							barFade -= 0.01;

							*( mOldLastAteBarFades.getElement( i ) ) = barFade;


							if( fade <= 0 ) {
								mOldLastAteStrings.deallocateStringElement( i );
								mOldLastAteFillMax.deleteElement( i );
								mOldLastAteFades.deleteElement( i );
								mOldLastAteBarFades.deleteElement( i );
								i--;
							}

							else if(
									strcmp(
											mCurrentLastAteString,
											mOldLastAteStrings.getElementDirect(i) )
									== 0 ) {
// already in stack, move to top
								mOldLastAteStrings.deallocateStringElement( i );
								mOldLastAteFillMax.deleteElement( i );
								mOldLastAteFades.deleteElement( i );
								mOldLastAteBarFades.deleteElement( i );
								i--;
							}
						}

						mOldLastAteStrings.push_back( mCurrentLastAteString );
						mOldLastAteFillMax.push_back( mCurrentLastAteFillMax );
						mOldLastAteFades.push_back( 1.0f );
						mOldLastAteBarFades.push_back( 1.0f );

						mCurrentLastAteString = NULL;
						mCurrentLastAteFillMax = 0;
					}

					if( lastAteID != 0 ) {
						ObjectRecord *lastAteObj = getObject( lastAteID );

						char *strUpper = stringToUpperCase(
								lastAteObj->description );

						stripDescriptionComment( strUpper );

						const char *key = "lastAte";

						if( lastAteObj->permanent ) {
							key = "lastAtePermanent";
						}

						mCurrentLastAteString =
								autoSprintf( "%s %s",
										translate( key ),
										strUpper );
						delete [] strUpper;

						mCurrentLastAteFillMax = lastAteFillMax;
					}
					else if( foodIncreased ) {
// we were fed, but we didn't eat anything
// must have been breast milk
						mCurrentLastAteString =
								stringDuplicate( translate( "breastMilk" ) );

						mCurrentLastAteFillMax = oldFoodStore;
					}


					printf( "Our food = %d/%d\n",
							ourLiveObject->foodStore,
							ourLiveObject->foodCapacity );


					if( ourLiveObject->foodStore >
						ourLiveObject->maxFoodStore ) {

						ourLiveObject->maxFoodStore = ourLiveObject->foodStore;
					}
					if( ourLiveObject->foodCapacity >
						ourLiveObject->maxFoodCapacity ) {

						ourLiveObject->maxFoodCapacity =
								ourLiveObject->foodCapacity;
					}
					if( ourLiveObject->foodStore ==
						ourLiveObject->foodCapacity ) {

						mPulseHungerSound = false;

						mHungerSlipVisible = 0;
					}
					else if( ourLiveObject->foodStore + mYumBonus <= 4 &&
							 computeCurrentAge( ourLiveObject ) < 117.33 ) {

// don't play hunger sounds at end of life
// because it interrupts our end-of-life song
// currently it's 2:37 long


// quiet music so hunger sound can be heard
						setMusicLoudness( 0 );
						mHungerSlipVisible = 2;

						if( ourLiveObject->foodStore > 0 ) {

							if( ourLiveObject->foodStore > 1 ) {
								if( mHungerSound != NULL ) {
// make sure it can be heard
// even if paused
									setSoundLoudness( 1.0 );
									playSoundSprite( mHungerSound,
											getSoundEffectsLoudness(),
// middle
											0.5 );
								}
								mPulseHungerSound = false;
							}
							else {
								mPulseHungerSound = true;
							}
						}
					}
					else if( ourLiveObject->foodStore + mYumBonus <= 8 ) {
						mHungerSlipVisible = 1;
						mPulseHungerSound = false;
					}
					else {
						mHungerSlipVisible = -1;
					}

					if( ourLiveObject->foodStore + mYumBonus > 4 ||
						computeCurrentAge( ourLiveObject ) >= 57 ) {
// restore music
						setMusicLoudness( musicLoudness );

						mPulseHungerSound = false;
					}

				}
			}
		}
		else if( type == HEAT_CHANGE ) {

			LiveObject *ourLiveObject = getOurLiveObject();

			if( ourLiveObject != NULL ) {
				sscanf( message, "HX\n%f",
						&( ourLiveObject->heat ) );

			}
		}

		delete [] message;

// process next message if there is one
		message = getNextServerMessage();
/**********************************************************************************************************************/
		message = nullptr;
	}

	/*
	if( mapPullMode && mapPullCurrentSaved && ! mapPullCurrentSent )
	{
		char *message = autoSprintf( "MAP %d %d#",
				sendX( mapPullCurrentX ),
				sendY( mapPullCurrentY ) );

		sendToServerSocket( message );


		delete [] message;
		mapPullCurrentSent = true;
	}
	*/
}

void OneLife::game::Application::selectScreen()
{
	if((void*)this->currentController == (void*)this->controller.initializationScreen)
	{
		if(this->isControllerRecentlySet)
		{
			OneLife::game::Debug::writeControllerInfo("Initialization");
			this->isControllerRecentlySet = false;
		}
		this->status.connectedMode = false;
		if(demoMode)
		{
			if(this->idScreen !=1){printf("\n===>demoMode");this->idScreen = 1;}
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
			if(this->idScreen !=2){printf("\n===>write failed");this->idScreen = 2;}
			drawString( translate( "writeFailed" ), true );
		}
		else if( !this->isPlayingBack() && measureFrameRate )
		{
			if(this->idScreen !=3){printf("\n===>!isPlayingBack && measureFrameRate");this->idScreen = 3;}
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
		else if( !loadingMessageShown ){
			if(this->idScreen !=4){printf("\n===>!loadingMessageShown");this->idScreen = 4;}
			drawString( translate( "loading" ), true );
			loadingMessageShown = true;
		}//step1
		else if( loadingFailedFlag ) {
			if(this->idScreen !=5){printf("\n===>loadingFailedFlag");this->idScreen = 5;}
			drawString( loadingFailedMessage, true );
		}
		else if( !writeFailed && !loadingFailedFlag && !frameDrawerInited ) {
			if(this->idScreen !=6){printf("\n===>!writeFailed && !loadingFailedFlag && !frameDrawerInited");this->idScreen = 6;}
			drawString( translate( "loading" ), true );
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

		}//step2
		else if(((OneLife::game::InitializationScreen*)this->currentController)->isTaskComplete())
		{
			this->setController(loadingPage);
		}
	}
	//!SEARCH LEG000001 for legacy place
	else if(this->currentController == loadingPage)
	{
		if(this->isControllerRecentlySet)
		{
			OneLife::game::Debug::writeControllerInfo("Loading assets");
			this->isControllerRecentlySet = false;
		}
		this->status.connectedMode = false;
		this->currentController->base_step();
		switch( loadingPhase )
		{
			case 0: {
				float progress;
				for( int i=0; i<loadingStepBatchSize; i++ ) {
					progress = initSpriteBankStep();
					loadingPage->setCurrentProgress( progress );
				}


				if( progress == 1.0 ) {
					initSpriteBankFinish();

					loadingPhaseStartTime = Time::getCurrentTime();

					char rebuilding;

					int numSounds = initSoundBankStart( &rebuilding );

					if( rebuilding ) {
						loadingPage->setCurrentPhase(
								translate( "soundsRebuild" ) );
					}
					else {
						loadingPage->setCurrentPhase(
								translate( "sounds" ) );
					}

					loadingPage->setCurrentProgress( 0 );


					loadingStepBatchSize = numSounds / numLoadingSteps;

					if( loadingStepBatchSize < 1 ) {
						loadingStepBatchSize = 1;
					}

					loadingPhase ++;
				}
				break;
			}
			case 1: {
				float progress;
				for( int i=0; i<loadingStepBatchSize; i++ ) {
					progress = initSoundBankStep();
					loadingPage->setCurrentProgress( progress );
				}

				if( progress == 1.0 ) {
					initSoundBankFinish();

					loadingPhaseStartTime = Time::getCurrentTime();

					char rebuilding;

					int numAnimations =
							initAnimationBankStart( &rebuilding );

					if( rebuilding ) {
						loadingPage->setCurrentPhase(
								translate( "animationsRebuild" ) );
					}
					else {
						loadingPage->setCurrentPhase(
								translate( "animations" ) );
					}
					loadingPage->setCurrentProgress( 0 );


					loadingStepBatchSize = numAnimations / numLoadingSteps;

					if( loadingStepBatchSize < 1 ) {
						loadingStepBatchSize = 1;
					}

					loadingPhase ++;
				}
				break;
			}
			case 2: {
				float progress;
				for( int i=0; i<loadingStepBatchSize; i++ ) {
					progress = initAnimationBankStep();
					loadingPage->setCurrentProgress( progress );
				}

				if( progress == 1.0 ) {
					initAnimationBankFinish();
					printf( "Finished loading animation bank in %f sec\n",
							Time::getCurrentTime() -
							loadingPhaseStartTime );
					loadingPhaseStartTime = Time::getCurrentTime();

					char rebuilding;

					int numObjects =
							initObjectBankStart( &rebuilding, true, true );

					if( rebuilding ) {
						loadingPage->setCurrentPhase(
								translate( "objectsRebuild" ) );
					}
					else {
						loadingPage->setCurrentPhase(
								translate( "objects" ) );
					}
					loadingPage->setCurrentProgress( 0 );


					loadingStepBatchSize = numObjects / numLoadingSteps;

					if( loadingStepBatchSize < 1 ) {
						loadingStepBatchSize = 1;
					}

					loadingPhase ++;
				}
				break;
			}
			case 3: {
				float progress;
				for( int i=0; i<loadingStepBatchSize; i++ ) {
					progress = initObjectBankStep();
					loadingPage->setCurrentProgress( progress );
				}

				if( progress == 1.0 ) {
					initObjectBankFinish();
					printf( "Finished loading object bank in %f sec\n",
							Time::getCurrentTime() -
							loadingPhaseStartTime );
					loadingPhaseStartTime = Time::getCurrentTime();

					char rebuilding;

					int numCats =
							initCategoryBankStart( &rebuilding );

					if( rebuilding ) {
						loadingPage->setCurrentPhase(
								translate( "categoriesRebuild" ) );
					}
					else {
						loadingPage->setCurrentPhase(
								translate( "categories" ) );
					}
					loadingPage->setCurrentProgress( 0 );


					loadingStepBatchSize = numCats / numLoadingSteps;

					if( loadingStepBatchSize < 1 ) {
						loadingStepBatchSize = 1;
					}

					loadingPhase ++;
				}
				break;
			}
			case 4: {
				float progress;
				for( int i=0; i<loadingStepBatchSize; i++ ) {
					progress = initCategoryBankStep();
					loadingPage->setCurrentProgress( progress );
				}

				if( progress == 1.0 ) {
					initCategoryBankFinish();
					printf( "Finished loading category bank in %f sec\n",
							Time::getCurrentTime() -
							loadingPhaseStartTime );
					loadingPhaseStartTime = Time::getCurrentTime();

					char rebuilding;

					// true to auto-generate concrete transitions
					// for all abstract category transitions
					int numTrans =
							initTransBankStart( &rebuilding, true, true, true,
									true );

					if( rebuilding ) {
						loadingPage->setCurrentPhase(
								translate( "transitionsRebuild" ) );
					}
					else {
						loadingPage->setCurrentPhase(
								translate( "transitions" ) );
					}
					loadingPage->setCurrentProgress( 0 );


					loadingStepBatchSize = numTrans / numLoadingSteps;

					if( loadingStepBatchSize < 1 ) {
						loadingStepBatchSize = 1;
					}

					loadingPhase ++;
				}
				break;
			}
			case 5: {
				float progress;
				for( int i=0; i<loadingStepBatchSize; i++ ) {
					progress = initTransBankStep();
					loadingPage->setCurrentProgress( progress );
				}

				if( progress == 1.0 ) {
					initTransBankFinish();
					printf( "Finished loading transition bank in %f sec\n",
							Time::getCurrentTime() -
							loadingPhaseStartTime );

					loadingPhaseStartTime = Time::getCurrentTime();

					loadingPage->setCurrentPhase(
							translate( "groundTextures" ) );

					loadingPage->setCurrentProgress( 0 );

					initGroundSpritesStart();

					loadingStepBatchSize = 1;


					loadingPhase ++;
				}
				break;
			}
			case 6: {
				float progress;
				for( int i=0; i<loadingStepBatchSize; i++ ) {
					progress = initGroundSpritesStep();
					loadingPage->setCurrentProgress( progress );
				}

				if( progress == 1.0 ) {
					initGroundSpritesFinish();
					printf( "Finished loading ground sprites in %f sec\n",
							Time::getCurrentTime() -
							loadingPhaseStartTime );

					loadingPhaseStartTime = Time::getCurrentTime();


					initLiveObjectSet();

					loadingPhaseStartTime = Time::getCurrentTime();

					serverIP = SettingsManager::getStringSetting("customServerAddress" );
					if( serverIP == NULL ) { serverIP = stringDuplicate( "127.0.0.1" ); }
					serverPort = SettingsManager::getIntSetting("customServerPort", 8005 );

					loadingPhase ++;
				}
				break;
			}
			default:
				// NOW game engine can start measuring frame rate
				loadingComplete();
				initEmotion();
				initPhotos();
				initLifeTokens();
				initFitnessScore();
				initMusicPlayer();
				setMusicLoudness( musicLoudness );
				mapPullMode = SettingsManager::getIntSetting( "mapPullMode", 0 );
				autoLogIn = SettingsManager::getIntSetting( "autoLogIn", 0 );
				if( userEmail == NULL || accountKey == NULL ){autoLogIn = false;}
				this->setController(existingAccountPage);
				this->currentController->base_makeActive( true );
		}
	}
	else if(this->currentController == getServerAddressPage)
	{
		if(this->isControllerRecentlySet)
		{
			OneLife::game::Debug::writeControllerInfo("Waiting Server Info settings");
			this->isControllerRecentlySet = false;
		}
		this->status.connectedMode = false;
		this->currentController->base_step();
		if( getServerAddressPage->isResponseReady() )
		{

			if( serverIP != NULL ) {
				delete [] serverIP;
			}

			serverIP = getServerAddressPage->getResponse( "serverIP" );
			serverPort = getServerAddressPage->getResponseInt( "serverPort" );

			if( strstr( serverIP, "NONE_FOUND" ) != NULL )
			{
				this->setController(finalMessagePage);
				finalMessagePage->setMessageKey( "serverShutdownMessage" );
				this->currentController->base_makeActive( true );
			}
			else
			{
				printf( "Got server address: %s:%d\n", serverIP, serverPort );
				int requiredVersion = getServerAddressPage->getResponseInt("requiredVersionNumber" );
				if( versionNumber < requiredVersion )
				{
					if( SettingsManager::getIntSetting(
							"useSteamUpdate", 0 ) )
					{

						// flag SteamGate that app needs update
						FILE *f = fopen( "steamGateForceUpdate.txt", "w" );
						if( f != NULL ) {
							fprintf( f, "1" );
							fclose( f );
						}

						// launch steamGateClient in parallel
						// it will tell Steam that the app is dirty
						// and needs to be updated.
						runSteamGateClient();



						this->currentController = finalMessagePage;

						finalMessagePage->setMessageKey(
								"upgradeMessageSteam" );

						this->currentController->base_makeActive( true );
					}
					else
					{
						char *autoUpdateURL =
								getServerAddressPage->getResponse(
										"autoUpdateURL" );

						char updateStarted =
								startUpdate( autoUpdateURL, versionNumber );

						delete [] autoUpdateURL;

						if( ! updateStarted ) {
							this->currentController = finalMessagePage;

							finalMessagePage->setMessageKey(
									"upgradeMessage" );

							this->currentController->base_makeActive( true );
						}
						else {
							this->currentController = autoUpdatePage;
							this->currentController->base_makeActive( true );
						}
					}
				}
				else
				{
					// up to date, okay to connect
					this->setController(livingLifePage);
					this->currentController->base_makeActive( true );
				}
			}
		}
	}
	else if(this->currentController == existingAccountPage)
	{
		if(this->isControllerRecentlySet)
		{
			OneLife::game::Debug::writeControllerInfo("Checking for existing account ...");
			this->isControllerRecentlySet = false;
		}
		this->status.connectedMode = false;
		this->currentController->base_step();
		if( existingAccountPage->checkSignal( "quit" ) ) {
			quitGame();
		}
		else if( existingAccountPage->checkSignal( "poll" ) )
		{
			this->setController(pollPage);
			this->currentController->base_makeActive( true );
		}
		else if( existingAccountPage->checkSignal( "genes" ) )
		{
			this->setController(geneticHistoryPage);
			this->currentController->base_makeActive( true );
		}
		else if( existingAccountPage->checkSignal( "settings" ) )
		{
			this->setController(settingsPage);
			this->currentController->base_makeActive( true );
		}
		else if( existingAccountPage->checkSignal( "review" ) )
		{
			this->setController(reviewPage);
			this->currentController->base_makeActive( true );
		}
		else if( existingAccountPage->checkSignal( "friends" ) )
		{
			this->setController(twinPage);
			this->currentController->base_makeActive( true );
		}
		else if( existingAccountPage->checkSignal( "done" )||mapPullMode || autoLogIn )
		{
			// auto-log-in one time for map pull
			// or one time for autoLogInMode
			mapPullMode = false;
			autoLogIn = false;

			// login button clears twin status
			// they have to login from twin page to play as twin
			if( userTwinCode != NULL ) {
				delete [] userTwinCode;
				userTwinCode = NULL;
			}
			this->setController(this->controller.mapGenerationScreen);
		}
		else if( existingAccountPage->checkSignal( "tutorial" ) )
		{
			livingLifePage->runTutorial();

			// tutorial button clears twin status
			// they have to login from twin page to play as twin
			if( userTwinCode != NULL )
			{
				delete [] userTwinCode;
				userTwinCode = NULL;
			}
			startConnecting();
		}
		else if( autoUpdatePage->checkSignal( "relaunchFailed" ) )
		{
			this->setController(finalMessagePage);
			finalMessagePage->setMessageKey( "manualRestartMessage" );
			this->currentController->base_makeActive( true );
		}
	}
	else if(this->currentController == (void*)this->controller.mapGenerationScreen)
	{
		if(this->isControllerRecentlySet)
		{
			OneLife::game::Debug::writeControllerInfo("Generating environment...");
			this->isControllerRecentlySet = false;
			OneLife::game::Debug::write("living life before handle : %p", livingLifePage);
			this->controller.mapGenerationScreen->handle(livingLifePage);
			OneLife::game::Debug::write("living life after handle : %p", livingLifePage);
		}
		this->status.connectedMode = false;
		switch(this->lastSignalValue)
		{
			case signal::DONE:
				this->setController(livingLifePage);
				startConnecting();
				break;
		}
	}
	else if(this->currentController == livingLifePage)
	{
		if(this->isControllerRecentlySet)
		{
			OneLife::game::Debug::writeControllerInfo("Living Life!");
			this->isControllerRecentlySet = false;
		}
		this->status.connectedMode = true;
		this->currentController->base_step();
		if( livingLifePage->checkSignal( "loginFailed" ) )
		{
			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;
			setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );
			this->setController(existingAccountPage);
			existingAccountPage->setStatus( "loginFailed", true );
			existingAccountPage->setStatusPositiion( true );
			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "noLifeTokens" ) )
		{
			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;
			setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );
			this->setController(existingAccountPage);
			existingAccountPage->setStatus( "noLifeTokens", true );
			existingAccountPage->setStatusPositiion( true );
			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "connectionFailed" ) )
		{
			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;
			setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );
			this->setController(existingAccountPage);
			existingAccountPage->setStatus( "connectionFailed", true );
			existingAccountPage->setStatusPositiion( true );
			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "versionMismatch" ) )
		{
			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;
			setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );
			this->setController(existingAccountPage);
			char *message = autoSprintf( translate( "versionMismatch" ),
					versionNumber,
					livingLifePage->
							getRequiredVersion() );

			if(!this->isUsingCustomServer())
			{
				existingAccountPage->showDisableCustomServerButton( true );
			}


			existingAccountPage->setStatusDirect( message, true );

			delete [] message;

			existingAccountPage->setStatusPositiion( true );

			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "twinCancel" ) ) {

			existingAccountPage->setStatus( NULL, false );

			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;

			setViewCenterPosition( lastScreenViewCenter.x,
					lastScreenViewCenter.y );

			this->currentController = existingAccountPage;

			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "serverShutdown" ) ) {
			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;

			setViewCenterPosition( lastScreenViewCenter.x,
					lastScreenViewCenter.y );

			this->currentController = existingAccountPage;

			existingAccountPage->setStatus( "serverShutdown", true );

			existingAccountPage->setStatusPositiion( true );

			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "serverUpdate" ) ) {
			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;
			setViewCenterPosition( lastScreenViewCenter.x,lastScreenViewCenter.y );
			this->setController(existingAccountPage);
			existingAccountPage->setStatus( "serverUpdate", true );
			existingAccountPage->setStatusPositiion( true );
			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "serverFull" ) ) {
			lastScreenViewCenter.x = 0;
			lastScreenViewCenter.y = 0;

			setViewCenterPosition( lastScreenViewCenter.x,
					lastScreenViewCenter.y );

			this->currentController = existingAccountPage;

			existingAccountPage->setStatus( "serverFull", true );

			existingAccountPage->setStatusPositiion( true );

			this->currentController->base_makeActive( true );
		}
		else if( livingLifePage->checkSignal( "died" ) ) {
			showDiedPage();
		}
		else if( livingLifePage->checkSignal( "disconnect" ) ) {
			showReconnectPage();
		}
		else if( livingLifePage->checkSignal( "loadFailure" ) ) {
			this->currentController = finalMessagePage;

			finalMessagePage->setMessageKey( "loadingMapFailedMessage" );

			char *failedFileName = getSpriteBankLoadFailure();
			if( failedFileName == NULL ) {
				failedFileName = getSoundBankLoadFailure();
			}

			if( failedFileName != NULL ) {

				char *detailMessage =
						autoSprintf( translate( "loadingMapFailedSubMessage" ),
								failedFileName );
				finalMessagePage->setSubMessage( detailMessage );
				delete [] detailMessage;
			}

			this->currentController->base_makeActive( true );
		}
	}
	else if( !writeFailed && !loadingFailedFlag  )//step3 (demo mode done or was never enabled )
	{
		this->status.connectedMode = false;
		if(this->idScreen !=7){printf("\n===>!writeFailed && !loadingFailedFlag");this->idScreen = 7;}

		if( this->currentController != NULL )
		{
			this->currentController->base_step();
			if( this->currentController == settingsPage ) {
				if( settingsPage->checkSignal( "back" ) ) {
					existingAccountPage->setStatus( NULL, false );
					this->currentController = existingAccountPage;
					this->currentController->base_makeActive( true );
				}
				else if( settingsPage->checkSignal( "editAccount" ) ) {
					loginEditOverride = true;

					existingAccountPage->setStatus( "editAccountWarning", false );
					existingAccountPage->setStatusPositiion( true );

					this->currentController = existingAccountPage;
					this->currentController->base_makeActive( true );
				}
				else if( settingsPage->checkSignal( "relaunchFailed" ) ) {
					this->currentController = finalMessagePage;

					finalMessagePage->setMessageKey( "manualRestartMessage" );

					this->currentController->base_makeActive( true );
				}

			}
			else if( this->currentController == reviewPage ) {
				if( reviewPage->checkSignal( "back" ) ) {
					existingAccountPage->setStatus( NULL, false );
					this->currentController = existingAccountPage;
					this->currentController->base_makeActive( true );
				}
			}
			else if( this->currentController == twinPage ) {
				if( twinPage->checkSignal( "cancel" ) ) {
					existingAccountPage->setStatus( NULL, false );
					this->currentController = existingAccountPage;
					this->currentController->base_makeActive( true );
				}
				else if( twinPage->checkSignal( "done" ) ) {
					printf("\n=====>startConnecting() livingLifePage done");
					startConnecting();
				}
			}
			else  if( this->currentController == autoUpdatePage ) {
				if( autoUpdatePage->checkSignal( "failed" ) ) {
					this->currentController = finalMessagePage;

					finalMessagePage->setMessageKey( "upgradeMessage" );

					this->currentController->base_makeActive( true );
				}
				else if( autoUpdatePage->checkSignal( "writeError" ) ) {
					this->currentController = finalMessagePage;

					finalMessagePage->setMessageKey(
							"updateWritePermissionMessage" );

					this->currentController->base_makeActive( true );
				}
				else if( autoUpdatePage->checkSignal( "relaunchFailed" ) ) {
					this->currentController = finalMessagePage;

					finalMessagePage->setMessageKey( "manualRestartMessage" );

					this->currentController->base_makeActive( true );
				}
			}
			else if( this->currentController == extendedMessagePage ) {
				if( extendedMessagePage->checkSignal( "done" ) ) {

					extendedMessagePage->setSubMessage( "" );

					if( userReconnect ) {
						printf("\n===>setting this->currentController to livingLifePage (extendedMessagePage)");
						this->currentController = livingLifePage;
					}
					else {
						this->currentController = pollPage;
					}
					this->currentController->base_makeActive( true );
				}
			}
			else if( this->currentController == pollPage ) {
				if( pollPage->checkSignal( "done" ) ) {
					this->currentController = rebirthChoicePage;
					this->currentController->base_makeActive( true );
				}
			}
			else if( this->currentController == geneticHistoryPage ) {
				if( geneticHistoryPage->checkSignal( "done" ) ) {
					if( !isHardToQuitMode() ) {
						this->currentController = existingAccountPage;
					}
					else {
						this->currentController = rebirthChoicePage;
					}
					this->currentController->base_makeActive( true );
				}
			}
			else if( this->currentController == rebirthChoicePage ) {
				if( rebirthChoicePage->checkSignal( "reborn" ) ) {
					// get server address again from scratch, in case
					// the server we were on just crashed

					// but keep twin status, if set
					printf("\n=====>startConnecting() geneticHistoryPage reborn");
					startConnecting();
				}
				else if( rebirthChoicePage->checkSignal( "tutorial" ) ) {
					livingLifePage->runTutorial();
					// heck, allow twins in tutorial too, for now, it's funny
					printf("\n=====>startConnecting() geneticHistoryPage tutorial");
					startConnecting();
				}
				else if( rebirthChoicePage->checkSignal( "review" ) ) {
					this->currentController = reviewPage;
					this->currentController->base_makeActive( true );
				}
				else if( rebirthChoicePage->checkSignal( "menu" ) ) {
					this->currentController = existingAccountPage;
					this->currentController->base_makeActive( true );
				}
				else if( rebirthChoicePage->checkSignal( "genes" ) ) {
					this->currentController = geneticHistoryPage;
					this->currentController->base_makeActive( true );
				}
				else if( rebirthChoicePage->checkSignal( "quit" ) ) {
					quitGame();
				}
			}
			else if( this->currentController == finalMessagePage ) {
				if( finalMessagePage->checkSignal( "quit" ) ) {
					quitGame();
				}
			}
		}

	}
}

void OneLife::game::Application::update(OneLife::dataType::UiComponent* dataScreen)
{
	if(!this->currentController) return;
	if( pauseOnMinimize && this->isMinimized() ) sceneHandler->setPause(true);// auto-pause when minimized
	this->currentController->handle(dataScreen);
}

/**
 *
 * @param dataScreen
 */
void OneLife::game::Application::render(OneLife::dataType::UiComponent* dataScreen)
{

	char update = !sceneHandler->isPaused();//TODO: paused is triggered in gameSceneHandler => change this! // don't update while paused
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
	sceneHandler->drawScene();//TODO: screenHandler/this->handle/update(screen)->readMessage(message);
	if(this->isNewSystemEnable)
	{
		char* keyboard = (char*)this->deviceListener->getEvent().at(0).keyboard;

		// alt-enter, toggle fullscreen (but only if we started there,
		// to prevent window content centering issues due to mWidth and
		// mHeight changes mid-game)
		if((mStartedFullScreen && keyboard[KEY::ALT] && keyboard[KEY::RETURN])
			|| (mStartedFullScreen && keyboard[KEY::META] && keyboard[KEY::RETURN]))
		{
			printf( "\nToggling fullscreen" );
			this->currentScreen.status.fullScreen = !this->currentScreen.status.fullScreen;
			this->screenRenderer->switchFullScreenMode();
		}
		/*
		// alt-tab when not in fullscreen mode
		else if( ! this->currentScreen.status.fullScreen &&
				 // ! mMinimized && //TODO: replace mMinimized with !this->currentScreen.status.fullScreen
				 ((keyboard[KEY::ALT] && keyboard[KEY::TAB]) || (keyboard[KEY::META] && keyboard[KEY::TAB])))
		{
			//mWantToMimimize = true;//TODO: delete (seem not used)
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
		else if( this->currentScreen.status.fullScreen &&
				 ! mMinimized &&
				 event.type == SDL_KEYDOWN &&
				 event.key.keysym.sym == SDLK_TAB &&
				 ( ( mods & KMOD_META ) || ( mods & KMOD_ALT ) ) )
		{
			printf( "Minimizing from fullscreen on Alt-tab\n" );

			this->currentScreen.status.fullScreen = false;

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
			if( mRecordingEvents && mRecordingOrPlaybackStarted )
			{

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
				 event.active.state == SDL_APPACTIVE )
		{
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
				 event.active.state == SDL_APPACTIVE )
		{
			// window becoming active out of minimization, needs
			// to return to full-screen mode

			printf( "Restoring to fullscreen after Alt-tab\n" );

			this->currentScreen.status.fullScreen = true;

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
				   ( ( event.key.keysym.unicode & 0xFF ) == 17 ) ) )
		{

			// map to 27, escape
			int mouseX, mouseY;
			SDL_GetMouseState( &mouseX, &mouseY );

			callbackKeyboard( 27, mouseX, mouseY );
		}
		*/
		else
		{
			/*
			switch( event.type )
			{
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

					if( mgKey != 0 )
					{
						if( event.type == SDL_KEYDOWN ) {
							callbackSpecialKeyboard( mgKey, mouseX, mouseY );
						}
						else {
							callbackSpecialKeyboardUp( mgKey, mouseX, mouseY );
						}
					}
					else
					{
						unsigned char asciiKey;

						// try unicode first, if 8-bit clean (extended ASCII)
						if( ( event.key.keysym.unicode & 0xFF00 ) == 0 && ( event.key.keysym.unicode & 0x00FF ) != 0 )
						{
							asciiKey = event.key.keysym.unicode & 0xFF;
						}
						else
						{
							// else unicode-to-ascii failed
							// fall back
							asciiKey = mapSDLKeyToASCII( event.key.keysym.sym );
						}

						if( asciiKey != 0 )
						{
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
			*/
		}
	}

	this->screenRenderer->render(dataScreen);
}

void OneLife::game::Application::sendClientMessage()
{
	//more than 15 seconds without client making a move send KA to keep connection open
	if(this->status.connectedMode && this->connection->isConnected() && this->connection->getTimeLastMessageSent()>15)
	{
		try
		{
			this->connection->sendMessage("KA 0 0#");
		}
		catch(OneLife::game::Exception* e)
		{
			//TODO: send disconnection message
		}
	}
	else//Socket is not connected
	{
	}
}

/**********************************************************************************************************************/

void OneLife::game::Application::setController(void* controller)
{
	this->currentController = (GamePage*)controller;
	currentGamePage = this->currentController;
	this->isControllerRecentlySet = true;
}

/**********************************************************************************************************************/

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
			if( this->currentScreen.status.fullScreen ) {
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

	if( this->currentScreen.status.fullScreen ) {
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

		if( this->currentScreen.status.fullScreen && mDoNotChangeNativeResolution ) {
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
	else if( mForceSpecifiedDimensions && this->currentScreen.status.fullScreen ) {

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

			this->currentScreen.status.fullScreen = false;

			flags = SDL_OPENGL;
#ifdef RASPBIAN
			flags = 0;
#endif
		}
	}
	else{

		if( this->currentScreen.status.fullScreen && mDoNotChangeNativeResolution ) {
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
	//screenGLStencilBufferSupported = true;//TODO: check if not used somewhere =>delete
#else
	int setStencilSize;
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &setStencilSize );
	if( setStencilSize > 0 ) {
		// we have a stencil buffer
		//screenGLStencilBufferSupported = true;//TODO: check if not used somewhere =>delete
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
