//
// Created by olivier on 18/08/2022.
//

#include "sdl.h"

#include "../../game/ui/GameSceneHandler.h"

#include "../../FinalMessagePage.h"
#include "../../LoadingPage.h"
#include "../../AutoUpdatePage.h"
#include "../LivingLifePage.h"
#include "../../ExistingAccountPage.h"
#include "../../ExtendedMessagePage.h"
#include "../../RebirthChoicePage.h"
#include "../../SettingsPage.h"
#include "../../ReviewPage.h"
#include "../../TwinPage.h"
#include "../../PollPage.h"
#include "../../GeneticHistoryPage.h"
#include "../../ServerActionPage.h"

#include "../../categoryBank.h"
#include "../../groundSprites.h"
#include "../../soundBank.h"
#include "../../spriteBank.h"

#include "../../fitnessScore.h"
#include "../../musicPlayer.h"
#include "../../liveObjectSet.h"
#include "../../lifeTokens.h"
#include "../../photos.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"
#include "minorGems/graphics/openGL/glInclude.h"//gluProject
#include "minorGems/io/file/File.h"
#include "minorGems/sound/audioNoClip.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/SimpleVector.h"

#include <SDL/SDL.h>
#include <GL/gl.h>

char bufferSizeHinted = false;
unsigned int frameNumber = 0;
SDL_Cursor *ourCursor = NULL;
char soundOpen = false;

extern char *accountKey;
extern FILE *aiffOutFile;
extern float blendOutputFrameFraction;
extern char blendOutputFramePairs;
extern char *currentUserTypedMessage;
extern float currentSoundLoudness;
extern char frameDrawerInited;
extern SpriteHandle instructionsSprite;
extern unsigned char *lastFrame_rgbaBytes;
extern char manualScreenShot;
extern char outputAllFrames;
extern SimpleVector<double> playingSoundSpriteRates;
extern SimpleVector<SoundSprite> playingSoundSprites;
SimpleVector<double> playingSoundSpriteVolumesR( 100 );
SimpleVector<double> playingSoundSpriteVolumesL( 100 );
extern char recordAudio;
extern char *reflectorURL;
extern int samplesLeftToRecord;
extern ScreenGL *screen;
extern GameSceneHandler *sceneHandler;
extern int screenWidth;
extern int screenHeight;
extern Image **screenShotImageDest;
extern char *screenShotPrefix;
extern char *serverIP;
extern char *shutdownMessage;
extern float soundLoudness;
float soundLoudnessIncrementPerSample = 0.0f;
extern float soundSpriteFadeIncrementPerSample;
extern float soundSpriteGlobalLoudness;
extern double *soundSpriteMixingBufferR;
extern double *soundSpriteMixingBufferL;
extern NoClip soundSpriteNoClip;
extern SimpleVector<SoundSprite*> soundSprites;
extern char soundSpritesFading;
extern int targetFrameRate;
extern NoClip totalAudioMixNoClip;
extern double totalSoundSpriteNormalizeFactor;
extern char *userEmail;
extern char *userTwinCode;
extern float viewCenterX;
extern float viewCenterY;
extern float viewSize;
extern float visibleWidth;
extern float visibleHeight;
extern char *webProxy;
extern SimpleVector<WebRequestRecord> webRequestRecords;

extern FinalMessagePage *finalMessagePage;
extern ServerActionPage *getServerAddressPage;
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

extern Font *mainFont;
extern Font *mainFontFixed;
extern Font *mainFontReview;
extern Font *numbersFontFixed;
extern Font *handwritingFont;
extern Font *pencilFont;
extern Font *pencilErasedFont;
extern Font *smallFont;
extern Font *titleFont;

static int nextShotNumber = -1;
static int outputFrameCount = 0;
static char shotDirExists = false;

#ifdef USE_JPEG
#include "../../minorGems/graphics/converters/JPEGImageConverter.h"
    static JPEGImageConverter screenShotConverter( 90 );
    static const char *screenShotExtension = "jpg";
#elif defined(USE_PNG)
#include "../../minorGems/graphics/converters/PNGImageConverter.h"
    static PNGImageConverter screenShotConverter;
    static const char *screenShotExtension = "png";
#else
static TGAImageConverter screenShotConverter;
static const char *screenShotExtension = "tga";
#endif

/**
 *
 * @param inUserData
 * @param inStream
 * @param inLengthToFill
 * @note from minorGems/game/platforms/SDL/gameSDL.cpp
 */
void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill )
{
	getSoundSamples( inStream, inLengthToFill );

	int numSamples = inLengthToFill / 4;


	if( playingSoundSprites.size() > 0 ) {

		for( int i=0; i<numSamples; i++ ) {
			soundSpriteMixingBufferL[ i ] = 0.0;
			soundSpriteMixingBufferR[ i ] = 0.0;
		}

		for( int i=0; i<playingSoundSprites.size(); i++ ) {
			SoundSprite *s = playingSoundSprites.getElement( i );

			double rate = playingSoundSpriteRates.getElementDirect( i );
			double volumeL = playingSoundSpriteVolumesL.getElementDirect( i );
			double volumeR = playingSoundSpriteVolumesR.getElementDirect( i );

			int filled = 0;

			//int filledBytes = 0;

			if( rate == 1 ) {

				int samplesPlayed = s->samplesPlayed;
				int spriteNumSamples = s->numSamples;

				while( filled < numSamples &&
					   samplesPlayed < spriteNumSamples  ) {

					Sint16 sample = s->samples[ samplesPlayed ];

					soundSpriteMixingBufferL[ filled ]
							+= volumeL * sample;

					soundSpriteMixingBufferR[ filled ]
							+= volumeR * sample;

					filled ++;
					samplesPlayed ++;
				}
				s->samplesPlayed = samplesPlayed;
			}
			else {
				double samplesPlayedF = s->samplesPlayedF;
				int spriteNumSamples = s->numSamples;

				while( filled < numSamples &&
					   samplesPlayedF < spriteNumSamples - 1  ) {

					int aIndex = (int)floor( samplesPlayedF );
					int bIndex = (int)ceil( samplesPlayedF );

					Sint16 sampleA = s->samples[ aIndex ];
					Sint16 sampleB = s->samples[ bIndex ];

					double bWeight = samplesPlayedF - aIndex;
					double aWeight = 1 - bWeight;

					double sampleBlend = sampleA * aWeight + sampleB * bWeight;

					soundSpriteMixingBufferL[ filled ]
							+= volumeL * sampleBlend;

					soundSpriteMixingBufferR[ filled ]
							+= volumeR * sampleBlend;

					filled ++;
					samplesPlayedF += rate;
				}
				s->samplesPlayedF = samplesPlayedF;
			}
		}


		// respect their collective volume cap
		audioNoClip( &soundSpriteNoClip,
					 soundSpriteMixingBufferL, soundSpriteMixingBufferR,
					 numSamples );

		// and normalize to compensate for any compression below that cap
		if( totalSoundSpriteNormalizeFactor != 1.0 ) {
			for( int i=0; i<numSamples; i++ ) {
				soundSpriteMixingBufferL[i] *= totalSoundSpriteNormalizeFactor;
				soundSpriteMixingBufferR[i] *= totalSoundSpriteNormalizeFactor;
			}
		}


		// now mix them in
		int filledBytes = 0;


		// next, do final mix, then
		//  apply global no-clip, for mix of sound sprites and
		// music or other sounds created by getSoundSamples

		for( int i=0; i<numSamples; i++ ) {
			Sint16 lSample =
					(Sint16)( (inStream[filledBytes+1] << 8) |
							  inStream[filledBytes] );
			Sint16 rSample =
					(Sint16)( (inStream[filledBytes+3] << 8) |
							  inStream[filledBytes+2] );

			filledBytes += 4;

			// apply global loudness to sound sprites as part of this mx
			soundSpriteMixingBufferL[i] *= soundSpriteGlobalLoudness;
			soundSpriteMixingBufferR[i] *= soundSpriteGlobalLoudness;

			if( soundSpritesFading ) {
				soundSpriteGlobalLoudness -= soundSpriteFadeIncrementPerSample;

				if( soundSpriteGlobalLoudness < 0.0f ) {
					soundSpriteGlobalLoudness = 0.0f;
				}
			}


			soundSpriteMixingBufferL[i] += lSample;
			soundSpriteMixingBufferR[i] += rSample;
		}



		// we have our final mix, make sure it never clips
		audioNoClip( &totalAudioMixNoClip,
					 soundSpriteMixingBufferL, soundSpriteMixingBufferR,
					 numSamples );


		// now convert back to integers
		filledBytes = 0;
		for( int i=0; i<numSamples; i++ ) {
			Sint16 lSample = lrint( soundSpriteMixingBufferL[i] );
			Sint16 rSample = lrint( soundSpriteMixingBufferR[i] );


			inStream[filledBytes++] = (Uint8)( lSample & 0xFF );
			inStream[filledBytes++] =
					(Uint8)( ( lSample >> 8 ) & 0xFF );
			inStream[filledBytes++] = (Uint8)( rSample & 0xFF );
			inStream[filledBytes++] =
					(Uint8)( ( rSample >> 8 ) & 0xFF );
		}

		// walk backward, removing any that are done
		// OR remove all if sound sprites are completely faded out
		for( int i=playingSoundSprites.size()-1; i>=0; i-- ) {
			SoundSprite *s = playingSoundSprites.getElement( i );

			if( soundSpriteGlobalLoudness == 0 ||
				s->samplesPlayed >= s->numSamples ||
				s->samplesPlayedF >= s->numSamples - 1 ) {

				playingSoundSprites.deleteElement( i );
				playingSoundSpriteRates.deleteElement( i );
				playingSoundSpriteVolumesL.deleteElement( i );
				playingSoundSpriteVolumesR.deleteElement( i );
			}
		}
	}

	// now apply global loudness fade for pause
	if( ( currentSoundLoudness != soundLoudness && ! sceneHandler->mPaused )
		||
		( currentSoundLoudness != 0.0f && sceneHandler->mPaused )
		||
		currentSoundLoudness != 1.0f ) {


		int nextByte = 0;
		for( int i=0; i<numSamples; i++ ) {
			Sint16 lSample =
					inStream[nextByte] |
					( inStream[nextByte + 1] << 8 );

			Sint16 rSample =
					inStream[nextByte + 2] |
					( inStream[nextByte + 3] << 8 );

			lSample = (Sint16)( lSample * currentSoundLoudness );
			rSample = (Sint16)( rSample * currentSoundLoudness );


			inStream[nextByte++] = (Uint8)( lSample & 0xFF );
			inStream[nextByte++] = (Uint8)( ( lSample >> 8 ) & 0xFF );
			inStream[nextByte++] = (Uint8)( rSample & 0xFF );
			inStream[nextByte++] = (Uint8)( ( rSample >> 8 ) & 0xFF );

			if( currentSoundLoudness != soundLoudness &&
				! sceneHandler->mPaused ) {
				currentSoundLoudness += soundLoudnessIncrementPerSample;

				if( currentSoundLoudness > soundLoudness ) {
					currentSoundLoudness = soundLoudness;
				}
			}
			else if( currentSoundLoudness != 0 &&
					 sceneHandler->mPaused ) {

				currentSoundLoudness -= soundLoudnessIncrementPerSample;

				if( currentSoundLoudness < 0 ) {
					currentSoundLoudness = 0;
				}
			}

		}
	}


	if( recordAudio ) {


		if( numSamples > samplesLeftToRecord ) {
			numSamples = samplesLeftToRecord;
		}

		// reverse byte order
		int nextByte = 0;
		for( int i=0; i<numSamples; i++ ) {

			fwrite( &( inStream[ nextByte + 1 ] ), 1, 1, aiffOutFile );
			fwrite( &( inStream[ nextByte ] ), 1, 1, aiffOutFile );

			fwrite( &( inStream[ nextByte + 3 ] ), 1, 1, aiffOutFile );
			fwrite( &( inStream[ nextByte + 2 ] ), 1, 1, aiffOutFile );
			nextByte += 4;
		}


		samplesLeftToRecord -= numSamples;


		if( samplesLeftToRecord <= 0 ) {
			recordAudio = false;
			fclose( aiffOutFile );
			aiffOutFile = NULL;
		}
	}
}

/**
 * @note from minorGems/game/platforms/SDL/gameSDL.cpp
 * function that destroys object when exit is called.
 * exit is the only way to stop the loop in  ScreenGL
 */
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

/**
 *
 */
void freeDrawString()
{
	delete mainFont;
}

/**
 * @note minorGems/game/platforms/SDL/gameSDL.cpp
 */
void pauseGame()
{
	sceneHandler->mPaused = !( sceneHandler->mPaused );
}

/**
 *
 * @note from game.cpp
 * // called at application termination
// good time to save state for next launch
 */
void freeFrameDrawer()
{
	freeSprite( instructionsSprite );
	delete mainFontReview;
	delete mainFontFixed;
	delete numbersFontFixed;
	delete handwritingFont;
	delete pencilFont;
	delete pencilErasedFont;
	delete smallFont;
	if( currentUserTypedMessage != NULL )
	{
		delete [] currentUserTypedMessage;
		currentUserTypedMessage = NULL;
	}
	if( shutdownMessage != NULL )
	{
		delete [] shutdownMessage;
		shutdownMessage = NULL;
	}
	delete getServerAddressPage;
	delete finalMessagePage;
	delete loadingPage;
	delete autoUpdatePage;
	if( livingLifePage != NULL )
	{
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
	if( reflectorURL != NULL )
	{
		delete [] reflectorURL;
		reflectorURL = NULL;
	}

	if( serverIP != NULL )
	{
		delete [] serverIP;
		serverIP = NULL;
	}

	if( userEmail != NULL )
	{
		delete [] userEmail;
	}

	if( accountKey != NULL )
	{
		delete [] accountKey;
	}

	if( userTwinCode != NULL )
	{
		delete [] userTwinCode;
	}
}

/**
 *
 * @param inStartX
 * @param inStartY
 * @param inWidth
 * @param inHeight
 * @param inForceManual
 * @return
 * @note from minorGems/game/platforms/SDL/gameSDL.cpp
 * if manualScreenShot false, then any blend settings (for saving blended
 * double-frames) are applied can return NULL in this case (when this frame should not be output
 * according to blending settings)
 *  Region in screen pixels
 */
static Image *getScreenRegionInternal(
		int inStartX, int inStartY, int inWidth, int inHeight,
		char inForceManual = false )
{
	int numBytes = inWidth * inHeight * 3;
	unsigned char *rgbBytes = new unsigned char[ numBytes ];

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
		lastFrame_rgbaBytes != NULL )
	{
		// save blended frames on odd frames
		if( blendOutputFrameFraction > 0 )
		{
			float blendA = 1 - blendOutputFrameFraction;
			float blendB = blendOutputFrameFraction;
			for( int i=0; i<numBytes; i++ )
			{
				rgbBytes[i] = (unsigned char)(
						blendA * rgbBytes[i] +
						blendB * lastFrame_rgbaBytes[i] );
			}
		}
	}
	else if( ! inForceManual &&
			 ! manualScreenShot &&
			 blendOutputFramePairs &&
			 frameNumber % 2 == 0 )
	{
		// skip even frames, but save them for next blending
		if( lastFrame_rgbaBytes != NULL )
		{
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
	for( int y=inHeight - 1; y>=0; y-- )
	{
		for( int x=0; x<inWidth; x++ )
		{
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

/**
 *
 * @param inX
 * @param inY
 * @param inWidth
 * @param inHeight
 * @return
 * @note from minorGems/game/platforms/SDL/gameSDL.cpp
 */
Image *getScreenRegion( double inX, double inY,
						double inWidth, double inHeight )
{
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
	Image *result = getScreenRegionInternal(
			lrint( winStartX ), lrint( winStartY ),
			lrint( winEndX - winStartX ), lrint( winEndY - winStartY ) );
	manualScreenShot = oldManual;
	return result;
}

/**
 *
 * @param inStartX
 * @param inStartY
 * @param inWidth
 * @param inHeight
 * @return
 * @note from minorGems/game/platforms/SDL/gameSDL.cpp
 */
Image *getScreenRegionRaw(int inStartX, int inStartY, int inWidth, int inHeight )
{
	return getScreenRegionInternal( inStartX, inStartY, inWidth, inHeight, true );
}

void redoDrawMatrix()
{
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

void setViewCenterPosition( float inX, float inY )
{
	viewCenterX = inX;
	viewCenterY = inY;
	redoDrawMatrix();
}

void setViewSize( float inSize )
{
	viewSize = inSize;
	redoDrawMatrix();
}

void takeScreenShot()
{
	File shotDir( NULL, "screenShots" );

	if( !shotDirExists && !shotDir.exists() ) {
		shotDir.makeDirectory();
		shotDirExists = shotDir.exists();
	}

	if( nextShotNumber < 1 ) {
		if( shotDir.exists() && shotDir.isDirectory() )
		{
			int numFiles;
			File **childFiles = shotDir.getChildFiles( &numFiles );
			nextShotNumber = 1;
			char *formatString = autoSprintf( "%s%%d.%s", screenShotPrefix,
											  screenShotExtension );

			for( int i=0; i<numFiles; i++ )
			{
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

	if( nextShotNumber < 1 )
	{
		return;
	}

	char *fileName = autoSprintf( "%s%05d.%s",
								  screenShotPrefix, nextShotNumber,
								  screenShotExtension );

	File *file = shotDir.getChildFile( fileName );
	delete [] fileName;

	if( outputAllFrames )
	{
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
