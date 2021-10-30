//
// Created by olivier on 23/10/2021.
//
/*
 * Modification History
 *
 * 2010-September-3  Jason Rohrer
 * Fixed mouse to world translation function.
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <SDL/SDL_main.h>// let SDL override our main function with SDLMain
int mainFunction( int inArgCount, char **inArgs );// must do this before SDL include to prevent WinMain linker errors on win32

int main( int inArgCount, char **inArgs )
{
	return mainFunction( inArgCount, inArgs );
}


#include <SDL/SDL.h>
#include "minorGems/graphics/openGL/ScreenGL.h"
#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/Color.h"
#include "minorGems/graphics/openGL/gui/GUIComponentGL.h"
#include "minorGems/network/web/WebRequest.h"
#include "minorGems/graphics/openGL/glInclude.h"
#include "minorGems/graphics/openGL/SceneHandlerGL.h"
#include "minorGems/graphics/openGL/MouseHandlerGL.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"
#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/system/Time.h"
#include "minorGems/system/Thread.h"
#include "minorGems/io/file/File.h"
#include "minorGems/network/HostAddress.h"
#include "minorGems/network/Socket.h"
#include "minorGems/network/SocketClient.h"
#include "minorGems/network/upnp/portMapping.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/TranslationManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/log/FileLog.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"
#include "minorGems/io/file/FileInputStream.h"
#include "minorGems/util/ByteBufferInputStream.h"
#include "minorGems/sound/formats/aiff.h"
#include "minorGems/sound/audioNoClip.h"
#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"
#include "minorGems/util/random/CustomRandomSource.h"
#include "OneLife/gameSource/components/gameSceneHandler.h"
#include "OneLife/gameSource/dataTypes/web.h"
#include "OneLife/gameSource/dataTypes/sound.h"
#include "OneLife/gameSource/application.h"

extern char rCtrlDown;
extern char lCtrlDown;
extern char rAltDown;
extern char lAltDown;
extern char rMetaDown;
extern char lMetaDown;
extern char lShiftDown;
extern char rShiftDown;

// static seed
CustomRandomSource randSource( 34957197 );


#ifdef RASPBIAN

#include "minorGems/graphics/openGL/RaspbianGLSurface.cpp"

// demo code panel uses non-GLES code
void showDemoCodePanel( ScreenGL *inScreen, const char *inFontTGAFileName,
                        int inWidth, int inHeight ) {
    }
char isDemoCodePanelShowing() {
    return false;
    }
void freeDemoCodePanel() {
    }
void showWriteFailedPanel( ScreenGL *inScreen, const char *inFontTGAFileName,
                           int inWidth, int inHeight ) {
    }
void freeWriteFailedPanel() {
    }


#else

//#include "minorGems/game/platforms/SDL/demoCodePanel.h"//legacy
#include "OneLife/gameSource/components/pages/demoPanelPage.h"

#endif

#include "OneLife/gameSource/components/asyncFileThread.h"



extern float visibleWidth;
extern float visibleHeight;
// some settings
int cursorMode = 0;
double emulatedCursorScale = 1.0;
// size of game image
int gameWidth = 320;
int gameHeight = 240;

// size of screen for fullscreen mode
extern int screenWidth;
extern int screenHeight;
extern char outputAllFrames;

int idealTargetFrameRate = 60;
int targetFrameRate = idealTargetFrameRate;
SimpleVector<int> possibleFrameRates;
char countingOnVsync = false;
int soundSampleRate = 22050;
//int soundSampleRate = 44100;
char soundRunning = false;
char soundOpen = false;
char hardToQuitMode = false;
int pauseOnMinimize = 1;
char demoMode = false;
char writeFailed = false;

char *loadingFailedMessage = NULL;
char loadingDone = false;
// ^ and & keys to slow down and speed up for testing
// read from settings folder
char enableSpeedControlKeys = false;
// should output only every other frame, and blend in dropped frames?
char blendOutputFramePairs = false;
float blendOutputFrameFraction = 0;
char *webProxy = NULL;
unsigned char *lastFrame_rgbaBytes = NULL;
char recordAudio = false;
FILE *aiffOutFile = NULL;
static int samplesLeftToRecord = 0;
char bufferSizeHinted = false;
char measureFrameRate = true;
char shouldTakeScreenshot = false;// should screenshot be taken at end of next redraw?
char manualScreenShot = false;
extern char *screenShotPrefix;
GameSceneHandler *sceneHandler;


// how many pixels wide is each game pixel drawn as?
int pixelZoomFactor;
SimpleVector<WebRequestRecord> webRequestRecords;
SimpleVector<SocketConnectionRecord> socketConnectionRecords;
// don't worry about dynamic mallocs here
// because we don't need to lock audio thread before adding values
// (playing sound sprites is handled directly through SoundSprite* handles
//  without accessing this vector).
SimpleVector<SoundSprite*> soundSprites;
// audio thread is locked every time we touch this vector
// So, we want to avoid mallocs here
// Can we imagine more than 100 sound sprites ever playing at the same time?
SimpleVector<SoundSprite> playingSoundSprites( 100 );
double *soundSpriteMixingBufferL = NULL;
double *soundSpriteMixingBufferR = NULL;
// variable rate per sprite
// these are also touched with audio thread locked, so avoid mallocs
// we won't see more than 100 of these simultaneously either
SimpleVector<double> playingSoundSpriteRates( 100 );
SimpleVector<double> playingSoundSpriteVolumesR( 100 );
SimpleVector<double> playingSoundSpriteVolumesL( 100 );
SDL_Cursor *ourCursor = NULL;
static int nextSoundSpriteHandle;
static double soundSpriteRateMax = 1.0;
static double soundSpriteRateMin = 1.0;

OneLife::game::Application *screen;


void setSoundSpriteRateRange( double inMin, double inMax ) {
	soundSpriteRateMin = inMin;
	soundSpriteRateMax = inMax;
}

static double soundSpriteVolumeMax = 1.0;
static double soundSpriteVolumeMin = 1.0;


void setSoundSpriteVolumeRange( double inMin, double inMax ) {
	soundSpriteVolumeMin = inMin;
	soundSpriteVolumeMax = inMax;
}


static float soundLoudness = 1.0f;

static float currentSoundLoudness = 0.0f;

static float soundLoudnessIncrementPerSample = 0.0f;

static float soundSpriteGlobalLoudness = 1.0f;


static char soundSpritesFading = false;
static float soundSpriteFadeIncrementPerSample = 0.0f;


void setSoundLoudness( float inLoudness ) {
	lockAudio();
	soundLoudness = inLoudness;
	currentSoundLoudness = inLoudness;
	unlockAudio();
}


void fadeSoundSprites( double inFadeSeconds ) {
	lockAudio();
	soundSpritesFading = true;

	soundSpriteFadeIncrementPerSample =
			1.0f / ( inFadeSeconds * soundSampleRate );

	unlockAudio();
}



void resumePlayingSoundSprites() {
	lockAudio();
	soundSpritesFading = false;
	soundSpriteGlobalLoudness = 1.0f;
	unlockAudio();
}





SoundSpriteHandle loadSoundSprite( const char *inAIFFFileName ) {
	return loadSoundSprite( "sounds", inAIFFFileName );
}


SoundSpriteHandle loadSoundSprite( const char *inFolderName,
								   const char *inAIFFFileName ) {

	File aiffFile( new Path( inFolderName ), inAIFFFileName );

	if( ! aiffFile.exists() ) {
		printf( "File does not exist in sounds folder: %s\n",
				inAIFFFileName );
		return NULL;
	}

	int numBytes;
	unsigned char *data = aiffFile.readFileContents( &numBytes );


	if( data == NULL ) {
		printf( "Failed to read sound file: %s\n", inAIFFFileName );
		return NULL;
	}


	int numSamples;
	int16_t *samples = readMono16AIFFData( data, numBytes, &numSamples );

	delete [] data;

	if( samples == NULL ) {
		printf( "Failed to parse AIFF sound file: %s\n", inAIFFFileName );
		return NULL;
	}

	SoundSpriteHandle s = setSoundSprite( samples, numSamples );

	delete [] samples;

	return s;
}



SoundSpriteHandle setSoundSprite( int16_t *inSamples, int inNumSamples ) {
	SoundSprite *s = new SoundSprite;

	s->handle = nextSoundSpriteHandle ++;
	s->numSamples = inNumSamples;

	s->noVariance = false;

	s->samplesPlayed = 0;
	s->samplesPlayedF = 0;

	s->samples = new Sint16[ s->numSamples ];

	memcpy( s->samples, inSamples, inNumSamples * sizeof( int16_t ) );

	soundSprites.push_back( s );

	return (SoundSpriteHandle)s;
}



void toggleVariance( SoundSpriteHandle inHandle, char inNoVariance ) {
	SoundSprite *s = (SoundSprite*)inHandle;
	s->noVariance = inNoVariance;
}




static double maxTotalSoundSpriteVolume = 1.0;

static double soundSpriteCompressionFraction = 0.0;

static double totalSoundSpriteNormalizeFactor = 1.0;

static NoClip soundSpriteNoClip;

static NoClip totalAudioMixNoClip;


void setMaxTotalSoundSpriteVolume( double inMaxTotal,
								   double inCompressionFraction ) {
	lockAudio();

	maxTotalSoundSpriteVolume = inMaxTotal;
	soundSpriteCompressionFraction = inCompressionFraction;

	totalSoundSpriteNormalizeFactor =
			1.0 / ( 1.0 - soundSpriteCompressionFraction );

	soundSpriteNoClip =
			resetAudioNoClip( ( 1.0 - soundSpriteCompressionFraction ) *
							  maxTotalSoundSpriteVolume * 32767,
					// half second hold and release
							  soundSampleRate / 2, soundSampleRate / 2 );

	unlockAudio();
}


static int maxSimultaneousSoundSprites = -1;


void setMaxSimultaneousSoundSprites( int inMaxCount ) {
	maxSimultaneousSoundSprites = inMaxCount;
}




static double pickRandomRate() {
	if( soundSpriteRateMax != 1.0 ||
		soundSpriteRateMin != 1.0 ) {

		return randSource.getRandomBoundedDouble( soundSpriteRateMin,
												  soundSpriteRateMax );
	}
	else {
		return 1.0;
	}
}



static double pickRandomVolume() {
	if( soundSpriteVolumeMax != 1.0 ||
		soundSpriteVolumeMin != 1.0 ) {

		return randSource.getRandomBoundedDouble( soundSpriteVolumeMin,
												  soundSpriteVolumeMax );
	}
	else {
		return 1.0;
	}
}




// no locking
static void playSoundSpriteInternal(
		SoundSpriteHandle inHandle, double inVolumeTweak,
		double inStereoPosition,
		double inForceVolume = -1,
		double inForceRate = -1 ) {


	if( soundSpritesFading && soundSpriteGlobalLoudness == 0.0f ) {
		// don't play any new sound sprites
		return;
	}

	if( maxSimultaneousSoundSprites != -1 &&
		playingSoundSpriteVolumesR.size() >= maxSimultaneousSoundSprites ) {
		// cap would be exceeded
		// don't play this sound sprite at all
		return;
	}


	double volume = inVolumeTweak;

	SoundSprite *s = (SoundSprite*)inHandle;

	if( ! s->noVariance ) {

		if( inForceVolume == -1 ) {
			volume *= pickRandomVolume();
		}
		else {
			volume *= inForceVolume;
		}
	}






	// constant power rule
	double p = M_PI * inStereoPosition * 0.5;

	double rightVolume = volume * sin( p );
	double leftVolume = volume * cos( p );


	s->samplesPlayed = 0;
	s->samplesPlayedF = 0;




	playingSoundSprites.push_back( *s );

	if( s->noVariance ) {
		playingSoundSpriteRates.push_back( 1.0 );
	}
	else {

		if( inForceRate != -1 ) {
			playingSoundSpriteRates.push_back( inForceRate );
		}
		else {
			playingSoundSpriteRates.push_back(  pickRandomRate() );
		}
	}



	playingSoundSpriteVolumesR.push_back( rightVolume );
	playingSoundSpriteVolumesL.push_back( leftVolume );
}


// locking
void playSoundSprite( SoundSpriteHandle inHandle, double inVolumeTweak,
					  double inStereoPosition ) {

	lockAudio();
	playSoundSpriteInternal( inHandle, inVolumeTweak, inStereoPosition );
	unlockAudio();
}



// multiple with single lock
void playSoundSprite( int inNumSprites, SoundSpriteHandle *inHandles,
					  double *inVolumeTweaks,
					  double *inStereoPositions ) {
	lockAudio();

	// one random volume and rate for whole batch
	double volume = pickRandomVolume();
	double rate = pickRandomRate();

	for( int i=0; i<inNumSprites; i++ ) {
		playSoundSpriteInternal( inHandles[i], inVolumeTweaks[i],
								 inStereoPositions[i], volume, rate );
	}
	unlockAudio();
}





void freeSoundSprite( SoundSpriteHandle inHandle ) {
	// make sure this sprite isn't playing
	lockAudio();

	SoundSprite *s = (SoundSprite*)inHandle;

	// find it in vector to remove it
	for( int i=playingSoundSprites.size()-1; i>=0; i-- ) {
		SoundSprite *s2 = playingSoundSprites.getElement( i );
		if( s2->handle == s->handle ) {
			// stop it abruptly
			playingSoundSprites.deleteElement( i );
			playingSoundSpriteRates.deleteElement( i );
			playingSoundSpriteVolumesL.deleteElement( i );
			playingSoundSpriteVolumesR.deleteElement( i );
		}
	}

	unlockAudio();


	for( int i=0; i<soundSprites.size(); i++ ) {
		SoundSprite *s2 = soundSprites.getElementDirect( i );
		if( s2->handle == s->handle ) {
			delete [] s2->samples;
			soundSprites.deleteElement( i );
			delete s2;
		}
	}
}




void audioCallback( void *inUserData, Uint8 *inStream, int inLengthToFill ) {
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


int getSampleRate() {
	return soundSampleRate;
}


void setSoundPlaying( char inPlaying ) {
	SDL_PauseAudio( !inPlaying );
}



void lockAudio() {
	SDL_LockAudio();
}



void unlockAudio() {
	SDL_UnlockAudio();
}



char isSoundRunning() {
	return soundRunning;
}



#ifdef __mac__

#include <unistd.h>
#include <stdarg.h>


// returns path to folder
static char *pickFolder( const char *inPrompt ) {

    const char *commandFormat =
        "osascript -e 'tell application \"Finder\" to activate' "
        "-e 'tell app \"Finder\" to return POSIX path of "
        "(choose folder with prompt \"%s\")'";

    char *command = autoSprintf( commandFormat, inPrompt );

    printf( "Running osascript command:\n\n%s\n\n", command );

    FILE *osascriptPipe = popen( command, "r" );

    delete [] command;

    if( osascriptPipe == NULL ) {
        AppLog::error(
            "Failed to open pipe to osascript for picking a folder." );
        }
    else {
        char buffer[200];

        int numRead = fscanf( osascriptPipe, "%199s", buffer );

        if( numRead == 1 ) {
            return stringDuplicate( buffer );
            }

        pclose( osascriptPipe );
        }
    return NULL;
    }



static void showMessage( const char *inAppName,
                         const char *inTitle, const char *inMessage,
                         char inError = false ) {
    const char *iconName = "note";
    if( inError ) {
        // stop icon is broken in OS 10.12
        // always use note
        // iconName = "stop";
        }

    const char *commandFormat =
        "osascript -e 'tell app \"Finder\" to activate' "
        "-e 'tell app \"Finder\" to display dialog \"%s\" "
        "with title \"%s:  %s\" buttons \"Ok\" "
        "with icon %s default button \"Ok\"' ";

    char *command = autoSprintf( commandFormat, inMessage, inAppName, inTitle,
                                 iconName );

    printf( "Running osascript command:\n\n%s\n\n", command );

    FILE *osascriptPipe = popen( command, "r" );

    delete [] command;

    if( osascriptPipe == NULL ) {
        AppLog::error(
            "Failed to open pipe to osascript for displaying GUI messages." );
        }
    else {
        pclose( osascriptPipe );
        }
    }


static char *getPrefFilePath() {
    return autoSprintf( "%s/Library/Preferences/%s_prefs.txt",
                        getenv( "HOME" ),
                        getAppName() );
    }


static char isSettingsFolderFound() {

    File settingsFolder( NULL, "settings" );

    if( settingsFolder.exists() && settingsFolder.isDirectory() ) {
        return true;
        }

    return false;
    }


#endif



#ifdef WIN32

#include <windows.h>
#include <tchar.h>

#endif



int mainFunction( int inNumArgs, char **inArgs ) {

#ifdef WIN32


	HMODULE hShcore = LoadLibrary( _T( "shcore.dll" ) );

    if( hShcore != NULL ) {
        printf( "shcore.dll loaded successfully\n" );

        typedef enum _PROCESS_DPI_AWARENESS {
            PROCESS_DPI_UNAWARE            = 0,
            PROCESS_SYSTEM_DPI_AWARE       = 1,
            PROCESS_PER_MONITOR_DPI_AWARE  = 2
            } PROCESS_DPI_AWARENESS;

        typedef HRESULT (*SetProcessDpiAwarenessFunc)( PROCESS_DPI_AWARENESS );

        SetProcessDpiAwarenessFunc setAwareness =
            (SetProcessDpiAwarenessFunc)GetProcAddress(
                hShcore,
                "SetProcessDpiAwareness" );

        if( setAwareness ) {
            printf( "Found SetProcessDpiAwareness function in shcore.dll\n" );
            setAwareness( PROCESS_PER_MONITOR_DPI_AWARE );
            }
        else {
            printf( "Could NOT find SetProcessDpiAwareness function in "
                    "Shcore.dll\n" );
            }
        FreeLibrary( hShcore );
        }
    else {
        printf( "shcore.dll NOT loaded successfully... pre Win 8.1 ?\n" );


        printf( "   Trying to load user32.dll instead\n" );


        // backwards compatible code
        // on vista and higher, this will tell Windows that we are DPI aware
        // and that we should not be artificially scaled.
        //
        // Found here:  http://www.rw-designer.com/DPI-aware
        HMODULE hUser32 = LoadLibrary( _T( "user32.dll" ) );

        if( hUser32 != NULL ) {

            typedef BOOL (*SetProcessDPIAwareFunc)();

            SetProcessDPIAwareFunc setDPIAware =
                (SetProcessDPIAwareFunc)GetProcAddress( hUser32,
                                                        "SetProcessDPIAware" );
            if( setDPIAware ) {
                printf( "Found SetProcessDPIAware function in user32.dll\n" );
                setDPIAware();
                }
            else {
                printf( "Could NOT find SetProcessDPIAware function in "
                        "user32.dll\n" );
                }
            FreeLibrary( hUser32 );
            }
        else {
            printf( "Failed to load user32.dll\n" );
            }
        }
#endif


	// check result below, after opening log, so we can log failure
	Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
	if( getUsesSound() ) {
		flags |= SDL_INIT_AUDIO;
	}

	int sdlResult = SDL_Init( flags );


	// do this mac check after initing SDL,
	// since it causes various Mac frameworks to be loaded (which can
	// change the current working directory out from under us)
#ifdef __mac__
	// make sure working directory is the same as the directory
        // that the app resides in
        // this is especially important on the mac platform, which
        // doesn't set a proper working directory for double-clicked
        // app bundles

        // arg 0 is the path to the app executable
        char *appDirectoryPath = stringDuplicate( inArgs[0] );

        char *bundleName = autoSprintf( "%s_%d.app",
                                        getAppName(), getAppVersion() );

        char *appNamePointer = strstr( appDirectoryPath, bundleName );

        if( appNamePointer != NULL ) {
            // terminate full app path to get parent directory
            appNamePointer[0] = '\0';

            chdir( appDirectoryPath );
            }


        if( ! isSettingsFolderFound() ) {
            // first, try setting dir based on preferences file
            char *prefFilePath = getPrefFilePath();

            FILE *prefFile = fopen( prefFilePath, "r" );

            if( prefFile != NULL ) {

                char path[200];

                int numRead = fscanf( prefFile, "%199s", path );

                if( numRead == 1 ) {
                    chdir( path );
                    }
                fclose( prefFile );
                }

            delete [] prefFilePath;
            }


        if( ! isSettingsFolderFound() ) {

            showMessage( getAppName(), "First Start Up Error",
                         "Cannot find game data.\n\n"
                         "Newer versions of MacOS have strict sandboxing, "
                         "so we have to work around this issue by asking "
                         "you a question.\n\n"
                         //"There will also be some info presented along the "
                         //"way for debugging purposes.\n\n"
                         "Hopefully, you will only have to do this once.",
                         true );


            //showMessage( getAppName(), "Debug Info, Executable path =",
            //             inArgs[0], false );


            //showMessage( getAppName(), "Debug Info, Home dir =",
            //             getenv( "HOME" ), false );


            showMessage( getAppName(), "First Start Up",
                         "Please locate the game folder "
                         "in the next dialog box.",
                         false );

            char *prompt = autoSprintf(
                "Find the game folder (where the %s App is located):",
                getAppName() );

            char *path = pickFolder( prompt );

            delete [] prompt;

            if( path != NULL ) {
                //showMessage( getAppName(), "Debug Info, Chosen path =",
                //             path, false );


                char *prefFilePath = getPrefFilePath();

                FILE *prefFile = fopen( prefFilePath, "w" );

                if( prefFilePath == NULL ) {
                    char *message =
                        autoSprintf( "Failed to open this preferences file "
                                     "for writing:\n\n%s\n\n"
                                     "You will have to find the game folder "
                                     "again at next startup.",
                                     prefFilePath );

                    showMessage( getAppName(), "First Start Up Error",
                                 message,
                                 true );
                    delete [] message;
                    }
                else {
                    fprintf( prefFile, path );
                    fclose( prefFile );
                    }

                delete [] prefFilePath;


                chdir( path );

                delete [] path;

                if( !isSettingsFolderFound() ) {
                    showMessage( getAppName(), "First Start Up Error",
                                 "Still cannot find game data, exiting.",
                                 true );
                    return 1;
                    }
                }
            else {
                showMessage( getAppName(), "First Start Up Error",
                             "Picking a folder failed, exiting.",
                             true );
                return 1;
                }

            }


        delete [] bundleName;
        delete [] appDirectoryPath;
#endif



	AppLog::setLog( new FileLog( "log.txt" ) );
	AppLog::setLoggingLevel( Log::DETAIL_LEVEL );

	AppLog::info( "New game starting up" );


	if( sdlResult < 0 ) {
		AppLog::getLog()->logPrintf(
				Log::CRITICAL_ERROR_LEVEL,
				"Couldn't initialize SDL: %s\n", SDL_GetError() );
		return 0;
	}


	if( doesOverrideGameImageSize() ) {
		getGameImageSize( &gameWidth, &gameHeight );
	}

	AppLog::getLog()->logPrintf(
			Log::INFO_LEVEL,
			"Target game image size:  %dx%d\n",
			gameWidth, gameHeight );


	// read screen size from settings
	char widthFound = false;
	int readWidth = SettingsManager::getIntSetting( "screenWidth",
													&widthFound );
	char heightFound = false;
	int readHeight = SettingsManager::getIntSetting( "screenHeight",
													 &heightFound );

	if( widthFound && heightFound ) {
		// override hard-coded defaults
		screenWidth = readWidth;
		screenHeight = readHeight;
	}

	AppLog::getLog()->logPrintf(
			Log::INFO_LEVEL,
			"Ideal window dimensions:  %dx%d\n",
			screenWidth, screenHeight );


	if( ! isNonIntegerScalingAllowed() &&
		screenWidth < gameWidth ) {

		AppLog::info(
				"Screen width smaller than target game width, fixing" );
		screenWidth = gameWidth;
	}
	if( ! isNonIntegerScalingAllowed() &&
		screenHeight < gameHeight ) {
		AppLog::info(
				"Screen height smaller than target game height, fixing" );
		screenHeight = gameHeight;
	}


	if( isNonIntegerScalingAllowed() ) {

		double screenRatio = (double)screenWidth / (double)screenHeight;
		double gameRatio = (double)gameWidth / (double)gameHeight;

		if( screenRatio > gameRatio ) {
			// screen too wide

			// tell game about this by making game image wider than requested}

			AppLog::info(
					"Screen has wider aspect ratio than desired game image, "
					"fixing by makign game image wider" );

			gameWidth = (int)( screenRatio * gameHeight );

			// if screen too narrow, this is already handled elsewhere
		}
	}


	char fullscreenFound = false;
	int readFullscreen = SettingsManager::getIntSetting( "fullscreen",
														 &fullscreenFound );

	char fullscreen = true;

	if( fullscreenFound && readFullscreen == 0 ) {
		fullscreen = false;
	}

	if( fullscreen ) {
		AppLog::info( "Trying to start in fullscreen mode." );
	}
	else {
		AppLog::info( "Trying to start in window mode." );
	}


	char useLargestWindowFound = false;
	int readUseLargestWindow =
			SettingsManager::getIntSetting( "useLargestWindow",
											&useLargestWindowFound );

	char useLargestWindow = true;

	if( useLargestWindowFound && readUseLargestWindow == 0 ) {
		useLargestWindow = false;
	}


	if( !fullscreen && useLargestWindow ) {
		AppLog::info( "Want to use largest window that fits on screen." );

		const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

		int currentW = currentScreenInfo->current_w;
		int currentH = currentScreenInfo->current_h;

		if( isNonIntegerScalingAllowed() ) {
			double aspectRatio = screenHeight / (double) screenWidth;

			int tryW = currentW;
			int tryH = lrint( aspectRatio * currentW );

			// never fill more than 85% of screen vertically, because
			// this large of a window is a pain to manage
			if( tryH >= 0.85 * currentH ) {
				tryH = lrint( 0.84 * currentH );
				tryW = lrint( tryH / aspectRatio );
			}
			if( tryW < screenWidth ) {
				// largest window is smaller than requested screen size,
				// that's okay.
				screenWidth = tryW;
				screenHeight = tryH;
			}
			else if( tryW > screenWidth ) {
				// we're attempting a blow-up
				// but this is not worth it if the blow-up is too small
				// we loose pixel accuracy without giving the user a much
				// larger image.
				// make sure it's at least 25% wider to be worth it,
				// otherwise, keep the requested window size
				if( tryW >= 1.25 * screenWidth ) {
					screenWidth = tryW;
					screenHeight = tryH;
				}
				else {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Largest-window mode would offer a <25% width "
							"increase, using requested window size instead." );
				}
			}
		}
		else {

			int blowUpFactor = 1;

			while( gameWidth * blowUpFactor < currentW &&
				   gameHeight * blowUpFactor < currentH ) {

				blowUpFactor++;
			}

			while( blowUpFactor > 1 &&
				   ( gameWidth * blowUpFactor >= currentW * 0.85 ||
					 gameHeight * blowUpFactor >= currentH * 0.85 ) ) {

				// scale back, because we don't want to totally
				// fill the screen (annoying to manage such a big window)

				// stop at a window that fills < 85% of screen in
				// either direction
				blowUpFactor --;
			}

			screenWidth = blowUpFactor * gameWidth;
			screenHeight = blowUpFactor * gameHeight;
		}


		AppLog::getLog()->logPrintf(
				Log::INFO_LEVEL,
				"Screen dimensions for largest-window mode:  %dx%d\n",
				screenWidth, screenHeight );
	}
	else if( !fullscreen && !useLargestWindow ) {
		// make sure window isn't too big for screen

		const SDL_VideoInfo* currentScreenInfo = SDL_GetVideoInfo();

		int currentW = currentScreenInfo->current_w;
		int currentH = currentScreenInfo->current_h;

		if( isNonIntegerScalingAllowed() &&
			( screenWidth > currentW || screenHeight > currentH ) ) {
			double aspectRatio = screenHeight / (double) screenWidth;

			// make window as wide as screen, preserving game aspect ratio
			screenWidth = currentW;

			int testScreenHeight = lrint( aspectRatio * screenWidth );

			if( testScreenHeight <= currentH ) {
				screenHeight = testScreenHeight;
			}
			else {
				screenHeight = currentH;

				screenWidth = lrint( screenHeight / aspectRatio );
			}
		}
		else {


			int blowDownFactor = 1;

			while( screenWidth / blowDownFactor > currentW
				   ||
				   screenHeight / blowDownFactor > currentH ) {
				blowDownFactor += 1;
			}

			if( blowDownFactor > 1 ) {
				screenWidth /= blowDownFactor;
				screenHeight /= blowDownFactor;
			}
		}
	}





	char frameRateFound = false;
	int readFrameRate = SettingsManager::getIntSetting( "halfFrameRate",
														&frameRateFound );

	if( frameRateFound && readFrameRate >= 1 ) {
		// cut frame rate in half N times
		targetFrameRate /= (int)pow( 2, readFrameRate );
	}

	// can't draw less than 1 frame per second
	if( targetFrameRate < 1 ) {
		targetFrameRate = 1;
	}

	SimpleVector<char*> *possibleFrameRatesSetting =
			SettingsManager::getSetting( "possibleFrameRates" );

	for( int i=0; i<possibleFrameRatesSetting->size(); i++ ) {
		char *f = possibleFrameRatesSetting->getElementDirect( i );

		int v;

		int numRead = sscanf( f, "%d", &v );

		if( numRead == 1 ) {
			possibleFrameRates.push_back( v );
		}

		delete [] f;
	}

	delete possibleFrameRatesSetting;


	if( possibleFrameRates.size() == 0 ) {
		possibleFrameRates.push_back( 60 );
		possibleFrameRates.push_back( 120 );
		possibleFrameRates.push_back( 144 );
	}

	AppLog::info( "The following frame rates are considered possible:" );

	for( int i=0; i<possibleFrameRates.size(); i++ ) {
		AppLog::infoF( "%d fps", possibleFrameRates.getElementDirect( i ) );
	}



	char recordFound = false;
	int readRecordFlag = SettingsManager::getIntSetting( "recordGame",
														 &recordFound );

	char recordGame = false;

	if( recordFound && readRecordFlag == 1 ) {
		recordGame = true;
	}


	int speedControlKeysFlag =
			SettingsManager::getIntSetting( "enableSpeedControlKeys", 0 );

	if( speedControlKeysFlag == 1 ) {
		enableSpeedControlKeys = true;
	}



	int outputAllFramesFlag =
			SettingsManager::getIntSetting( "outputAllFrames", 0 );

	if( outputAllFramesFlag == 1 ) {
		outputAllFrames = true;
		// start with very first frame
		shouldTakeScreenshot = true;

		screenShotPrefix = stringDuplicate( "frame" );
	}

	int blendOutputFramePairsFlag =
			SettingsManager::getIntSetting( "blendOutputFramePairs", 0 );

	if( blendOutputFramePairsFlag == 1 ) {
		blendOutputFramePairs = true;
	}

	blendOutputFrameFraction =
			SettingsManager::getFloatSetting( "blendOutputFrameFraction", 0.0f );

	webProxy = SettingsManager::getStringSetting( "webProxy" );

	if( webProxy != NULL &&
		strcmp( webProxy, "" ) == 0 ) {

		delete [] webProxy;
		webProxy = NULL;
	}


	// make sure dir is writeable
	FILE *testFile = fopen( "testWrite.txt", "w" );

	if( testFile == NULL ) {
		writeFailed = true;
	}
	else {
		fclose( testFile );

		remove( "testWrite.txt" );

		writeFailed = false;
	}


	// don't try to record games if we can't write to dir
	// can cause a crash.
	if( writeFailed ) {
		recordGame = false;
	}






	char *customData = getCustomRecordedGameData();

	char *hashSalt = getHashSalt();

	screen = new OneLife::game::Application( screenWidth, screenHeight, fullscreen,
						  shouldNativeScreenResolutionBeUsed(),
						  targetFrameRate,
						  recordGame,
						  customData,
						  hashSalt,
						  getWindowTitle(), NULL, NULL, NULL );

	delete [] customData;
	delete [] hashSalt;


	// may change if specified resolution is not supported
	// or for event playback mode
	screenWidth = screen->getWidth();
	screenHeight = screen->getHeight();
	targetFrameRate = screen->getMaxFramerate();




	// watch out for huge resolutions that make default SDL cursor
	// too small

	int forceBigPointer = SettingsManager::getIntSetting( "forceBigPointer",
														  0 );
	if( forceBigPointer ||
		screenWidth > 1920 || screenHeight > 1080 ) {
		// big cursor

		AppLog::info( "Trying to load pointer from graphics/bigPointer.tga" );


		Image *cursorImage = readTGAFile( "bigPointer.tga" );

		if( cursorImage != NULL ) {

			if( cursorImage->getWidth() == 32 &&
				cursorImage->getHeight() == 32 &&
				cursorImage->getNumChannels() == 4 ) {

				double *r = cursorImage->getChannel( 0 );
				double *a = cursorImage->getChannel( 3 );


				Uint8 data[4*32];
				Uint8 mask[4*32];
				int i = -1;

				for( int y=0; y<32; y++ ) {
					for( int x=0; x<32; x++ ) {
						int p = y * 32 + x;

						if ( x % 8 ) {
							data[i] <<= 1;
							mask[i] <<= 1;
						}
						else {
							i++;
							data[i] = mask[i] = 0;
						}

						if( a[p] == 1 ) {
							if( r[p] == 0 ) {
								data[i] |= 0x01;
							}
							mask[i] |= 0x01;
						}
					}
				}

				// hot in upper left corner, (0,0)
				ourCursor =
						SDL_CreateCursor( data, mask, 32, 32, 0, 0 );

				SDL_SetCursor( ourCursor );
			}
			else {
				AppLog::error(
						"bigPointer.tga is not a 32x32 4-channel image." );

			}

			delete cursorImage;
		}
		else {

			AppLog::error( "Failed to read bigPointer.tga" );
		}
	}













	// adjust gameWidth to match available screen space
	// keep gameHeight constant


	/*
	SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY,
						 SDL_DEFAULT_REPEAT_INTERVAL );
	*/

	// never cut off top/bottom of image, and always try to use largest
	// whole multiples of screen pixels per game pixel to fill
	// the screen vertically as well as we can
	pixelZoomFactor = screenHeight / gameHeight;

	if( pixelZoomFactor < 1 ) {
		pixelZoomFactor = 1;
	}


	if( ! isNonIntegerScalingAllowed() ) {

		// make sure game width fills the screen at this pixel zoom,
		// even if game
		// height does not (letterbox on top/bottom, but never on left/rigtht)

		// closest number of whole pixels
		// may be *slight* black bars on left/right
		gameWidth = screenWidth / pixelZoomFactor;

		screen->setImageSize( pixelZoomFactor * gameWidth,
							  pixelZoomFactor * gameHeight );
	}
	else {

		pixelZoomFactor = 1;

		double targetAspectRatio = (double)gameWidth / (double)gameHeight;

		double screenAspectRatio = (double)screenWidth / (double)screenHeight;

		int imageW = screenWidth;
		int imageH = screenHeight;

		if( screenAspectRatio > targetAspectRatio ) {
			// screen too wide

			imageW = (int)( targetAspectRatio * imageH );
		}
		else if( screenAspectRatio < targetAspectRatio ) {
			// too tall

			imageH = (int)( imageW / targetAspectRatio );
		}

		screen->setImageSize( imageH,
							  imageW );
	}


	screen->allowSlowdownKeysDuringPlayback( enableSpeedControlKeys );

	//SDL_ShowCursor( SDL_DISABLE );


	sceneHandler = new GameSceneHandler( screen );



	// also do file-dependent part of init for GameSceneHandler here
	// actually, constructor is file dependent anyway.
	sceneHandler->initFromFiles();


	// hard to quit mode?
	char hardToQuitFound = false;
	int readHardToQuit = SettingsManager::getIntSetting( "hardToQuitMode",
														 &hardToQuitFound );

	if( readHardToQuit == 1 ) {
		hardToQuitMode = true;
	}

	pauseOnMinimize = SettingsManager::getIntSetting( "pauseOnMinimize", 1 );



	// translation language
	File *languageNameFile = new File( NULL, "language.txt" );

	if( languageNameFile->exists() ) {
		char *languageNameText = languageNameFile->readFileContents();

		SimpleVector<char *> *tokens = tokenizeString( languageNameText );

		int numTokens = tokens->size();

		// first token is name
		if( numTokens > 0 ) {
			char *languageName = *( tokens->getElement( 0 ) );

			TranslationManager::setLanguage( languageName, true );

			// augment translation by adding other languages listed
			// to fill in missing keys in top-line language

			for( int i=1; i<numTokens; i++ ) {
				languageName = *( tokens->getElement( i ) );

				TranslationManager::setLanguage( languageName, false );
			}
		}
		else {
			// default

			// TranslationManager already defaults to English, but
			// it looks for the language files at runtime before we have set
			// the current working directory.

			// Thus, we specify the default again here so that it looks
			// for its language files again.
			TranslationManager::setLanguage( "English", true );
		}

		delete [] languageNameText;

		for( int t=0; t<numTokens; t++ ) {
			delete [] *( tokens->getElement( t ) );
		}
		delete tokens;
	}

	delete languageNameFile;




	// register cleanup function, since screen->start() will never return
	atexit( cleanUpAtExit );




	screen->switchTo2DMode();



	if( getUsesSound() ) {

		soundSampleRate =
				SettingsManager::getIntSetting( "soundSampleRate", 22050 );

		int requestedBufferSize =
				SettingsManager::getIntSetting( "soundBufferSize", 512 );

		// 1 second fade in/out
		soundLoudnessIncrementPerSample = 1.0f / soundSampleRate;

		// force user-specified value to closest (round up) power of 2
		int bufferSize = 2;
		while( bufferSize < requestedBufferSize ) {
			bufferSize *= 2;
		}


		SDL_AudioSpec audioFormat;

		/* Set 16-bit stereo audio at 22Khz */
		audioFormat.freq = soundSampleRate;
		audioFormat.format = AUDIO_S16;
		audioFormat.channels = 2;
		//audioFormat.samples = 512;        /* A good value for games */
		audioFormat.samples = bufferSize;
		audioFormat.callback = audioCallback;
		audioFormat.userdata = NULL;

		SDL_AudioSpec actualFormat;


		int recordAudioFlag =
				SettingsManager::getIntSetting( "recordAudio", 0 );
		int recordAudioLengthInSeconds =
				SettingsManager::getIntSetting( "recordAudioLengthInSeconds", 0 );



		SDL_LockAudio();

		int openResult = 0;

		if( ! recordAudioFlag ) {
			openResult = SDL_OpenAudio( &audioFormat, &actualFormat );
		}



		/* Open the audio device and start playing sound! */
		if( openResult < 0 ) {
			AppLog::getLog()->logPrintf(
					Log::ERROR_LEVEL,
					"Unable to open audio: %s\n", SDL_GetError() );
			soundRunning = false;
			soundOpen = false;
		}
		else {

			if( !recordAudioFlag &&
				( actualFormat.format != AUDIO_S16 ||
				  actualFormat.channels != 2 ) ) {


				AppLog::getLog()->logPrintf(
						Log::ERROR_LEVEL,
						"Able to open audio, "
						"but stereo S16 samples not availabl\n");

				SDL_CloseAudio();
				soundRunning = false;
				soundOpen = false;
			}
			else {

				int desiredRate = soundSampleRate;

				if( !recordAudioFlag ) {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Successfully opened audio: %dHz (requested %dHz), "
							"sample buffer size=%d (requested %d)\n",
							actualFormat.freq, desiredRate, actualFormat.samples,
							bufferSize );

					// tell game what their buffer size will be
					// so they can allocate it outside the callback
					hintBufferSize( actualFormat.samples * 4 );
					bufferSizeHinted = true;
				}
				else {
					AppLog::getLog()->logPrintf(
							Log::INFO_LEVEL,
							"Successfully faked opening of audio for "
							"recording to file: %dHz\n", desiredRate );
					// don't hint buffer size yet
					bufferSizeHinted = false;
				}




				soundSpriteNoClip =
						resetAudioNoClip(
								( 1.0 - soundSpriteCompressionFraction ) *
								maxTotalSoundSpriteVolume * 32767,
								// half second hold and release
								soundSampleRate / 2, soundSampleRate / 2 );

				totalAudioMixNoClip =
						resetAudioNoClip( 32767.0,
								// 10x faster hold and release
								// for master mix
								// pull music and sound effects down
								// to prevent clipping, but bring
								// it right back up again quickly
								// after the transient passes to
								// avoid audible pumping in music
										  soundSampleRate / 20,
										  soundSampleRate / 20 );



				if( !recordAudioFlag ) {
					soundSampleRate = actualFormat.freq;

					soundSpriteMixingBufferL =
							new double[ actualFormat.samples ];
					soundSpriteMixingBufferR =
							new double[ actualFormat.samples ];
				}


				soundRunning = true;
				soundOpen = true;

				if( recordAudioFlag ) {
					soundOpen = false;
				}

				if( recordAudioFlag == 1 && recordAudioLengthInSeconds > 0 ) {
					recordAudio = true;

					samplesLeftToRecord =
							recordAudioLengthInSeconds * soundSampleRate;

					aiffOutFile = fopen( "recordedAudio.aiff", "wb" );

					int headerLength;

					unsigned char *header =
							getAIFFHeader( 2, 16,
										   soundSampleRate,
										   samplesLeftToRecord,
										   &headerLength );

					fwrite( header, 1, headerLength, aiffOutFile );

					delete [] header;
				}



			}
		}

		SDL_UnlockAudio();
	}






	if( ! writeFailed ) {
		demoMode = isDemoMode();
	}



	initDrawString( pixelZoomFactor * gameWidth,
					pixelZoomFactor * gameHeight );



	//glLineWidth( pixelZoomFactor );


	if( demoMode ) {
		showDemoCodePanel( screen, getFontTGAFileName(), gameWidth,
						   gameHeight );

		// wait to start handling events
		// wait to start recording/playback
	}
	else if( writeFailed ) {
		// handle key events right away to listen for ESC
		screen->addKeyboardHandler( sceneHandler );
	}
	else {
		// handle events right away
		screen->addMouseHandler( sceneHandler );
		screen->addKeyboardHandler( sceneHandler );

		if( screen->isPlayingBack() ) {

			// start playback right away
			screen->startRecordingOrPlayback();

			AppLog::infoF( "Using frame rate from recording file:  %d fps\n",
						   screen->getMaxFramerate() );
		}
		// else wait to start recording until after we've measured
		// frame rate
	}


	int readTarget = SettingsManager::getIntSetting( "targetFrameRate", -1 );
	int readCounting = SettingsManager::getIntSetting( "countingOnVsync", -1 );

	if( readTarget != -1 && readCounting != -1 ) {
		targetFrameRate = readTarget;
		countingOnVsync = readCounting;

		screen->setFullFrameRate( targetFrameRate );
		screen->useFrameSleep( !countingOnVsync );
		screen->startRecordingOrPlayback();

		if( screen->isPlayingBack() ) {
			screen->useFrameSleep( true );
		}
		measureFrameRate = false;
	}


	// default texture mode
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );


	screen->start();


	return 0;
}















unsigned int getRandSeed() {
	return screen->getRandSeed();
}



void pauseGame() {
	sceneHandler->mPaused = !( sceneHandler->mPaused );
}


char isPaused() {
	return sceneHandler->mPaused;
}



void blockQuitting( char inNoQuitting ) {
	sceneHandler->mBlockQuitting = inNoQuitting;
}



char isQuittingBlocked() {
	return sceneHandler->mBlockQuitting;
}



void wakeUpPauseFrameRate() {
	sceneHandler->mPausedSleepTime = 0;
}


// returns true if we're currently executing a recorded game
char isGamePlayingBack() {
	return screen->isPlayingBack();
}







void mapKey( unsigned char inFromKey, unsigned char inToKey ) {
	screen->setKeyMapping( inFromKey, inToKey );
}


void toggleKeyMapping( char inMappingOn ) {
	screen->toggleKeyMapping( inMappingOn );
}


void setCursorVisible( char inIsVisible ) {
	if( inIsVisible ) {
		SDL_ShowCursor( SDL_ENABLE );
	}
	else {
		SDL_ShowCursor( SDL_DISABLE );
	}
}



void setCursorMode( int inMode ) {
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


int getCursorMode() {
	return cursorMode;
}



void setEmulatedCursorScale( double inScale ) {
	SettingsManager::setDoubleSetting( "emulatedCursorScale", inScale );
	emulatedCursorScale = inScale;
}



double getEmulatedCursorScale() {
	return emulatedCursorScale;
}




void grabInput( char inGrabOn ) {
	if( inGrabOn ) {
		SDL_WM_GrabInput( SDL_GRAB_ON );
	}
	else {
		SDL_WM_GrabInput( SDL_GRAB_OFF );
	}
}




extern int totalLoadedTextureBytes;

char isShiftKeyDown() {
	SDLMod modState = SDL_GetModState();


	if( ( modState & KMOD_SHIFT ) ) {

		return true;
	}

	if( screen->isPlayingBack() ) {
		// ignore these, saved internally, unless we're playing back
		// they can fall out of sync with keyboard reality as the user
		// alt-tabs between windows and release events are lost.
		if( rShiftDown || lShiftDown ) {
			return true;
		}
	}

	return false;
}



char isLastMouseButtonRight() {
	return screen->isLastMouseButtonRight();
}


// FOVMOD NOTE:  Change 1/1 - Take these lines during the merge process
int getLastMouseButton() {
	return screen->getLastMouseButton();
}



void obscureRecordedNumericTyping( char inObscure,
								   char inCharToRecordInstead ) {

	screen->obscureRecordedNumericTyping( inObscure, inCharToRecordInstead );
}








static Image *readTGAFile( File *inFile ) {

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



Image *readTGAFile( const char *inTGAFileName ) {

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFile( &tgaFile );
}



Image *readTGAFileBase( const char *inTGAFileName ) {

	File tgaFile( NULL, inTGAFileName );

	return readTGAFile( &tgaFile );
}




static RawRGBAImage *readTGAFileRaw( InputStream *inStream ) {
	TGAImageConverter converter;

	RawRGBAImage *result = converter.deformatImageRaw( inStream );


	return result;
}




static RawRGBAImage *readTGAFileRaw( File *inFile ) {

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



RawRGBAImage *readTGAFileRaw( const char *inTGAFileName ) {

	File tgaFile( new Path( "graphics" ), inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}



RawRGBAImage *readTGAFileRawBase( const char *inTGAFileName ) {

	File tgaFile( NULL, inTGAFileName );

	return readTGAFileRaw( &tgaFile );
}




RawRGBAImage *readTGAFileRawFromBuffer( unsigned char *inBuffer,
										int inLength ) {

	ByteBufferInputStream tgaStream( inBuffer, inLength );

	return readTGAFileRaw( &tgaStream );
}





void writeTGAFile( const char *inTGAFileName, Image *inImage ) {
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


	if( screen->isPlayingBack() ) {
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

	if( screen->isPlayingBack() ) {
		// don't step request, because we're only simulating the response
		// of the server

		int nextType = screen->getWebEventType( inHandle );

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

		screen->registerWebEvent( inHandle, stepResult );

		return stepResult;
	}

	return -1;
}



// gets the response body as a \0-terminated string, destroyed by caller
char *getWebResult( int inHandle ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return screen->getWebEventResultBody( inHandle );
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
			screen->registerWebEvent( inHandle,
					// the type for "result" is 2
									  2,
									  result );
		}

		return result;
	}

	return NULL;
}




unsigned char *getWebResult( int inHandle, int *outSize ) {
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

		if( nextType == 2 ) {
			return (unsigned char *)
					screen->getWebEventResultBody( inHandle, outSize );
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
			screen->registerWebEvent( inHandle,
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
	if( screen->isPlayingBack() ) {
		// return a recorded server result

		int nextType = screen->getWebEventType( inHandle );

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
			screen->registerWebEvent( inHandle,
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

	if( screen->isPlayingBack() ) {
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





int nextSocketConnectionHandle = 0;



int openSocketConnection( const char *inNumericalAddress, int inPort ) {
	SocketConnectionRecord r;

	r.handle = nextSocketConnectionHandle;
	nextSocketConnectionHandle++;


	if( screen->isPlayingBack() ) {
		// stop here, don't actually open a real socket
		return r.handle;
	}

	HostAddress address( stringDuplicate( inNumericalAddress ), inPort );


	char timedOut;

	// non-blocking connet
	r.sock = SocketClient::connectToServer( &address, 0, &timedOut );

	if( r.sock != NULL ) {
		socketConnectionRecords.push_back( r );

		return r.handle;
	}
	else {
		return -1;
	}
}



static Socket *getSocketByHandle( int inHandle ) {
	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			return r->sock;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - getSocketByHandle:  "
				   "Requested Socket handle not found\n" );
	return NULL;
}





// non-blocking send
// returns number sent (maybe 0) on success, -1 on error
int sendToSocket( int inHandle, unsigned char *inData, int inDataLength ) {
	if( screen->isPlayingBack() ) {
		// play back result of this send

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
										   &nextType, &nextNumBodyBytes );

		while( nextType == 2 && nextNumBodyBytes == 0 ) {
			// skip over any lingering waiting-for-read events
			// sometimes there are extra in recording that aren't needed
			// on playback for some reason
			screen->getSocketEventTypeAndSize( inHandle,
											   &nextType, &nextNumBodyBytes );
		}


		if( nextType == 0 ) {
			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numSent = 0;


		if( sock->isConnected() ) {

			numSent = sock->send( inData, inDataLength, false, false );

			if( numSent == -2 ) {
				// would block
				numSent = 0;
			}
		}

		int type = 0;

		if( numSent == -1 ) {
			type = 1;
			numSent = 0;
		}

		screen->registerSocketEvent( inHandle, type, numSent, NULL );

		return numSent;
	}

	return -1;
}



// non-blocking read
// returns number of bytes read (maybe 0), -1 on error
int readFromSocket( int inHandle,
					unsigned char *inDataBuffer, int inBytesToRead ) {

	if( screen->isPlayingBack() ) {
		// play back result of this read

		int nextType, nextNumBodyBytes;
		screen->getSocketEventTypeAndSize( inHandle,
										   &nextType, &nextNumBodyBytes );

		if( nextType == 2 ) {

			if( nextNumBodyBytes == 0 ) {
				return 0;
			}
			// else there are body bytes waiting

			if( nextNumBodyBytes > inBytesToRead ) {
				AppLog::errorF( "gameSDL - readFromSocket:  "
								"Expecting to read at most %d bytes, but "
								"recording has %d bytes waiting\n",
								inBytesToRead, nextNumBodyBytes );
				return -1;
			}

			unsigned char *bodyBytes =
					screen->getSocketEventBodyBytes( inHandle );

			memcpy( inDataBuffer, bodyBytes, nextNumBodyBytes );
			delete [] bodyBytes;


			return nextNumBodyBytes;
		}
		else {
			return -1;
		}
	}


	Socket *sock = getSocketByHandle( inHandle );

	if( sock != NULL ) {

		int numRead = 0;

		if( sock->isConnected() ) {

			numRead = sock->receive( inDataBuffer, inBytesToRead, 0 );

			if( numRead == -2 ) {
				// would block
				numRead = 0;
			}
		}

		int type = 2;
		if( numRead == -1 ) {
			type = 3;
		}

		unsigned char *bodyBytes = NULL;
		if( numRead > 0 ) {
			bodyBytes = inDataBuffer;
		}

		screen->registerSocketEvent( inHandle, type, numRead, bodyBytes );

		return numRead;
	}

	return -1;
}



void closeSocket( int inHandle ) {

	if( screen->isPlayingBack() ) {
		// not a real socket, do nothing
		return;
	}

	for( int i=0; i<socketConnectionRecords.size(); i++ ) {
		SocketConnectionRecord *r = socketConnectionRecords.getElement( i );

		if( r->handle == inHandle ) {
			delete r->sock;

			socketConnectionRecords.deleteElement( i );

			// found, done
			return;
		}
	}

	// else not found?
	AppLog::error( "gameSDL - closeSocket:  "
				   "Requested Socket handle not found\n" );
}

timeSec_t game_timeSec() {
	return screen->getTimeSec();
}



double game_getCurrentTime() {
	return screen->getCurrentTime();
}



double getRecentFrameRate() {
	if( screen->isPlayingBack() ) {

		return screen->getRecordedFrameRate();
	}
	else {
		screen->registerActualFrameRate( sceneHandler->mLastFrameRate );

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



// same for all platforms
// load a .wav file
int16_t *load16BitMonoSound( int *outNumSamples, int *outSampleRate ) {

	File wavFile( NULL, arecordFileName );

	if( ! wavFile.exists() ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "File does not exist in game folder: %s\n",
						arecordFileName );
		return NULL;
	}

	char *fileName = wavFile.getFullFileName();

	FILE *file = fopen( fileName, "rb" );

	delete [] fileName;

	if( file == NULL ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Failed to open sound file for reading: %s\n",
						arecordFileName );
		return NULL;
	}

	fseek( file, 0L, SEEK_END );
	int fileSize  = ftell( file );
	rewind( file );

	if( fileSize <= 44 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file too small to contain a WAV header: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}


	// skip 20 bytes of header to get to format flag
	fseek( file, 20, SEEK_SET );

	unsigned char readBuffer[4];

	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 1 || readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not in PCM format: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}


	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 1 || readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not  in mono: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}

	fread( readBuffer, 1, 4, file );

	// little endian
	*outSampleRate =
			(int)( readBuffer[3] << 24 |
				   readBuffer[2] << 16 |
				   readBuffer[1] << 8 |
				   readBuffer[0] );


	fseek( file, 34, SEEK_SET );


	fread( readBuffer, 1, 2, file );

	if( readBuffer[0] != 16 && readBuffer[1] != 0 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Sound file not 16-bit: %s\n",
						arecordFileName );
		fclose( file );
		return NULL;
	}



	/*
	  // this is not reliable as arecord leaves this blank when
	  // recording a stream

	fseek( file, 40, SEEK_SET );


	fread( readBuffer, 1, 4, file );

	// little endian
	int numSampleBytes =
		(int)( readBuffer[3] << 24 |
			   readBuffer[2] << 16 |
			   readBuffer[1] << 8 |
			   readBuffer[0] );

	*/

	fseek( file, 44, SEEK_SET );


	int currentPos = ftell( file );

	int numSampleBytes = fileSize - currentPos;

	*outNumSamples = numSampleBytes / 2;

	int numSamples = *outNumSamples;


	unsigned char *rawSamples = new unsigned char[ 2 * numSamples ];

	int numRead = fread( rawSamples, 1, numSamples * 2, file );


	if( numRead != numSamples * 2 ) {
		AppLog::printOutNextMessage();
		AppLog::errorF( "Failed to read %d samples from file: %s\n",
						numSamples, arecordFileName );

		delete [] rawSamples;
		fclose( file );
		return NULL;
	}


	int16_t *returnSamples = new int16_t[ numSamples ];

	int r = 0;
	for( int i=0; i<numSamples; i++ ) {
		// little endian
		returnSamples[i] =
				( rawSamples[r+1] << 8 ) |
				rawSamples[r];

		r += 2;
	}
	delete [] rawSamples;



	fclose( file );

	return returnSamples;
}




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



