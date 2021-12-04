//
// Created by olivier on 04/12/2021.
//

#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/TranslationManager.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"
#include "minorGems/io/file/FileInputStream.h"
#include "minorGems/io/file/FileOutputStream.h"
#include "minorGems/util/ByteBufferInputStream.h"
#include "OneLife/gameSource/dataTypes/socket.h"
#include "OneLife/gameSource/components/engines/audioRenderer.h"
#include "OneLife/gameSource/application.h"
#include "OneLife/gameSource/components/engines/GameSceneHandler.h"

extern int totalLoadedTextureBytes;
extern int cursorMode;
extern double emulatedCursorScale;
extern char lShiftDown;
extern char rShiftDown;
extern char hardToQuitMode;
extern char countingOnVsync;
extern char loadingDone;
extern char *webProxy;
extern char shouldTakeScreenshot;
extern char outputAllFrames;
extern char *screenShotPrefix;
extern char manualScreenShot;
extern SimpleVector<WebRequestRecord> webRequestRecords;
extern OneLife::game::Application *gameApplication;
extern GameSceneHandler *sceneHandler;

unsigned int getRandSeed()
{
	return gameApplication->getRandSeed();
}

void pauseGame()
{
	sceneHandler->switchPause();
}

char isPaused()
{
	return sceneHandler->isPaused();
}

void blockQuitting( char inNoQuitting )
{
	sceneHandler->mBlockQuitting = inNoQuitting;
}

char isQuittingBlocked()
{
	return sceneHandler->mBlockQuitting;
}

void wakeUpPauseFrameRate()
{
	sceneHandler->mPausedSleepTime = 0;
}

// returns true if we're currently executing a recorded game
char isGamePlayingBack()
{
	return gameApplication->isPlayingBack();
}

void mapKey( unsigned char inFromKey, unsigned char inToKey )
{
	gameApplication->setKeyMapping( inFromKey, inToKey );
}

void toggleKeyMapping( char inMappingOn )
{
	gameApplication->toggleKeyMapping( inMappingOn );
}

void setCursorVisible( char inIsVisible )
{
	if( inIsVisible ) {
		SDL_ShowCursor( SDL_ENABLE );
	}
	else {
		SDL_ShowCursor( SDL_DISABLE );
	}
}

void setCursorMode( int inMode )
{
	SettingsManager::setSetting( "cursorMode", inMode );
	cursorMode = inMode;

	switch( cursorMode ) {
		case 0:
		case 2:
			setCursorVisible( true );
			break;
		case 1:
			setCursorVisible( false );
			break;
		default:
			setCursorVisible( true );
			break;
	}
}

int getCursorMode()
{
	return cursorMode;
}

void setEmulatedCursorScale( double inScale )
{
	SettingsManager::setDoubleSetting( "emulatedCursorScale", inScale );
	emulatedCursorScale = inScale;
}

double getEmulatedCursorScale()
{
	return emulatedCursorScale;
}

char isShiftKeyDown()
{
	SDLMod modState = SDL_GetModState();


	if( ( modState & KMOD_SHIFT ) ) {

		return true;
	}

	if( gameApplication->isPlayingBack() ) {
		// ignore these, saved internally, unless we're playing back
		// they can fall out of sync with keyboard reality as the user
		// alt-tabs between windows and release events are lost.
		if( rShiftDown || lShiftDown ) {
			return true;
		}
	}

	return false;
}

char isLastMouseButtonRight()
{
	return gameApplication->isLastMouseButtonRight();
}

// FOVMOD NOTE:  Change 1/1 - Take these lines during the merge process
int getLastMouseButton()
{
	return gameApplication->getLastMouseButton();
}

void obscureRecordedNumericTyping( char inObscure,
								   char inCharToRecordInstead )
{

	gameApplication->obscureRecordedNumericTyping( inObscure, inCharToRecordInstead );
}

static Image *readTGAFile( File *inFile )
{
	if( !inFile->exists() ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  TGA file %s does not exist",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;

		return NULL;
	}


	FileInputStream tgaStream( inFile );

	TGAImageConverter converter;

	Image *result = converter.deformatImage( &tgaStream );

	if( result == NULL ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  could not read TGA file %s, wrong format?",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;
	}

	return result;
}

Image *readTGAFile( const char *inTGAFileName )
{

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFile( &tgaFile );
}

Image *readTGAFileBase( const char *inTGAFileName )
{

	File tgaFile( NULL, inTGAFileName );

	return readTGAFile( &tgaFile );
}

static RawRGBAImage *readTGAFileRaw( InputStream *inStream )
{
	TGAImageConverter converter;

	RawRGBAImage *result = converter.deformatImageRaw( inStream );


	return result;
}

static RawRGBAImage *readTGAFileRaw( File *inFile )
{

	if( !inFile->exists() ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  TGA file %s does not exist",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;

		return NULL;
	}


	FileInputStream tgaStream( inFile );


	RawRGBAImage *result = readTGAFileRaw( &tgaStream );

	if( result == NULL ) {
		char *fileName = inFile->getFullFileName();

		char *logString = autoSprintf(
				"CRITICAL ERROR:  could not read TGA file %s, wrong format?",
				fileName );
		delete [] fileName;

		AppLog::criticalError( logString );
		delete [] logString;
	}

	return result;
}

RawRGBAImage *readTGAFileRaw( const char *inTGAFileName )
{

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}

RawRGBAImage *readTGAFileRawBase( const char *inTGAFileName )
{

	File tgaFile( NULL, inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}

RawRGBAImage *readTGAFileRawFromBuffer( unsigned char *inBuffer,
										int inLength )
										{

	ByteBufferInputStream tgaStream( inBuffer, inLength );

	return readTGAFileRaw( &tgaStream );
}

void writeTGAFile( const char *inTGAFileName, Image *inImage )
{
	File tgaFile( NULL, inTGAFileName );
	FileOutputStream tgaStream( &tgaFile );

	TGAImageConverter converter;

	return converter.formatImage( inImage, &tgaStream );
}

SpriteHandle fillSprite( RawRGBAImage *inRawImage ) {
	if( inRawImage->mNumChannels != 4 ) {
		printf( "Sprite not a 4-channel image, "
				"failed to load.\n" );

		return NULL;
	}

	return fillSprite( inRawImage->mRGBABytes,
			inRawImage->mWidth,
			inRawImage->mHeight );
}

SpriteHandle loadSprite( const char *inTGAFileName,
						 char inTransparentLowerLeftCorner ) {

	if( !inTransparentLowerLeftCorner ) {
		// faster to load raw, avoid double conversion
		RawRGBAImage *spriteImage = readTGAFileRaw( inTGAFileName );

		if( spriteImage != NULL ) {

			SpriteHandle result = fillSprite( spriteImage );

			delete spriteImage;

			return result;
		}
		else {
			printf( "Failed to load sprite from graphics/%s\n",
					inTGAFileName );
			return NULL;
		}
	}

	// or if trans corner, load converted to doubles for processing

	Image *result = readTGAFile( inTGAFileName );

	if( result == NULL ) {
		return NULL;
	}
	else {

		SpriteHandle sprite = fillSprite( result,
				inTransparentLowerLeftCorner );

		delete result;
		return sprite;
	}
}

SpriteHandle loadSpriteBase( const char *inTGAFileName,
							 char inTransparentLowerLeftCorner ) {
	if( !inTransparentLowerLeftCorner ) {
		// faster to load raw, avoid double conversion
		RawRGBAImage *spriteImage = readTGAFileRawBase( inTGAFileName );

		if( spriteImage != NULL ) {

			SpriteHandle result = fillSprite( spriteImage );

			delete spriteImage;

			return result;
		}
		else {
			printf( "Failed to load sprite from %s\n",
					inTGAFileName );
			return NULL;
		}
	}

	// or if trans corner, load converted to doubles for processing

	Image *result = readTGAFileBase( inTGAFileName );

	if( result == NULL ) {
		return NULL;
	}
	else {

		SpriteHandle sprite = fillSprite( result,
				inTransparentLowerLeftCorner );

		delete result;
		return sprite;
	}
}

const char *translate( const char *inTranslationKey ) {
	return TranslationManager::translate( inTranslationKey );
}

Image **screenShotImageDest = NULL;

void saveScreenShot( const char *inPrefix, Image **outImage ) {
	if( screenShotPrefix != NULL ) {
		delete [] screenShotPrefix;
	}
	screenShotPrefix = stringDuplicate( inPrefix );
	shouldTakeScreenshot = true;
	manualScreenShot = true;

	screenShotImageDest = outImage;
}



void startOutputAllFrames() {
	if( screenShotPrefix != NULL ) {
		delete [] screenShotPrefix;
	}
	screenShotPrefix = stringDuplicate( "frame" );

	outputAllFrames = true;
	shouldTakeScreenshot = true;
}


void stopOutputAllFrames() {
	outputAllFrames = false;
	shouldTakeScreenshot = false;
}

int nextWebRequestHandle = 0;




int startWebRequest( const char *inMethod, const char *inURL,
					 const char *inBody ) {

	WebRequestRecord r;

	r.handle = nextWebRequestHandle;
	nextWebRequestHandle ++;


	if( gameApplication->isPlayingBack() ) {
		// stop here, don't actually start a real web request
		return r.handle;
	}


	r.request = new WebRequest( inMethod, inURL, inBody, webProxy );

	webRequestRecords.push_back( r );

	return r.handle;
}



static WebRequest *getRequestByHandle( int inHandle ) {
	for( int i=0; i<webRequestRecords.size(); i++ ) {
		WebRequestRecord *r = webRequestRecords.getElement( i );

		if( r->handle == inHandle ) {
			return r->request;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - getRequestByHandle:  "
				   "Requested WebRequest handle not found\n" );
	return NULL;
}



int stepWebRequest( int inHandle ) {

	if( gameApplication->isPlayingBack() ) {
		// don't step request, because we're only simulating the response
		// of the server

		int nextType = gameApplication->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return 1;
		}
		else if( nextType == 1 ) {
			// recording said our result was ready
			// but it may not be the actual next result, due to timing
			// keep processing results until we see an actual 2 in the recording
			return 0;
		}
		else {
			return nextType;
		}
	}


	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {

		int stepResult = r->step();

		gameApplication->registerWebEvent( inHandle, stepResult );

		return stepResult;
	}

	return -1;
}



// gets the response body as a \0-terminated string, destroyed by caller
char *getWebResult( int inHandle ) {
	if( gameApplication->isPlayingBack() ) {
		// return a recorded server result

		int nextType = gameApplication->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return gameApplication->getWebEventResultBody( inHandle );
		}
		else {
			AppLog::error( "Expecting a web result body in playback file, "
						   "but found none." );

			return NULL;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		char *result = r->getResult();

		if( result != NULL ) {
			gameApplication->registerWebEvent( inHandle,
					// the type for "result" is 2
					2,
					result );
		}

		return result;
	}

	return NULL;
}




unsigned char *getWebResult( int inHandle, int *outSize ) {
	if( gameApplication->isPlayingBack() ) {
		// return a recorded server result

		int nextType = gameApplication->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return (unsigned char *)
					gameApplication->getWebEventResultBody( inHandle, outSize );
		}
		else {
			AppLog::error( "Expecting a web result body in playback file, "
						   "but found none." );

			return NULL;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		unsigned char *result = r->getResult( outSize );

		if( result != NULL ) {
			gameApplication->registerWebEvent( inHandle,
					// the type for "result" is 2
					2,
					(char*)result,
					*outSize );
		}

		return result;
	}

	return NULL;
}



int getWebProgressSize( int inHandle ) {
	if( gameApplication->isPlayingBack() ) {
		// return a recorded server result

		int nextType = gameApplication->getWebEventType( inHandle );

		if( nextType > 2 ) {
			return nextType;
		}
		else {
			AppLog::error(
					"Expecting a web result progress event in playback file, "
					"but found none." );

			return 0;
		}
	}



	WebRequest *r = getRequestByHandle( inHandle );

	if( r != NULL ) {
		int progress = r->getProgressSize();
		if( progress > 2 ) {
			gameApplication->registerWebEvent( inHandle,
					// the type for "progress" is
					// the actual size
					progress );
			return progress;
		}
		else {
			// progress of 2 or less is returned as 0, to keep consistency
			// for recording and playback
			return 0;
		}
	}

	return 0;
}




// frees resources associated with a web request
// if request is not complete, this cancels it
// if hostname lookup is not complete, this call might block.
void clearWebRequest( int inHandle ) {

	if( gameApplication->isPlayingBack() ) {
		// not a real request, do nothing
		return;
	}


	for( int i=0; i<webRequestRecords.size(); i++ ) {
		WebRequestRecord *r = webRequestRecords.getElement( i );

		if( r->handle == inHandle ) {
			delete r->request;

			webRequestRecords.deleteElement( i );

			// found, done
			return;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - clearWebRequest:  "
				   "Requested WebRequest handle not found\n" );
}


timeSec_t game_timeSec() {
	return gameApplication->getTimeSec();
}



double game_getCurrentTime() {
	return gameApplication->getCurrentTime();
}



double getRecentFrameRate() {
	if( gameApplication->isPlayingBack() ) {

		return gameApplication->getRecordedFrameRate();
	}
	else {
		gameApplication->registerActualFrameRate( sceneHandler->mLastFrameRate );

		return sceneHandler->mLastFrameRate;
	}
}



void loadingComplete() {
	loadingDone = true;
}


char getCountingOnVsync() {
	return countingOnVsync;
}



char isHardToQuitMode() {
	return hardToQuitMode;
}



// platform-specific clipboard code



#ifdef LINUX
static char clipboardSupportKnown = false;
static char clipboardSupport = false;
#endif


char isClipboardSupported() {
#ifdef LINUX
	// only check once, since system forks a process each time
    if( !clipboardSupportKnown ) {

        if( system( "which xclip > /dev/null 2>&1" ) ) {
            // xclip not installed
            AppLog::error( "xclip must be installed for clipboard to work" );
            clipboardSupport = false;
            }
        else {
            clipboardSupport = true;
            }
        clipboardSupportKnown = true;
        }
    return clipboardSupport;
#elif defined(__mac__)
	return true;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}


char isURLLaunchSupported() {
#ifdef LINUX
	return true;
#elif defined(__mac__)
	return true;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}









#ifdef LINUX
// X windows clipboard tutorial found here
// http://michael.toren.net/mirrors/doc/X-copy+paste.txt

// X11 has it's own Time type
// avoid conflicts with our Time class from above by replacing the word
// (Trick found here:  http://stackoverflow.com/questions/8865744 )
#define Time X11_Time

#include <X11/Xlib.h>
#include <X11/Xatom.h>


char *getClipboardText() {

    FILE* pipe = popen( "xclip -silent -selection clipboard -o", "r");
    if( pipe == NULL ) {
        return stringDuplicate( "" );
        }

    SimpleVector<char> textVector;

    char buffer[512];
    char *line = fgets( buffer, sizeof( buffer ), pipe );

    while( line != NULL ) {
        textVector.appendElementString( buffer );
        line = fgets( buffer, sizeof( buffer ), pipe );
        }

    pclose( pipe );


    return textVector.getElementString();
    }


void setClipboardText( const char *inText  ) {
    // x copy paste is a MESS
    // after claiming ownership of the clipboard, application needs
    // to listen to x events forever to handle any consumers of the clipboard
    // data.  Yuck!

    // farm this out to xclip with -silent flag
    // it forks its own process and keeps it live as long as the clipboard
    // data is still needed (kills itself when the clipboard is claimed
    // by someone else with new data)

    FILE* pipe = popen( "xclip -silent -selection clipboard -i", "w");
    if( pipe == NULL ) {
        return;
        }
    fputs( inText, pipe );

    pclose( pipe );
    }


void launchURL( char *inURL ) {
    char *call = autoSprintf( "xdg-open \"%s\" &", inURL );
    system( call );
    delete [] call;
    }




#elif defined(__mac__)

// pbpaste command line trick found here:
// http://www.alecjacobson.com/weblog/?p=2376

char *getClipboardText() {
    FILE* pipe = popen( "pbpaste", "r");
    if( pipe == NULL ) {
        return stringDuplicate( "" );
        }

    char buffer[ 128 ];

    char *result = stringDuplicate( "" );

    // read until pipe closed
    while( ! feof( pipe ) ) {
        if( fgets( buffer, 128, pipe ) != NULL ) {
            char *newResult = concatonate( result, buffer );
            delete [] result;
            result = newResult;
            }
        }
    pclose( pipe );


    return result;
    }



void setClipboardText( const char *inText  ) {
    FILE* pipe = popen( "pbcopy", "w");
    if( pipe == NULL ) {
        return;
        }
    fputs( inText, pipe );

    pclose( pipe );
    }



void launchURL( char *inURL ) {
    char *call = autoSprintf( "open \"%s\"", inURL );
    system( call );
    delete [] call;
    }




#elif defined(WIN_32)

// simple windows clipboard solution found here:
// https://www.allegro.cc/forums/thread/606034

#include <windows.h>

char *getClipboardText() {
    char *fromClipboard = NULL;
    if( OpenClipboard( NULL ) ) {
        HANDLE hData = GetClipboardData( CF_TEXT );
        char *buffer = (char*)GlobalLock( hData );
        if( buffer != NULL ) {
            fromClipboard = stringDuplicate( buffer );
            }
        GlobalUnlock( hData );
        CloseClipboard();
        }

    if( fromClipboard == NULL ) {
        fromClipboard = stringDuplicate( "" );
        }

    return fromClipboard;
    }


void setClipboardText( const char *inText  ) {
    if (OpenClipboard(NULL)) {

        EmptyClipboard();
        HGLOBAL clipBuffer = GlobalAlloc( GMEM_DDESHARE, strlen(inText) + 1 );
        char *buffer = (char*)GlobalLock( clipBuffer );

        strcpy( buffer, inText );
        GlobalUnlock( clipBuffer );
        SetClipboardData( CF_TEXT, clipBuffer );
        CloseClipboard();
        }
    }


void launchURL( char *inURL ) {
    // for some reason, on Windows, need extra set of "" before quoted URL
    // found here:
    // https://stackoverflow.com/questions/3037088/
    //         how-to-open-the-default-web-browser-in-windows-in-c

    // the wmic method allows spawning a browser without it lingering as
    // a child process
    // https://steamcommunity.com/groups/steamworks/
    //         discussions/0/154645427521397803/
    char *call = autoSprintf(
        "wmic process call create 'cmd /c start \"\" \"%s\"'", inURL );
    system( call );
    delete [] call;
    }



#else
// unsupported platform
char *getClipboardText() {
	return stringDuplicate( "" );
}

void launchURL( char *inURL ) {
}
#endif





#define macLaunchExtension ".app"
#define winLaunchExtension ".exe"

#define steamGateClientName "steamGateClient"


#ifdef LINUX

#include <unistd.h>
#include <stdarg.h>

char relaunchGame() {
    char *launchTarget =
        autoSprintf( "./%s", getLinuxAppName() );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[2] = { launchTarget, NULL };

        execvp( launchTarget, arguments );

        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;
    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    }

char runSteamGateClient() {
    char *launchTarget =
        autoSprintf( "./%s", steamGateClientName );

    AppLog::infoF( "Running steamGateClient: %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[2] = { launchTarget, NULL };

        execvp( launchTarget, arguments );

        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;
    printf( "Returning from launching steamGateClient\n" );
    return true;
    }


#elif defined(__mac__)

#include <unistd.h>
#include <stdarg.h>

char relaunchGame() {
    // Gatekeeper on 10.12 prevents relaunch from working
    // to be safe, just have user manually relaunch on Mac
    return false;

    /*
    char *launchTarget =
        autoSprintf( "%s_$d%s", getAppName(),
                     getAppVersion(), macLaunchExtension );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    int forkValue = fork();

    if( forkValue == 0 ) {
        // we're in child process, so exec command
        char *arguments[4] = { (char*)"open", (char*)"-n",
                               launchTarget, NULL };

        execvp( "open", arguments );
        // we'll never return from this call

        // small memory leak here, but okay
        delete [] launchTarget;
        }

    delete [] launchTarget;

    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    */
    }


char runSteamGateClient() {
    // have never tested this on Mac, who knows?
    return false;
    }



#elif defined(WIN_32)

#include <windows.h>
#include <process.h>

char relaunchGame() {
    char *launchTarget =
        autoSprintf( "%s%s", getAppName(), winLaunchExtension );

    AppLog::infoF( "Relaunching game %s", launchTarget );

    char *arguments[2] = { (char*)launchTarget, NULL };

    _spawnvp( _P_NOWAIT, launchTarget, arguments );

    delete [] launchTarget;

    printf( "Returning from relaunching game, exiting this process\n" );
    exit( 0 );
    return true;
    }



char runSteamGateClient() {
    char *launchTarget =
        autoSprintf( "%s%s", steamGateClientName, winLaunchExtension );

    AppLog::infoF( "Running steamGateClient: %s", launchTarget );

    char *arguments[2] = { (char*)launchTarget, NULL };

    _spawnvp( _P_NOWAIT, launchTarget, arguments );

    delete [] launchTarget;

    printf( "Returning from running steamGateClient\n" );
    return true;
    }


#else
// unsupported platform
char relaunchGame() {
	return false;
}

char runSteamGateClient() {
	return false;
}
#endif




void quitGame() {
	exit( 0 );
}




// true if platform supports sound recording, false otherwise
char isSoundRecordingSupported() {
#ifdef LINUX
	// check for arecord existence
    // The redirect to /dev/null ensures that your program does not produce
    // the output of these commands.
    // found here:
    // http://stackoverflow.com/questions/7222674/
    //             how-to-check-if-command-is-available-or-existant
    //
    int ret = system( "arecord --version > /dev/null 2>&1" );
    if( ret == 0 ) {
        return true;
        }
    else {
        return false;
        }
#elif defined(__mac__)
	return false;
#elif defined(WIN_32)
	return true;
#else
	return false;
#endif
}



#ifdef LINUX

static FILE *arecordPipe = NULL;
const char *arecordFileName = "inputSoundTemp.wav";
static int arecordSampleRate = 0;

// starts recording asynchronously
// keeps recording until stop called
char startRecording16BitMonoSound( int inSampleRate ) {
    if( arecordPipe != NULL ) {
        pclose( arecordPipe );
        arecordPipe = NULL;
        }

    arecordSampleRate = inSampleRate;

    char *arecordLine =
        autoSprintf( "arecord -f S16_LE -c1 -r%d %s",
                     inSampleRate, arecordFileName );

    arecordPipe = popen( arecordLine, "w" );

    delete [] arecordLine;

    if( arecordPipe == NULL ) {
        return false;
        }
    else {
        return true;
        }
    }



// returns array of samples destroyed by caller
int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    if( arecordPipe == NULL ) {
        return NULL;
        }

    // kill arecord to end the recording gracefully
    // this is reasonable to do because I can't imagine situations
    // where more than one arecord is running
    system( "pkill arecord" );

    pclose( arecordPipe );
    arecordPipe = NULL;

    int rate = -1;

    int16_t *data = load16BitMonoSound( outNumSamples, &rate );

    if( rate != arecordSampleRate ) {
        *outNumSamples = 0;

        if( data != NULL ) {
            delete [] data;
            }
        return NULL;
        }
    else {
        return data;
        }
    }




#elif defined(__mac__)

const char *arecordFileName = "inputSound.wav";

// mac implementation does nothing for now
char startRecording16BitMonoSound( int inSampleRate ) {
    return false;
    }

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    *outNumSamples = 0;
    return NULL;
    }

#elif defined(WIN_32)

#include <mmsystem.h>

const char *arecordFileName = "inputSound.wav";
static int arecordSampleRate = 0;

// windows implementation does nothing for now
char startRecording16BitMonoSound( int inSampleRate ) {

    arecordSampleRate = inSampleRate;

    if( mciSendString( "open new type waveaudio alias my_sound",
                       NULL, 0, 0 ) == 0 ) {

        char *settingsString =
            autoSprintf( "set my_sound alignment 2 bitspersample 16"
                         " samplespersec %d"
                         " channels 1"
                         " bytespersec %d"
                         " time format milliseconds format tag pcm",
                         inSampleRate,
                         ( 16 * inSampleRate ) / 8 );

        mciSendString( settingsString, NULL, 0, 0 );

        delete [] settingsString;

        mciSendString( "record my_sound", NULL, 0, 0 );
        return true;
        }

    return false;
    }

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    mciSendString( "stop my_sound", NULL, 0, 0 );

    char *saveCommand = autoSprintf( "save my_sound %s", arecordFileName );

    mciSendString( saveCommand, NULL, 0, 0 );
    delete [] saveCommand;

    mciSendString( "close my_sound", NULL, 0, 0 );

    int rate = -1;

    int16_t *data = load16BitMonoSound( outNumSamples, &rate );

    if( rate != arecordSampleRate ) {
        *outNumSamples = 0;

        if( data != NULL ) {
            delete [] data;
            }
        return NULL;
        }
    else {
        return data;
        }
    }

#else

const char *arecordFileName = "inputSound.wav";

// default implementation does nothing
char startRecording16BitMonoSound( int inSampleRate ) {
	return false;
}

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
	return NULL;
}

#endif


#ifdef LINUX

char isPrintingSupported() {
    int ret = system( "convert --version > /dev/null 2>&1" );
    if( ret == 0 ) {
        return true;
        }
    else {
        return false;
        }
    }


void printImage( Image *inImage, char inFullColor ) {
    const char *fileName = "printImage_temp.tga";

    writeTGAFile( fileName, inImage );

    const char *colorspaceFlag = "-colorspace gray";

    if( inFullColor ) {
        colorspaceFlag = "";
        }


    char *command =
        autoSprintf( "convert -density 72x72 "
                     " %s %s ps:- | lpr",
                     colorspaceFlag, fileName );


    system( command );

    delete [] command;

    // File file( NULL, fileName );
    // file.remove();
    }


#else

char isPrintingSupported() {
	return false;
}

void printImage( Image *inImage, char inFullColor ) {
}

#endif



