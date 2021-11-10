//
// Created by olivier on 07/11/2021.
//

#include "audioRenderer.h"

#include <cstdio>
#include <SDL/SDL.h>

#include "minorGems/sound/audioNoClip.h"
#include "minorGems/util/random/CustomRandomSource.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/sound/formats/aiff.h"
#include "minorGems/io/file/File.h"
#include "OneLife/gameSource/game.h"
#include "OneLife/gameSource/dataTypes/sound.h"
#include "OneLife/gameSource/components/engines/GameSceneHandler.h"

extern CustomRandomSource randSource;
extern GameSceneHandler *sceneHandler;
extern char recordAudio;
extern const char *arecordFileName;
extern int soundSampleRate;
extern char soundRunning;
extern int samplesLeftToRecord;
extern FILE *aiffOutFile;
static double soundSpriteRateMax = 1.0;
static double soundSpriteRateMin = 1.0;
static double soundSpriteVolumeMax = 1.0;
static double soundSpriteVolumeMin = 1.0;
static int maxSimultaneousSoundSprites = -1;
static int nextSoundSpriteHandle;
static float soundLoudness = 1.0f;
static float currentSoundLoudness = 0.0f;
static float soundSpriteGlobalLoudness = 1.0f;
static char soundSpritesFading = false;
static float soundSpriteFadeIncrementPerSample = 0.0f;
double maxTotalSoundSpriteVolume = 1.0;
double soundSpriteCompressionFraction = 0.0;
static double totalSoundSpriteNormalizeFactor = 1.0;
NoClip totalAudioMixNoClip;
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
float soundLoudnessIncrementPerSample = 0.0f;
NoClip soundSpriteNoClip;

// set the playback rate wiggle range of sound sprites
// If enabled, each play of a sound sprite will pick a random
// rate from in this range
// defaults to [1.0, 1.0] (no variation)
void setSoundSpriteRateRange( double inMin, double inMax ) {
	soundSpriteRateMin = inMin;
	soundSpriteRateMax = inMax;
}

// defaults to [1.0, 1.0] (no variation)
void setSoundSpriteVolumeRange( double inMin, double inMax ) {
	soundSpriteVolumeMin = inMin;
	soundSpriteVolumeMax = inMax;
}

// loudness in [0..1] for sound
// instantaneous
void setSoundLoudness( float inLoudness )
{
	lockAudio();
	soundLoudness = inLoudness;
	currentSoundLoudness = inLoudness;
	unlockAudio();
}

double pickRandomRate() {
	if( soundSpriteRateMax != 1.0 ||
		soundSpriteRateMin != 1.0 ) {

		return randSource.getRandomBoundedDouble( soundSpriteRateMin,
				soundSpriteRateMax );
	}
	else {
		return 1.0;
	}
}

// defaults to -1, no limit
// To respect this limit, newly played sprites that would exceed the limit
// are ignored
void setMaxSimultaneousSoundSprites( int inMaxCount ) {
	maxSimultaneousSoundSprites = inMaxCount;
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

// locking
// plays sound sprite now
// volume tweak multiplies sound volume, should be between 0 and 1
// stereo position in [0,1] for [left,right], with 0.5 centered
// uses constant power law (sine)
void playSoundSprite( SoundSpriteHandle inHandle, double inVolumeTweak,
					  double inStereoPosition ) {

	lockAudio();
	playSoundSpriteInternal( inHandle, inVolumeTweak, inStereoPosition );
	unlockAudio();
}

// no locking
void playSoundSpriteInternal(
		SoundSpriteHandle inHandle,
		double inVolumeTweak,
		double inStereoPosition,
		double inForceVolume,
		double inForceRate) {


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

// multiple with single lock
// plays multiple sound sprites
// guarantees that they will start at the time.
// (separate calls to playSoundSprite don't guarantee this because of
//  audio threading)
void playSoundSprite(
		int inNumSprites,
		SoundSpriteHandle *inHandles,
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

				while( filled < numSamples && samplesPlayed < spriteNumSamples  )
				{
					Sint16 sample = s->samples[ samplesPlayed ];
					soundSpriteMixingBufferL[ filled ] += volumeL * sample;
					soundSpriteMixingBufferR[ filled ] += volumeR * sample;

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
		for( int i=0; i<numSamples; i++ )
		{
			Sint16 lSample = lrint( soundSpriteMixingBufferL[i] );
			Sint16 rSample = lrint( soundSpriteMixingBufferR[i] );

			inStream[filledBytes++] = (Uint8)( lSample & 0xFF );
			inStream[filledBytes++] = (Uint8)( ( lSample >> 8 ) & 0xFF );
			inStream[filledBytes++] = (Uint8)( rSample & 0xFF );
			inStream[filledBytes++] = (Uint8)( ( rSample >> 8 ) & 0xFF );
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

// each rendering platform sets a sample rate
int getSampleRate() {
	return soundSampleRate;
}

// true to start or resume playing
// false to pause
// Audio starts off paused
void setSoundPlaying( char inPlaying ) {
	SDL_PauseAudio( !inPlaying );
}

// for platforms where audio runs in separate thread
// Lock when manipulating data that is touched by getSoundSamples
void lockAudio() {
	SDL_LockAudio();
}

void unlockAudio() {
	SDL_UnlockAudio();
}

// returns true if sound started up and is running
char isSoundRunning() {
	return soundRunning;
}

// fades all sound sprites down to zero loudness over the next inFadeSeconds
// after fade is complete, playSoundSprite calls have no effect until
// resumePlayingSoundSprites is called
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

// if inNoVariance is true, sound ignores global rate and volume range
// defaults to false
void toggleVariance( SoundSpriteHandle inHandle, char inNoVariance ) {
	SoundSprite *s = (SoundSprite*)inHandle;
	s->noVariance = inNoVariance;
}

// max volume defaults to 1.0
// The total volume cap for all currently playing sound sprites.
// To respect the cap, sound sprites are mixed together at their play
// volumes and the mix is gain adjusted by an auto-leveller which
// guarantees that the mix never exceeds this cap.
// This cap is per channel (left and right)
//
// compression fraction defaults to 0
// fraction of total volume cap that is compressed up to full volume
// cap level.
// Example:  if 0.75, than all sounds in top 75% of volume range are
// compressed up to full volume, while sounds in the 0..25% range
// are expanded to cover the full volume dynamic range
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

// same for all platforms
// load a .wav file
// even if sound recording is not supported, we can read
// in a .wav file from a standard location
// inputSoundTemp.wav
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