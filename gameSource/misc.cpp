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

void quitGame() {
	exit( 0 );
}




